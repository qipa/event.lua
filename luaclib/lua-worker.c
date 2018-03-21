#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h> 

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#include "message_queue.h"

typedef struct workder_ctx {
	struct message_queue* queue;
	int id;
	uint32_t session;
	int quit;
	int ref;
	char* startup_args;
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

void
worker_callback(lua_State* L,uint32_t source,uint32_t session,void* data,int size) {
	lua_pushvalue(L,-1);
	lua_pushinteger(L,source);
	lua_pushinteger(L,session);
	if (data) {
		lua_pushlightuserdata(L,data);
		lua_pushinteger(L,size);
		lua_pcall(L,4,0,0);
	} else {
		lua_pcall(L,2,0,0);
	}

}

extern int load_helper(lua_State *L);

void*
_worker(void* ud) {
	worker_ctx_t* worker_ctx = ud;
	lua_State* L = luaL_newstate();
	luaL_openlibs(L);
	luaL_requiref(L,"helper",load_helper,0);

	int r = luaL_loadfile(L,"lualib/bootstrap.lua");
	if (r != LUA_OK)  {
		fprintf(stderr,"%s\n",lua_tostring(L,-1));
		exit(1);
	}

	int argc = 0;
	int from = 0;
	int i = 0;
	for(;i < strlen(worker_ctx->startup_args);i++) {
		if (worker_ctx->startup_args[i] == '@') {
			lua_pushlstring(L,&worker_ctx->startup_args[from],i - from);
			from = i+1;
			++argc;
		}
	}
	++argc;
	lua_pushlstring(L,&worker_ctx->startup_args[from],i - from);

	r = lua_pcall(L,argc,0,0);
	if (r != LUA_OK)  {
		fprintf(stderr,"%s\n",lua_tostring(L,-1));
		exit(1);
	}
	lua_close(L);

	workder_quit(worker_ctx);

	return NULL;
}


int
create(lua_State* L) {
	pthread_once(&_MANAGER_INIT,&create_manager);

	int top = lua_gettop(L);
	
	if (top > 1) {
		pthread_t* group = malloc(sizeof(pthread_t) * top);

		int i = 1;
		for(;i <= top;i++) {
			const char* args = lua_tostring(L,i);
			worker_ctx_t* worker_ctx = worker_create();
			args = lua_pushfstring(L,"%d@%s",worker_ctx->id,args);
			worker_ctx->startup_args = strdup(args);
			pthread_t pid;
			if (pthread_create(&pid, NULL, _worker, worker_ctx)) {
				exit(1);
			}
			group[i-1] = pid;
		}

		for(i=0;i<top;i++) {
			void* status;
			pthread_join(group[i],&status);
		}
		free(group);
	} else {
		const char* args = lua_tostring(L,1);
		worker_ctx_t* worker_ctx = worker_create();
		args = lua_pushfstring(L,"%d@%s",worker_ctx->id,args);
		worker_ctx->startup_args = strdup(args);
		_worker(worker_ctx);
	}
	return 0;
}

int
push(lua_State* L) {
	int id = lua_tointeger(L,1);
	int source = lua_tointeger(L,2);
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

	worker_ctx_t* worker_ctx = worker_ref(id);
	if (!worker_ctx) {
		lua_pushboolean(L,0);
		return 1;
	}

	queue_push(worker_ctx->queue,source,session,data,size);
	worker_unref(worker_ctx);
	lua_pushboolean(L,1);
	return 1;
}

int
update(lua_State* L) {
	int id = lua_tointeger(L,1);
	luaL_checktype(L,2,LUA_TFUNCTION);

	worker_ctx_t* worker_ctx = worker_ref(id);
	if (!worker_ctx)
		return 0;
	struct message_queue* queue_ctx = worker_ctx->queue;
	for(;;) {
		struct queue_message* message = queue_pop(queue_ctx,worker_ctx->id);
		if (message == NULL) {
			break;
		} else {
			worker_callback(L,message->source,message->session,message->data,message->size);
			free(message->data);
		}
	}
	worker_unref(worker_ctx);
	return 0;
}

int
luaopen_worker_core(lua_State* L) {
	const luaL_Reg l[] = {
		{ "create", create },
		{ "push", push },
		{ "update", update },
		{ NULL, NULL },
	};
	luaL_newlib(L, l);
	return 1;
}