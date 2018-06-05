#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include <lstate.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

struct profiler_context {
	uint alloc_total;
	uint alloc_count;
	lua_Alloc alloc;
	void* ud;
	lua_State* L;
};

static void *
lalloc (void *ud, void *ptr, size_t osize, size_t nsize) {
	struct profiler_context* ctx = ud;
	if (nsize != 0) {
		ctx->alloc_count++;
		ctx->alloc_total += nsize - osize;
	}
	return ctx->alloc(ctx->ud, ptr, osize, nsize);
}

static void 
lhook (lua_State *L, lua_Debug *ar) {
	
}

static int
lresume(lua_State* L) {
	lua_CFunction co_resume = lua_tocfunction(L, lua_upvalueindex(1));
	struct profiler_context* ctx = lua_touserdata(L, lua_upvalueindex(2));

	ctx->L = L;
	ctx->alloc_total = 0;
	ctx->alloc_count = 0;

	void* ud;
	ctx->alloc = lua_getallocf(L, &ctx->ud);

	lua_setallocf(L, lalloc, ctx);

	lua_sethook(L, lhook, LUA_MASKCALL | LUA_MASKRET, 0);

	int status = co_resume(L);

	lua_sethook(L, NULL, 0, 0);
	
	lua_setallocf(L, ctx->alloc, ctx->ud);

	return status;
}

static int
lstop(lua_State* L) {
	lua_CFunction co_resume = lua_tocfunction(L, lua_upvalueindex(1));
	lua_getglobal(L, "coroutine");
	lua_pushcfunction(L, co_resume);
	lua_setfield(L, -2, "resume");
	return 0;
}

int
lmem_profiler_start(lua_State* L) {
	struct profiler_context* ctx = malloc(sizeof(*ctx));
	memset(ctx, 0, sizeof(*ctx));

	lua_getglobal(L, "coroutine");
	lua_getfield(L, -1, "resume");

	lua_CFunction co_resume = lua_tocfunction(L, -1);
	if (co_resume == NULL)
		return luaL_error(L, "can't get coroutine.resume");
	lua_pop(L, 1);

	lua_pushcfunction(L, co_resume);
	lua_pushlightuserdata(L, ctx);
	lua_pushcclosure(L, lresume, 2);
	lua_setfield(L, -2, "resume");

	lua_pushcfunction(L, co_resume);
	lua_pushcclosure(L, lstop, 1);

	return 1;
}