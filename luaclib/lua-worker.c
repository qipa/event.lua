#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h> 
#include <errno.h>

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#include "message_queue.h"
#include "mail_box.h"

struct startup_args {
	int fd;
	char* args;
};

typedef struct workder_ctx {
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	struct message_queue* queue;
	int id;
	uint32_t session;
	int quit;
	int ref;
	int callback;
	char* startup_args;
	uint32_t oversion;
	uint32_t nversion;
	lua_State* L;

	int fd;
	struct mail_message* mail_first;
	struct mail_message* mail_last;
} worker_ctx_t;

typedef struct worker_manager {
	pthread_mutex_t mutex;
	int size;
	struct workder_ctx** slot;
} worker_manager_t;


static worker_manager_t* _MANAGER = NULL;
static pthread_once_t _MANAGER_INIT = PTHREAD_ONCE_INIT;

void
create_manager() {
	worker_manager_t* wm = malloc(sizeof(*wm));
	memset(wm,0,sizeof(*wm));
	wm->size = 8;
	wm->slot = malloc(sizeof(*wm->slot) * wm->size);
	memset(wm->slot,0,sizeof(*wm->slot) * wm->size);
	pthread_mutex_init(&wm->mutex, NULL);

	_MANAGER = wm;
}

void
add_worker(worker_ctx_t* ctx) {
	pthread_mutex_lock(&_MANAGER->mutex);
	int i;
	for(i = 0;i < _MANAGER->size;i++) {
		worker_ctx_t* node = _MANAGER->slot[i];
		if (node == NULL) {
			_MANAGER->slot[i] = ctx;
			ctx->id = i;
			pthread_mutex_unlock(&_MANAGER->mutex);
			return;
		}
	}

	int nsize = _MANAGER->size * 2;
	worker_ctx_t** nslot = malloc(sizeof(*nslot) * nsize);
	memset(nslot,0,sizeof(*nslot) * nsize);
	for(i=0;i<_MANAGER->size;i++) {
		worker_ctx_t* node = _MANAGER->slot[i];
		assert(node != NULL);
		nslot[i] = node;
	}

	nslot[_MANAGER->size] = ctx;
	ctx->id = _MANAGER->size;
	
	free(_MANAGER->slot);
	_MANAGER->slot = nslot;
	_MANAGER->size = nsize;
	pthread_mutex_unlock(&_MANAGER->mutex);
}

void
delete_worker(int id) {
	pthread_mutex_lock(&_MANAGER->mutex);
	_MANAGER->slot[id] = NULL;
	pthread_mutex_unlock(&_MANAGER->mutex);
}

worker_ctx_t*
worker_ref(int id) {
	pthread_mutex_lock(&_MANAGER->mutex);
	worker_ctx_t* ctx = _MANAGER->slot[id];
	if (ctx) {
		__sync_add_and_fetch(&ctx->ref,1);
	}
	pthread_mutex_unlock(&_MANAGER->mutex);
	return ctx;
}

void worker_release(worker_ctx_t* ctx);

void
worker_unref(worker_ctx_t* ctx) {
	int ref = __sync_sub_and_fetch(&ctx->ref,1);
	if (ref == 0) {
		worker_release(ctx);
	}
}

worker_ctx_t*
worker_create() {
	worker_ctx_t* worker_ctx = malloc(sizeof(*worker_ctx));
	memset(worker_ctx,0,sizeof(*worker_ctx));
	
	pthread_mutex_init(&worker_ctx->mutex, NULL);
	pthread_cond_init(&worker_ctx->cond,NULL);

	worker_ctx->id = -1;
	worker_ctx->session = 1;
	worker_ctx->quit = 0;
	worker_ctx->ref = 1;
	worker_ctx->queue = queue_create();

	add_worker(worker_ctx);
	return worker_ctx;
}

void
workder_quit(worker_ctx_t* ctx) {
	delete_worker(ctx->id);
	worker_unref(ctx);
}

void
worker_release(worker_ctx_t* ctx) {
	queue_free(ctx->queue);
	if (ctx->startup_args) {
		free(ctx->startup_args);
	}
	free(ctx);
}

int
worker_send_mail(worker_ctx_t* ctx,struct mail_message* mail) {
	for (;;) {
		int n = write(ctx->fd, &mail, sizeof(mail));
		if (n < 0) {
			if (errno == EINTR) {
				continue;
			} else if (errno == EAGAIN ) {
				return -1;
			} else {
				fprintf(stderr,"worker send mail error %s.\n", strerror(errno));
				assert(0);
			}
		}
		assert(n == sizeof(mail));
		return 0;
	}
}

void
worker_wakeup(worker_ctx_t* ctx) {
	while(ctx->mail_first) {
		struct mail_message* mail = ctx->mail_first;
		struct mail_message* next_mail = mail->next;
		if (worker_send_mail(ctx,mail) == -1) {
			return;
		} else {
			ctx->mail_first = next_mail;
		}
	}
	ctx->mail_first = ctx->mail_last = NULL;
}

void
worker_callback(worker_ctx_t* ctx,int source,int session,void* data,int size) {
	lua_rawgeti(ctx->L, LUA_REGISTRYINDEX, ctx->callback);

	lua_pushinteger(ctx->L,source);
	lua_pushinteger(ctx->L,session);
	if (data) {
		lua_pushlightuserdata(ctx->L,data);
		lua_pushinteger(ctx->L,size);
		lua_pcall(ctx->L,4,0,0);
	} else {
		lua_pcall(ctx->L,2,0,0);
	}
}

void
worker_dispatch(worker_ctx_t* ctx) {
	struct message_queue* queue_ctx = ctx->queue;
	for(;;) {
		struct queue_message* message = queue_pop(queue_ctx,ctx->id);
		if (message == NULL) {
			pthread_mutex_lock(&ctx->mutex);
			
			struct timespec timeout;
			clock_gettime(CLOCK_REALTIME, &timeout);
			timeout.tv_nsec = timeout.tv_nsec + 1000 * 1000 * 10;
			if (timeout.tv_nsec >= 1000 * 1000 * 1000) {
				timeout.tv_sec++;
				timeout.tv_nsec = timeout.tv_nsec % 1000 * 1000 * 1000;
			}
			pthread_cond_timedwait(&ctx->cond,&ctx->mutex,&timeout);

			pthread_mutex_unlock(&ctx->mutex);
			// usleep(100);
			ctx->nversion++;
			worker_wakeup(ctx);
		} else {
			worker_callback(ctx,message->source,message->session,message->data,message->size);
			if (ctx->quit) {
				workder_quit(ctx);
				break;
			}
		}
	}
}



extern int load_helper(lua_State *L);


int
worker_push(lua_State* L) {
	worker_ctx_t* ctx = lua_touserdata(L, 1);
	int target = lua_tointeger(L,2);
	int session = lua_tointeger(L,3);

	void* data = NULL;
	size_t size = 0;

	switch(lua_type(L,4)) {
		case LUA_TSTRING: {
			const char* str = lua_tolstring(L,4,&size);
			data = malloc(size);
			memcpy(data,str,size);
			break;
		}
		case LUA_TUSERDATA:{
			data = lua_touserdata(L,4);
			size = lua_tointeger(L,5);
			break;
		}
		default:
			break;
	}

	worker_ctx_t* target_ctx = worker_ref(target);
	if (!target_ctx) {
		lua_pushboolean(L,0);
		return 1;
	}

	queue_push(target_ctx->queue,ctx->id,session,data,size);
	if (!pthread_mutex_trylock(&target_ctx->mutex)) {
		pthread_cond_signal(&target_ctx->cond);
		pthread_mutex_unlock(&target_ctx->mutex);
	}

	worker_unref(target_ctx);
	lua_pushboolean(L,1);
	return 1;
}

int
send_mail(lua_State* L) {
	worker_ctx_t* ctx = lua_touserdata(L, 1);
	int session = lua_tointeger(L,2);

	void* data = NULL;
	size_t size = 0;

	switch(lua_type(L,3)) {
		case LUA_TSTRING: {
			const char* str = lua_tolstring(L, 3, &size);
			data = malloc(size);
			memcpy(data,str,size);
			break;
		}
		case LUA_TLIGHTUSERDATA:{
			data = lua_touserdata(L, 3);
			size = lua_tointeger(L, 4);
			break;
		}
		default:
			break;
	}

	struct mail_message* mail = malloc(sizeof(*mail));
	mail->source = ctx->id;
	mail->session = session;
	mail->data = data;
	mail->size = size;
	if (ctx->mail_first == NULL) {
		if (worker_send_mail(ctx,mail) == -1) {
			ctx->mail_first = ctx->mail_last = mail;
		}
	} else {
		ctx->mail_last->next = mail;
		ctx->mail_last = mail;
	}
	return 0;
}

int 
quit(lua_State* L) {
	worker_ctx_t* ctx = lua_touserdata(L, 1);
	ctx->quit = 1;
	return 0;
}

int 
dispatch(lua_State* L) {
	worker_ctx_t* ctx = lua_touserdata(L, 1);
	luaL_checktype(L,2,LUA_TFUNCTION);
	ctx->callback = luaL_ref(L,LUA_REGISTRYINDEX);
	return 0;
}

void*
_worker(void* ud) {
	struct startup_args* args = ud;
	lua_State* L = luaL_newstate();
	luaL_openlibs(L);
	luaL_requiref(L,"helper",load_helper,0);

	luaL_newmetatable(L, "meta_worker");
	const luaL_Reg meta_worker[] = {
		{ "push", worker_push },
		{ "send_mail", send_mail },
		{ "quit", quit },
		{ "dispatch", dispatch },
		{ NULL, NULL },
	};
	luaL_newlib(L,meta_worker);
	lua_setfield(L, -2, "__index");
	lua_pop(L,1);

	lua_settop(L,0);

	if (luaL_loadfile(L,"lualib/worker_startup.lua") != LUA_OK)  {
		fprintf(stderr,"%s\n",lua_tostring(L,-1));
		exit(1);
	}

	worker_ctx_t* ctx = lua_newuserdata(L,sizeof(*ctx));
	memset(ctx,0,sizeof(*ctx));
	
	pthread_mutex_init(&ctx->mutex, NULL);
	pthread_cond_init(&ctx->cond,NULL);

	ctx->id = -1;
	ctx->session = 1;
	ctx->quit = 0;
	ctx->ref = 1;
	ctx->queue = queue_create();
	ctx->fd = args->fd;

	luaL_newmetatable(L,"meta_worker");
 	lua_setmetatable(L, -2);
	lua_pushvalue(L, -1);
	ctx->ref = luaL_ref(L, LUA_REGISTRYINDEX);

	add_worker(ctx);

	int argc = 1;
	int from = 0;
	int i = 0;
	for(;i < strlen(args->args);i++) {
		if (args->args[i] == '@') {
			lua_pushlstring(L,&args->args[from],i - from);
			from = i+1;
			++argc;
		}
	}
	++argc;
	lua_pushlstring(L,&args->args[from],i - from);

	if (lua_pcall(L,argc,0,0) != LUA_OK)  {
		fprintf(stderr,"%s\n",lua_tostring(L,-1));
		exit(1);
	}
	ctx->L = L;
	worker_dispatch(ctx);

	lua_close(L);

	workder_quit(ctx);
	free(args);

	return NULL;
}


int
create(lua_State* L) {
	pthread_once(&_MANAGER_INIT,&create_manager);

	int fd = lua_tointeger(L, 1);
	const char* startup_args = lua_tostring(L, 2);

	struct startup_args* args = malloc(sizeof(*args));
	args->fd = fd;
	args->args = strdup(startup_args);

	pthread_t pid;
	if (pthread_create(&pid, NULL, _worker, args))
		exit(1);

	lua_pushinteger(L, pid);
	return 1;
}

int
main_push(lua_State* L) {
	int target = lua_tointeger(L, 1);
	int session = lua_tointeger(L, 2);

	void* data = NULL;
	size_t size = 0;

	switch(lua_type(L,3)) {
		case LUA_TSTRING: {
			const char* str = lua_tolstring(L, 3, &size);
			data = malloc(size);
			memcpy(data,str,size);
			break;
		}
		case LUA_TLIGHTUSERDATA:{
			data = lua_touserdata(L, 3);
			size = lua_tointeger(L, 4);
			break;
		}
		default: {
			luaL_error(L,"unkown type:%s",lua_typename(L,lua_type(L,3)));
		}
	}

	worker_ctx_t* target_ctx = worker_ref(target);
	if (!target_ctx) {
		lua_pushboolean(L,0);
		return 1;
	}

	queue_push(target_ctx->queue,-1,session,data,size);
	if (!pthread_mutex_trylock(&target_ctx->mutex)) {
		pthread_cond_signal(&target_ctx->cond);
		pthread_mutex_unlock(&target_ctx->mutex);
	}

	worker_unref(target_ctx);
	lua_pushboolean(L,1);
	return 1;
}

int
luaopen_worker_core(lua_State* L) {
	const luaL_Reg l[] = {
		{ "create", create },
		{ "push", main_push },
		{ NULL, NULL },
	};
	luaL_newlib(L, l);
	return 1;
}