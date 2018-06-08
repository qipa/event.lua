#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include <lstate.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifdef _WIN32
#define inline __inline
#endif

typedef struct context {
	lua_Alloc alloc;
	void* ud;
	struct co* co_ctx;
	lua_State* pool;
	lua_State* main;
	struct frame* freelist;
} context_t;

typedef struct frame {
	const char* name;
	const char* source;
	int linedefined;
	int tailcall;
	int alloc_total;
	int alloc_count;
	struct frame* next;
	struct frame* prev;

	struct frame* free_next;
} frame_t;

typedef struct co {
	frame_t* head;
	frame_t* tail;

	struct co* next;
	struct co* prev;

	int alloc_total;
	int alloc_count;
} co_t;



static void *
lalloc (void *ud, void *ptr, size_t osize, size_t nsize) {
	context_t* ctx = ud;
	if (nsize != 0 && nsize > osize) {
		ctx->co_ctx->alloc_count++;
		ctx->co_ctx->alloc_total += nsize - osize;
	}
	return ctx->alloc(ctx->ud, ptr, osize, nsize);
}

static const char*
create_string(context_t* profiler, const char* str) {
	if (str == NULL)
		return str;

	lua_getfield(profiler->pool, 1, str);
	if ( !lua_isnil(profiler->pool, 2) ) {
		char* result = lua_touserdata(profiler->pool, -1);
		lua_pop(profiler->pool, 1);
		return result;
	}
	lua_pop(profiler->pool, 1);

	lua_pushstring(profiler->pool, str);
	char* ptr = (char*)lua_tostring(profiler->pool, -1);
	lua_pushlightuserdata(profiler->pool, ptr);
	lua_settable(profiler->pool, 1);
	return ptr;
}

static inline frame_t*
create_frame(context_t* ctx) {
	if ( ctx->freelist) {
		frame_t* frame = ctx->freelist;
		ctx->freelist = frame->free_next;
		memset(frame, 0, sizeof( *frame ));
		return frame;
	}
	frame_t* frame = malloc(sizeof( *frame ));
	memset(frame, 0, sizeof( *frame ));
	return frame;
}

static inline void
delete_frame(context_t* ctx,frame_t* frame) {
	frame->free_next = ctx->freelist;
	ctx->freelist = frame;
}

void 
push_frame(co_t* co_ctx, context_t* profiler, int tailcall, const char* name, const char* source, int linedefined) {
	frame_t* frame = create_frame(profiler);

	frame->name = create_string(profiler, name);
	frame->source = create_string(profiler, source);
	frame->linedefined = linedefined;
	frame->tailcall = tailcall;
	frame->alloc_count = co_ctx->alloc_count;
	frame->alloc_total = co_ctx->alloc_total;
	frame->next = frame->prev = NULL;

	if (co_ctx->head == NULL) {
		assert(co_ctx->head == co_ctx->tail);
		co_ctx->head = co_ctx->tail = frame;
	}
	else {
		co_ctx->tail->next = frame;
		frame->prev = co_ctx->tail;
		co_ctx->tail = frame;
	}
}

void
pop_frame(co_t* co_ctx, context_t* profiler) {
	assert(co_ctx->head != NULL);
	assert(co_ctx->tail != NULL);
	
	frame_t* frame = NULL;
	while (1) {
		frame = co_ctx->tail;
		if ( frame == co_ctx->head ) {
			co_ctx->head = co_ctx->tail = NULL;
		}
		else {
			co_ctx->tail = frame->prev;
			co_ctx->tail->next = NULL;
		}
		if ( co_ctx->tail == NULL || frame->tailcall == 0 )
		{
			break;
		}
	}

	frame->alloc_count = co_ctx->alloc_count - frame->alloc_count;
	frame->alloc_total = co_ctx->alloc_total - frame->alloc_total;

	//printf("%s,%s,%d,%d:%d,%d\n", frame->name, frame->source, frame->linedefined,frame->currentline, frame->alloc_count, frame->alloc_total);

	lua_getfield(profiler->main, LUA_REGISTRYINDEX, "profiler_record");
	const char* key = lua_pushfstring(profiler->main, "%s:%d:%s", frame->source, frame->linedefined, frame->name);
	lua_pushvalue(profiler->main, -1);
	lua_gettable(profiler->main, -3);
	if ( lua_isnil(profiler->main, -1)) {
		lua_pop(profiler->main, 1);
		lua_newtable(profiler->main);
		lua_pushinteger(profiler->main, frame->alloc_count);
		lua_setfield(profiler->main, -2, "alloc_count");
		lua_pushinteger(profiler->main, frame->alloc_total);
		lua_setfield(profiler->main, -2, "alloc_total");
		lua_setfield(profiler->main, -3, key);
		lua_pop(profiler->main, 2);
	}
	else {
		lua_getfield(profiler->main, -1, "alloc_count");
		int count = lua_tointeger(profiler->main, -1);
		lua_pop(profiler->main, 1);
		lua_pushinteger(profiler->main, count + frame->alloc_count);
		lua_setfield(profiler->main, -2, "alloc_count");

		lua_getfield(profiler->main, -1, "alloc_total");
		int total = lua_tointeger(profiler->main, -1);
		lua_pop(profiler->main, 1);
		lua_pushinteger(profiler->main, total + frame->alloc_total);
		lua_setfield(profiler->main, -2, "alloc_total");

		lua_pop(profiler->main, 3);
	}


	delete_frame(profiler, frame);
}

static void 
lhook (lua_State *L, lua_Debug *ar) {
	lua_getfield(L, LUA_REGISTRYINDEX, "profiler");
	lua_pushthread(L);
	lua_gettable(L, -2);
	co_t* co_ctx = lua_touserdata(L, -1);
	lua_pop(L, 2);
	if ( co_ctx->head == NULL ) {
		if ( ar->event == LUA_HOOKRET )
			return;
	}

	context_t* ctx;
	lua_getallocf(L, (void**)&ctx);
	
	lua_getinfo(L, "nS", ar);

	if ( ar->event == LUA_HOOKCALL || ar->event == LUA_HOOKTAILCALL ) {
		push_frame(co_ctx, ctx, ar->event == LUA_HOOKTAILCALL, ar->name, ar->source, ar->linedefined);
	}
	else {
		assert(ar->event == LUA_HOOKRET);
		pop_frame(co_ctx, ctx);
	}
}

static int 
lco_create(lua_State* L) {
	lua_CFunction co_create = lua_tocfunction(L, lua_upvalueindex(1));
	int status = co_create(L);
	lua_State* co = lua_tothread(L, -1);
	lua_sethook(co, lhook, LUA_MASKCALL | LUA_MASKRET, 0);

	co_t* co_ctx = malloc(sizeof( *co_ctx ));
	memset(co_ctx, 0, sizeof( *co_ctx ));
	lua_getfield(L, LUA_REGISTRYINDEX, "profiler");
	lua_pushvalue(L, -2);
	lua_pushlightuserdata(L, co_ctx);
	lua_settable(L, -3);

	lua_pop(L, 1);

	return status;
}

static int
lco_resume(lua_State* L) {
	lua_getfield(L, LUA_REGISTRYINDEX, "profiler");

	lua_pushvalue(L, 1);
	lua_gettable(L, -2);
	co_t* next_co_ctx = lua_touserdata(L, -1);
	lua_pop(L, 1);

	lua_pushthread(L);
	lua_gettable(L, -2);
	co_t* current_co_ctx = lua_touserdata(L, -1);
	lua_pop(L, 1);

	lua_pop(L, 1);

	current_co_ctx->next = next_co_ctx;
	next_co_ctx->prev = current_co_ctx;

	context_t* ctx;
	lua_getallocf(L, (void**)&ctx);
	ctx->co_ctx = next_co_ctx;

	lua_CFunction co_resume = lua_tocfunction(L, lua_upvalueindex(1));
	return co_resume(L);
}

static int
lco_yield(lua_State* L) {
	lua_getfield(L, LUA_REGISTRYINDEX, "profiler");
	lua_pushthread(L);
	lua_gettable(L, -2);
	co_t* co_ctx = lua_touserdata(L, -1);
	lua_pop(L, 2);

	context_t* ctx;
	lua_getallocf(L, (void**)&ctx);

	co_t* prev_co_ctx = co_ctx->prev;
	co_ctx->prev = NULL;
	prev_co_ctx->next = NULL;
	ctx->co_ctx = prev_co_ctx;

	lua_CFunction co_yield = lua_tocfunction(L, lua_upvalueindex(1));
	return co_yield(L);
}

static int
lstop(lua_State* L) {
	context_t* ctx = lua_touserdata(L, lua_upvalueindex(1));
	lua_setallocf(L, ctx->alloc, ctx->ud);

	lua_getglobal(L, "coroutine");

	lua_pushvalue(L, lua_upvalueindex(2));
	lua_setfield(L, -2, "create");

	lua_pushvalue(L, lua_upvalueindex(3));
	lua_setfield(L, -2, "resume");

	lua_pushvalue(L, lua_upvalueindex(4));
	lua_setfield(L, -2, "yield");

	lua_getfield(L, LUA_REGISTRYINDEX, "profiler");
	lua_pushnil(L);
	while (lua_next(L, -2) != 0)	{
		lua_State* co = lua_tothread(L, -2);
		co_t* co_ctx = lua_touserdata(L, -1);

		frame_t* cursor = co_ctx->head;
		while ( cursor ) {
			frame_t* tmp = cursor;
			cursor = cursor->free_next;
			free(tmp);
		}

		free(co_ctx);
		lua_sethook(co, NULL, 0, 0);
		lua_pop(L, 1);
	}

	lua_pushnil(L);
	lua_setfield(L, LUA_REGISTRYINDEX, "profiler");

	frame_t* cursor = ctx->freelist;
	while ( cursor ) {
		frame_t* tmp = cursor;
		cursor = cursor->free_next;
		free(tmp);
	}
	lua_close(ctx->pool);
	free(ctx);

	lua_getfield(L, LUA_REGISTRYINDEX, "profiler_record");
	lua_pushnil(L);
	lua_setfield(L, LUA_REGISTRYINDEX, "profiler_record");
	return 1;
}

int
lprofiler_start(lua_State* L) {
	if (L != G(L)->mainthread) {
		luaL_error(L, "profiler only start in main coroutine");
	}
	context_t* ctx = malloc(sizeof(*ctx));
	memset(ctx, 0, sizeof(*ctx));

	ctx->alloc = lua_getallocf(L, &ctx->ud);

	ctx->pool = luaL_newstate();
	luaL_openlibs(ctx->pool);
	lua_settop(ctx->pool, 0);
	lua_newtable(ctx->pool);

	co_t* co_ctx = malloc(sizeof( *co_ctx ));
	memset(co_ctx, 0, sizeof( *co_ctx ));

	ctx->co_ctx = co_ctx;
	lua_setallocf(L, lalloc, ctx);

	ctx->main = G(L)->mainthread;

	lua_newtable(L);
	lua_setfield(L, LUA_REGISTRYINDEX, "profiler_record");
	
	lua_newtable(L);
	lua_pushthread(L);
	lua_pushlightuserdata(L, co_ctx);
	lua_settable(L, -3);

	lua_setfield(L, LUA_REGISTRYINDEX, "profiler");

	lua_getglobal(L, "coroutine");
	lua_getfield(L, -1, "create");
	lua_CFunction co_create = lua_tocfunction(L, -1);
	lua_pushlightuserdata(L, ctx);
	lua_pushcclosure(L, lco_create, 2);
	lua_setfield(L, -2, "create");
	lua_pop(L, 1);

	lua_getglobal(L, "coroutine");
	lua_getfield(L, -1, "resume");
	lua_CFunction co_resume = lua_tocfunction(L, -1);
	lua_pushlightuserdata(L, ctx);
	lua_pushcclosure(L, lco_resume, 2);
	lua_setfield(L, -2, "resume");
	lua_pop(L, 1);

	lua_getglobal(L, "coroutine");
	lua_getfield(L, -1, "yield");
	lua_CFunction co_yield = lua_tocfunction(L, -1);
	lua_pushlightuserdata(L, ctx);
	lua_pushcclosure(L, lco_yield, 2);
	lua_setfield(L, -2, "yield");
	lua_pop(L, 1);

	lua_pushlightuserdata(L, ctx);
	lua_pushcfunction(L, co_create);
	lua_pushcfunction(L, co_resume);
	lua_pushcfunction(L, co_yield);
	lua_pushcclosure(L, lstop, 4);

	lua_sethook(L, lhook, LUA_MASKCALL | LUA_MASKRET, 0);

	return 1;
}