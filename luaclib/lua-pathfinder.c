#include <lua.h>
#include <lauxlib.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "pathfinder.h"

struct patfinder_context {
	int scene;
	struct pathfinder* finder;
};


static int
_release(lua_State *L) {
	struct patfinder_context* ctx = (struct patfinder_context*)lua_touserdata(L, 1);
	finder_release(ctx->finder);
	return 0;
}

struct pathfinder_args {
	lua_State *L;
	int index;
};

void finder_callback(void *ud, int x, int z) {
	struct pathfinder_args* args = (struct pathfinder_args*)ud;
	lua_pushinteger(args->L,x);
	lua_rawseti(args->L,-2,++args->index);
	lua_pushinteger(args->L,z);
	lua_rawseti(args->L,-2,++args->index);
}

static int
_find(lua_State *L) {
	struct patfinder_context* ctx = (struct patfinder_context*)lua_touserdata(L, 1);
	int x0 = lua_tointeger(L,2);
	int z0 = lua_tointeger(L,3);
	int x1 = lua_tointeger(L,4);
	int z1 = lua_tointeger(L,5);

	struct pathfinder_args ud;
	ud.L = L;
	ud.index = 0;

	lua_newtable(L);

	int ok = finder_find(ctx->finder, x0, z0, x1, z1, finder_callback, &ud, NULL, NULL, 50);
	if (!ok) {
		lua_pop(L,1);
		return 0;
	}
	return 1;
}

static int
_raycast(lua_State *L) {
	struct patfinder_context* ctx = (struct patfinder_context*)lua_touserdata(L, 1);
	int x0 = lua_tointeger(L,2);
	int z0 = lua_tointeger(L,3);
	int x1 = lua_tointeger(L,4);
	int z1 = lua_tointeger(L,5);
	int ignore = lua_tointeger(L,6);

	int rx,rz;

	finder_raycast(ctx->finder, x0, z0, x1, z1, ignore, &rx, &rz, NULL, NULL);

	lua_pushinteger(L,rx);
	lua_pushinteger(L,rz);
	return 2;
}

static int
_movable(lua_State *L) {
	struct patfinder_context* ctx = (struct patfinder_context*)lua_touserdata(L, 1);
	int x = lua_tointeger(L,2);
	int z = lua_tointeger(L,3);
	int ignore = lua_tointeger(L,4);
	int ok = finder_movable(ctx->finder, x, z, ignore);
	lua_pushboolean(L,ok);
	return 1;
}

static int
_mask_set(lua_State *L) {
	struct patfinder_context* ctx = (struct patfinder_context*)lua_touserdata(L, 1);
	int index = lua_tointeger(L,2);
	int enable = lua_tointeger(L,3);
	finder_mask_set(ctx->finder, index, enable);
	return 0;
}

static int
_mask_reset(lua_State *L) {
	struct patfinder_context* ctx = (struct patfinder_context*)lua_touserdata(L, 1);
	finder_mask_reset(ctx->finder);
	return 0;
}

static int
create(lua_State *L) {
	int scene = lua_tointeger(L,1);
	const char* file = lua_tostring(L,2);

	int width;
	int heigh;

	char* data = NULL;

	FILE* ptr = fopen(file,"r");
	if (!ptr)
		luaL_error(L,"no such file:%s",file);

	fseek(ptr,0,SEEK_END);
	int size = ftell(ptr);
	rewind(ptr);
	fread(&width,1,sizeof(int),ptr);
	fread(&heigh,1,sizeof(int),ptr);
	data = malloc(size - sizeof(int)*2);
	memset(data,0,size - sizeof(int)*2);
	fread(data,1,size - sizeof(int)*2,ptr);
	fclose(ptr);

	struct patfinder_context* ctx = (struct patfinder_context*)lua_newuserdata(L, sizeof(struct patfinder_context));
	ctx->scene = scene;
	ctx->finder = finder_create(width,heigh,data);
	free(data);

	lua_newtable(L);

	lua_pushcfunction(L, _release);
	lua_setfield(L, -2, "__gc");

	luaL_Reg l[] = {
		{ "find", _find },
		{ "raycast", _raycast },
		{ "movable", _movable },
		{ "mask_set", _mask_set },
		{ "mask_reset", _mask_reset },
		{ NULL, NULL },
	};
	luaL_newlib(L,l);

	lua_setfield(L, -2, "__index");

	lua_setmetatable(L, -2);

	return 1;
}

int 
luaopen_pathfinder_core(lua_State *L) {
	luaL_checkversion(L);
	luaL_Reg l[] = {
		{ "create", create},
		{ NULL, NULL },
	};

	lua_createtable(L, 0, (sizeof(l)) / sizeof(luaL_Reg) - 1);
	luaL_setfuncs(L, l, 0);
	return 1;
}
