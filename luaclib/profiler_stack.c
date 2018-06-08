
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include <lstate.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h> 
#include <sys/types.h>
#include <sys/syscall.h>

#include <execinfo.h>

typedef struct co_node {
	struct co_node* prev;
	struct co_node* next;
	lua_State* L;
} co_node_t;

typedef struct context {
	const char* file;
	lua_State* L;
	lua_State* report;
	co_node_t* head;
	co_node_t* tail;
	co_node_t* freelist;
} context_t;


static pthread_once_t init_once = PTHREAD_ONCE_INIT;
static pthread_key_t profiler_key;

void
profiler_key_init() {
	if (pthread_key_create(&profiler_key, NULL)) {
		fprintf(stderr, "pthread_key_create failed");
		exit(1);
	}
}

static void signal_profiler(int sig, siginfo_t* sinfo, void* ucontext);

static inline void
start_profiler() {
	struct sigaction sa;
	sa.sa_sigaction = signal_profiler;
	sa.sa_flags = SA_RESTART | SA_SIGINFO;
	sigemptyset(&sa.sa_mask);
	if (sigaction(SIGPROF, &sa, NULL) != 0) {
		perror("sigaction");
		exit(1);
	}

	struct itimerval timer;
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = 1000;
	timer.it_value = timer.it_interval;
	setitimer(ITIMER_PROF, &timer, 0);
}

static inline void
stop_profiler() {
	struct sigaction sa;
	sa.sa_handler = SIG_IGN;
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGPROF, &sa, 0);

	struct itimerval timer;
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = 0;
	timer.it_value = timer.it_interval;
	setitimer(ITIMER_PROF, &timer, 0);
}

void
traceback(lua_State* from,lua_State* L,int* argc) {
	int level = 0;
	lua_Debug ar;
	while (lua_getstack(from, level++, &ar)) {
		if (lua_getinfo(from, "Slnt", &ar) > 0) {
			if (ar.currentline >= 0) {
				if (*ar.namewhat != '\0')  /* is there a name from code? */
					lua_pushfstring(L, "%s:%d@%s '%s:%d'",ar.short_src,ar.currentline, ar.namewhat, ar.name, ar.linedefined);  /* use it */
				else if (*ar.what == 'm')  /* main? */
					lua_pushfstring(L, "%s:%d@main chunk",ar.short_src,ar.currentline);
				else if (*ar.what != 'C')  /* for Lua functions, use <file:line> */
					lua_pushfstring(L, "%s:%d@function <%s:%d>",ar.short_src,ar.currentline, ar.short_src, ar.linedefined);
				else  /* nothing left... */
					lua_pushfstring(L, "%s:%d@?",ar.short_src,ar.currentline);
			} else {
				if (*ar.namewhat != '\0')
					lua_pushfstring(L, "%s@%s '%s:%d'",ar.short_src, ar.namewhat, ar.name,ar.linedefined);  /* use it */
				else if (*ar.what == 'm')
					lua_pushfstring(L, "%s@main chunk",ar.short_src);
				else if (*ar.what != 'C')
					lua_pushfstring(L, "%s@function <%s:%d>",ar.short_src, ar.short_src, ar.linedefined);
				else 
					lua_pushfstring(L, "%s@?",ar.short_src);
			}
			(*argc) += 1;
		}
	}
}

static void 
hook_func (lua_State *L, lua_Debug *ar) {
	lua_sethook(L, NULL, 0, 0);
	context_t* ctx = pthread_getspecific(profiler_key);
	if (ctx == NULL)
		return;

	lua_getglobal(ctx->report, "collect");
	
	int argc = 0;

	co_node_t* cursor = ctx->tail;
	while(cursor) {
		traceback(cursor->L,ctx->report,&argc);
		cursor = cursor->prev;
	}
	// printf("\033c");
	if (lua_pcall(ctx->report,argc,0,0) != LUA_OK)  {
		fprintf(stderr,"%s\n",lua_tostring(ctx->report,-1));
	}
	start_profiler();
}

static void
signal_profiler(int sig, siginfo_t* sinfo, void* ucontext) {
	context_t* ctx = pthread_getspecific(profiler_key);
	stop_profiler();
	lua_sethook(ctx->tail->L,hook_func, LUA_MASKCOUNT, 1);
}

static inline co_node_t*
link_co(context_t* ctx,lua_State* L) {
	co_node_t* node = NULL;
	if (ctx->freelist) {
		node = ctx->freelist;
		ctx->freelist = node->next;
	} else {
		node = malloc(sizeof(co_node_t));
	}
	memset(node,0,sizeof(co_node_t));

	node->L = L;
	node->prev = NULL;
	node->next = NULL;

	if (ctx->head == NULL) {
		assert(ctx->tail == NULL);
		ctx->head = ctx->tail = node;
	} else {
		node->prev = ctx->tail;
		ctx->tail->next = node;
		ctx->tail = node;
	}

	return node;
}

static inline void
unlink_co(context_t* ctx,co_node_t* node) {
	assert(ctx->tail == node);
	ctx->tail = ctx->tail->prev;
	ctx->tail->next = NULL;
	if (ctx->tail == ctx->head) {
		ctx->tail->prev = ctx->tail->next = NULL;
	}
	node->next = ctx->freelist;
	ctx->freelist = node;
}

static int
lresume(lua_State *L) {
	context_t* ctx = lua_touserdata(L,lua_upvalueindex(1));
	lua_State* co = lua_tothread(L,1);
	
	co_node_t* node = link_co(ctx,co);

	lua_CFunction co_resume = lua_tocfunction(L, lua_upvalueindex(2));
	int status = co_resume(L);
	unlink_co(ctx,node);
	return status;
}

static int
lstop(lua_State *L) {
	stop_profiler();

	pthread_setspecific(profiler_key, 0);

	lua_getglobal(L, "coroutine");
	lua_pushvalue(L, lua_upvalueindex(2));
	lua_setfield(L, -2, "resume");
	lua_pop(L, 1);

	context_t* ctx = lua_touserdata(L,lua_upvalueindex(1));

	lua_State* report = ctx->report;
	ctx->report = NULL;

	lua_getglobal(report, "collect_over");
	lua_pushstring(report,ctx->file);
	int status = lua_pcall(report,1,0,0);
	if (status != LUA_OK)  {
		lua_close(report);
		luaL_error(L,"stop profiler report failed:%s",lua_tostring(report,-1));
	}
	lua_close(report);

	co_node_t* cursor = ctx->head;
	while(cursor) {
		co_node_t* tmp = cursor;
		cursor = cursor->next;
		free(tmp);
	}

	cursor = ctx->freelist;
	while(cursor) {
		co_node_t* tmp = cursor;
		cursor = cursor->next;
		free(tmp);
	}

	return 0;
}

int
lprofiler_stack_start(lua_State *L) {
	if (L != G(L)->mainthread) {
		luaL_error(L, "stack top only start in main coroutine");
	}

	size_t size;
	const char* file = lua_tolstring(L, 1, &size);

	pthread_once(&init_once,&profiler_key_init);

	context_t* ctx = malloc(sizeof(*ctx));
	ctx->file = strdup(file);
	ctx->head = ctx->tail = NULL;
	ctx->freelist = NULL;
	ctx->L = G(L)->mainthread;
	ctx->report = luaL_newstate();
	luaL_openlibs(ctx->report);

	int status = luaL_loadfile(ctx->report,"lualib/profiler_collect.lua");
	if (status != LUA_OK)  {
		luaL_error(L,"load profiler report failed:%s",lua_tostring(ctx->report,-1));
	}

	status = lua_pcall(ctx->report,0,0,0);
	if (status != LUA_OK)  {
		luaL_error(L,"init profiler report failed:%s",lua_tostring(ctx->report,-1));
	}

	lua_getglobal(ctx->report, "collect_start");
	status = lua_pcall(ctx->report,0,0,0);
	if (status != LUA_OK)  {
		luaL_error(L,"start profiler report failed:%s",lua_tostring(ctx->report,-1));
	}

	pthread_setspecific(profiler_key, (void *)ctx);

	lua_getglobal(L, "coroutine");
	lua_getfield(L, -1, "resume");
	lua_CFunction co_resume = lua_tocfunction(L, -1);
	lua_pushlightuserdata(L, ctx);
	lua_pushcclosure(L, lresume, 2);
	lua_setfield(L, -2, "resume");
	lua_pop(L, 1);

	lua_pushlightuserdata(L, ctx);
	lua_pushcfunction(L, co_resume);
	lua_pushcclosure(L, lstop, 2);

	link_co(ctx,L);

	start_profiler();

	return 1;
}
