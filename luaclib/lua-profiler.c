
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

typedef struct state_node {
	struct state_node* prev;
	struct state_node* next;
	lua_State* L;
} state_node_t;

typedef struct state_list {
	lua_State* L;
	lua_State* report;
	state_node_t* head;
	state_node_t* tail;
	state_node_t* freelist;
} state_list_t;


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
	state_list_t* state_list = pthread_getspecific(profiler_key);
	if (!state_list->report) {
		return;
	}
	
	lua_getglobal(state_list->report, "collect");
	
	int argc = 0;

	state_node_t* cursor = state_list->tail;
	while(cursor) {
		traceback(cursor->L,state_list->report,&argc);
		cursor = cursor->prev;
	}
	printf("\033c");
	if (lua_pcall(state_list->report,argc,0,0) != LUA_OK)  {
		fprintf(stderr,"%s\n",lua_tostring(state_list->report,-1));
	}
}

static void
signal_profiler(int sig, siginfo_t* sinfo, void* ucontext) {
	state_list_t* state_list = pthread_getspecific(profiler_key);
	if (!state_list->report) {
		return;
	}
	lua_sethook(state_list->tail->L,hook_func, LUA_MASKCOUNT, 1);
}

static inline state_node_t*
link_node(state_list_t* state_list,lua_State* L) {
	state_node_t* node = NULL;
	if (state_list->freelist) {
		node = state_list->freelist;
		state_list->freelist = node->next;
	} else {
		node = malloc(sizeof(state_node_t));
	}
	memset(node,0,sizeof(state_node_t));

	node->L = L;
	node->prev = NULL;
	node->next = NULL;

	if (state_list->head == NULL) {
		assert(state_list->tail == NULL);
		state_list->head = state_list->tail = node;
	} else {
		node->prev = state_list->tail;
		state_list->tail->next = node;
		state_list->tail = node;
	}

	return node;
}

static inline void
unlink_node(state_list_t* state_list,state_node_t* node) {
	assert(state_list->tail == node);
	state_list->tail = state_list->tail->prev;
	state_list->tail->next = NULL;
	if (state_list->tail == state_list->head) {
		state_list->tail->prev = state_list->tail->next = NULL;
	}
	node->next = state_list->freelist;
	state_list->freelist = node;
}

static int
lstart(lua_State *L) {
	state_list_t* state_list = lua_touserdata(L,lua_upvalueindex(1));
	state_list->L = G(L)->mainthread;
	state_list->report = luaL_newstate();
	luaL_openlibs(state_list->report);

	int status = luaL_loadfile(state_list->report,"lualib/profiler_collect.lua");
	if (status != LUA_OK)  {
		luaL_error(L,"load profiler report failed:%s",lua_tostring(state_list->report,-1));
	}

	status = lua_pcall(state_list->report,0,0,0);
	if (status != LUA_OK)  {
		luaL_error(L,"init profiler report failed:%s",lua_tostring(state_list->report,-1));
	}

	lua_getglobal(state_list->report, "collect_start");
	status = lua_pcall(state_list->report,0,0,0);
	if (status != LUA_OK)  {
		luaL_error(L,"start profiler report failed:%s",lua_tostring(state_list->report,-1));
	}

	pthread_setspecific(profiler_key, (void *)state_list);

	start_profiler();
	return 0;
}

static int
lstop(lua_State *L) {
	stop_profiler();

	const char* file = lua_tostring(L,1);

	state_list_t* state_list = lua_touserdata(L,lua_upvalueindex(1));
	lua_State* report = state_list->report;
	state_list->report = NULL;

	lua_getglobal(report, "collect_over");
	lua_pushstring(report,file);
	int status = lua_pcall(report,1,0,0);
	if (status != LUA_OK)  {
		lua_close(report);
		luaL_error(L,"stop profiler report failed:%s",lua_tostring(report,-1));
	}
	lua_close(report);

	return 0;
}

static int
lresume(lua_State *L) {
	state_list_t* state_list = lua_touserdata(L,lua_upvalueindex(1));
	lua_State* co = lua_tothread(L,1);
	
	state_node_t* node = link_node(state_list,co);

	lua_CFunction co_resume = lua_tocfunction(L, lua_upvalueindex(2));
	int status = co_resume(L);
	unlink_node(state_list,node);
	return status;
}

static int
_release(lua_State *L) {
	state_list_t* state_list = lua_touserdata(L,1);

	state_node_t* cursor = state_list->head;
	while(cursor) {
		state_node_t* tmp = cursor;
		cursor = cursor->next;
		free(tmp);
	}

	cursor = state_list->freelist;
	while(cursor) {
		state_node_t* tmp = cursor;
		cursor = cursor->next;
		free(tmp);
	}

	return 0;
}

int
luaopen_profiler_core(lua_State *L) {
	pthread_once(&init_once,&profiler_key_init);

	luaL_checkversion(L);
	luaL_Reg l[] = {
		{ "start", lstart },
		{ "stop", lstop },
		{ "resume", lresume },
		{ NULL, NULL },
	};
	luaL_newlibtable(L, l);

	state_list_t* state_list = lua_newuserdata(L,sizeof(state_list_t));
	state_list->head = state_list->tail = NULL;
	state_list->freelist = NULL;
	if (luaL_newmetatable(L, "meta_profiler")) {
		lua_pushcfunction(L, _release);
		lua_setfield(L, -2, "__gc");
	}
	lua_setmetatable(L, -2);

	if (L != G(L)->mainthread) {
		luaL_error(L,"profiler must init in main thread");
	}
	link_node(state_list,L);

	lua_pushnil(L);
	luaL_setfuncs(L,l,2);

	lua_getglobal(L, "coroutine");
	lua_getfield(L, -1, "resume");

	lua_CFunction co_resume = lua_tocfunction(L, -1);
	if (co_resume == NULL)
		return luaL_error(L, "Can't get coroutine.resume");
	lua_pop(L,2);

	lua_getfield(L, -1, "resume");
	lua_pushcfunction(L, co_resume);
	lua_setupvalue(L, -2, 2);
	lua_pop(L,1);

	return 1;
}
