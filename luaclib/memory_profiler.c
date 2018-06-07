#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include <lstate.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

typedef struct context {
	lua_Alloc alloc;
	void* ud;
	struct co* co_ctx;
	lua_State* pool;
} context_t;

typedef struct frame {
	const char* name;
	const char* source;
	int linedefined;
	int tailcall;
	int currentline;
	int alloc_total;
	int alloc_count;
	struct frame* next;
	struct frame* prev;
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
	if (nsize != 0) {
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

void 
push_frame(co_t* co_ctx, context_t* profiler, int tailcall, const char* name, const char* source, int linedefined, int currentline) {
	frame_t* frame = malloc(sizeof( *frame ));
	frame->name = create_string(profiler, name);
	frame->source = create_string(profiler, source);
	frame->linedefined = linedefined;
	frame->currentline = currentline;
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
	
	int count = co_ctx->alloc_count;
	int total = co_ctx->alloc_total;
	frame->alloc_count = co_ctx->alloc_count - frame->alloc_count;
	frame->alloc_total = co_ctx->alloc_total - frame->alloc_total;

	printf("%s,%s,%d:%d,%d\n", frame->name, frame->source, frame->linedefined, frame->alloc_count, frame->alloc_total);
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
	lua_getallocf(L, &ctx);
	
	int currentline;
	lua_Debug previous_ar;

	if ( lua_getstack(L, 1, &previous_ar) == 0 ) {
		currentline = -1;
	}
	else {
		lua_getinfo(L, "l", &previous_ar);
		currentline = previous_ar.currentline;
	}

	lua_getinfo(L, "nS", ar);

	//switch ( ar->event )
	//{
	//case LUA_HOOKCALL:
	//printf("call,");
	//break;
	//case LUA_HOOKTAILCALL:
	//printf("tailcall,");
	//break;
	//case LUA_HOOKRET:
	//printf("return,");
	//break;
	//default:
	//break;
	//}
	//printf("%s,%s,%d,%d,%d,%d\n", ar->name, ar->source, ar->linedefined, currentline,ctx->alloc_total,ctx->alloc_count);

	if ( ar->event == LUA_HOOKCALL || ar->event == LUA_HOOKTAILCALL ) {
		push_frame(co_ctx, ctx, ar->event == LUA_HOOKTAILCALL, ar->name, ar->source, ar->linedefined, currentline);
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
	lua_getallocf(L, &ctx);
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
	lua_getallocf(L, &ctx);

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
		free(co_ctx);
		lua_sethook(co, NULL, 0, 0);
		lua_pop(L, 1);
	}

	lua_pushnil(L);
	lua_setfield(L, LUA_REGISTRYINDEX, "profiler");

	lua_close(ctx->pool);
	free(ctx);
	return 0;
}

int
lprofiler_start(lua_State* L) {
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