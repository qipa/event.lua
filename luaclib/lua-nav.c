

#include <math.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include <lua.h>
#include <lauxlib.h>

#include "nav.h"


struct scene_context {
	struct nav_mesh_context* ctx;
	int scene;
};

static int 
meta_nav_info(struct lua_State* L) {
	struct scene_context* scene_ctx = (struct scene_context*)lua_touserdata(L, 1);
	struct nav_mesh_context* ctx = scene_ctx->ctx;
	lua_newtable(L);

	lua_newtable(L);
	lua_pushnumber(L,ctx->lt.x);
	lua_setfield(L,-2,"x");
	lua_pushnumber(L,ctx->lt.z);
	lua_setfield(L,-2,"z");
	lua_setfield(L,-2,"lt");

	lua_newtable(L);
	lua_pushnumber(L,ctx->br.x);
	lua_setfield(L,-2,"x");
	lua_pushnumber(L,ctx->br.z);
	lua_setfield(L,-2,"z");
	lua_setfield(L,-2,"br");

	lua_pushinteger(L,ctx->width);
	lua_setfield(L,-2,"width");

	lua_pushinteger(L,ctx->heigh);
	lua_setfield(L,-2,"heigh");

	lua_newtable(L);
	int i;
	for(i=0;i<ctx->len;i++) {
		lua_newtable(L);
		lua_pushnumber(L,ctx->vertices[i].x);
		lua_setfield(L,-2,"x");
		lua_pushnumber(L,ctx->vertices[i].z);
		lua_setfield(L,-2,"z");
		lua_rawseti(L,-2,i+1);
	}
	lua_setfield(L,-2,"vertices");

	lua_newtable(L);
	for(i=0;i<ctx->size;i++) {
		lua_newtable(L);
		
		struct nav_node* node = &ctx->node[i];
		lua_pushinteger(L,node->id);
		lua_setfield(L,-2,"id");

		lua_newtable(L);
		lua_pushnumber(L,node->center.x);
		lua_setfield(L,-2,"x");
		lua_pushnumber(L,node->center.z);
		lua_setfield(L,-2,"z");
		lua_setfield(L,-2,"center");

		lua_pushinteger(L,node->mask);
		lua_setfield(L,-2,"mask");

		lua_newtable(L);
		int j;
		for(j=0;j<node->size;j++) {
			lua_pushinteger(L,node->poly[j]);
			lua_rawseti(L,-2,j+1);
		}
		lua_setfield(L,-2,"poly");

		lua_newtable(L);
		for(j=0;j<node->size;j++) {
			lua_pushinteger(L,node->border[j]);
			lua_rawseti(L,-2,j+1);
		}
		lua_setfield(L,-2,"border");

		lua_rawseti(L,-2,i+1);
	}
	lua_setfield(L,-2,"node");

	lua_newtable(L);
	for(i=0;i<ctx->border_ctx.border_offset;i++) {
		struct nav_border* border = &ctx->border_ctx.borders[i];
		lua_newtable(L);

		lua_pushinteger(L,i);
		lua_setfield(L,-2,"id");

		lua_pushinteger(L,border->a);
		lua_setfield(L,-2,"a");

		lua_pushinteger(L,border->b);
		lua_setfield(L,-2,"b");

		lua_pushinteger(L,border->opposite);
		lua_setfield(L,-2,"opposite");

		lua_newtable(L);
		lua_pushinteger(L,border->node[0]);
		lua_rawseti(L,-2,1);
		lua_pushinteger(L,border->node[1]);
		lua_rawseti(L,-2,2);
		lua_setfield(L,-2,"node");

		lua_newtable(L);
		lua_pushnumber(L,border->center.x);
		lua_setfield(L,-2,"x");
		lua_pushnumber(L,border->center.z);
		lua_setfield(L,-2,"z");
		lua_setfield(L,-2,"center");

		lua_rawseti(L,-2,i+1);
	}
	lua_setfield(L,-2,"border");

	return 1;
}

static int
meta_tile_info(struct lua_State* L) {
	struct scene_context* scene_ctx = (struct scene_context*)lua_touserdata(L, 1);
	struct nav_mesh_context* ctx = scene_ctx->ctx;
	if (ctx->tile == NULL) {
		return 0;
	}

	lua_newtable(L);
	int i;
	for(i=0;i<ctx->width*ctx->heigh;i++) {
		struct nav_tile* tile = &ctx->tile[i];

		lua_newtable(L);
		int j;
		for(j=0;j<tile->offset;j++) {
			lua_pushinteger(L,tile->node[j]);
			lua_rawseti(L,-2,j+1);
		}

		lua_rawseti(L,-2,i+1);
	}
	return 1;
}

static int
meta_create_tile(struct lua_State* L) {
	struct scene_context* scene_ctx = (struct scene_context*)lua_touserdata(L, 1);
	struct nav_mesh_context* ctx = scene_ctx->ctx;
	ctx->tile = create_tile(ctx);
	return 0;
}

static int
meta_load_tile(struct lua_State* L) {
	struct scene_context* scene_ctx = (struct scene_context*)lua_touserdata(L, 1);
	struct nav_mesh_context* ctx = scene_ctx->ctx;

	luaL_checktype(L,2,LUA_TTABLE);

	int len = lua_rawlen(L,-1);

	ctx->tile = malloc(sizeof(struct nav_tile)*len);
	memset(ctx->tile,0,sizeof(struct nav_tile)*len);
	int i;
	for(i=0;i<len;i++) {
		lua_rawgeti(L,-1,i+1);
		struct nav_tile* tile = &ctx->tile[i];
		size_t size = lua_rawlen(L,-1);
		tile->offset = tile->size = size;
		tile->node = NULL;
		if (tile->offset != 0) {
			tile->node = malloc(sizeof(int)*tile->offset);
			int j;
			for(j=0;j<size;j++) {
				lua_rawgeti(L,-1,j+1);
				tile->node[j] = lua_tointeger(L,-1);
				lua_pop(L,1);
			}
		}
		lua_pop(L,1);
	}

	return 0;
}

static int 
meta_release(struct lua_State* L) {
	struct scene_context* scene_ctx = (struct scene_context*)lua_touserdata(L, 1);
	struct nav_mesh_context* ctx = scene_ctx->ctx;
	release_mesh(ctx);
	return 0;
}

static int 
meta_set_mask(struct lua_State* L) {
	struct scene_context* scene_ctx = (struct scene_context*)lua_touserdata(L, 1);
	struct nav_mesh_context* ctx = scene_ctx->ctx;
	int mask = lua_tointeger(L, 2);
	int enable = lua_tointeger(L, 3);
	set_mask(&ctx->mask_ctx, mask, enable);
	return 0;
}

static int
meta_get_mask(struct lua_State* L) {
	struct scene_context* scene_ctx = (struct scene_context*)lua_touserdata(L, 1);
	struct nav_mesh_context* ctx = scene_ctx->ctx;
	int index = lua_tointeger(L, 2);
	int mask = get_mask(ctx->mask_ctx,index);
	lua_pushinteger(L,mask);
	return 1;
}

static int 
meta_find(struct lua_State* L) {
	struct scene_context* scene_ctx = (struct scene_context*)lua_touserdata(L, 1);
	struct nav_mesh_context* ctx = scene_ctx->ctx;

	struct vector3 start, over;
	start.x = lua_tonumber(L, 2);
	start.z = lua_tonumber(L, 3);
	over.x = lua_tonumber(L, 4);
	over.z = lua_tonumber(L, 5);

	struct nav_path* path = astar_find(ctx, &start, &over, NULL, NULL);
	if (!path)
		return 0;

	lua_createtable(L, path->offset, 0);
	int i;
	for (i = 0; i < path->offset; i++) {
		lua_pushinteger(L, i + 1);
		lua_createtable(L, 0, 2);
		lua_pushnumber(L, path->wp[i].x);
		lua_setfield(L, -2, "x");
		lua_pushnumber(L, path->wp[i].z);
		lua_setfield(L, -2, "z");
		lua_settable(L, -3);
	}

	return 1;
}

static int 
meta_raycast(struct lua_State* L) {
	struct scene_context* scene_ctx = (struct scene_context*)lua_touserdata(L, 1);
	struct nav_mesh_context* ctx = scene_ctx->ctx;

	struct vector3 start, over, result;
	start.x = lua_tonumber(L, 2);
	start.z = lua_tonumber(L, 3);
	over.x = lua_tonumber(L, 4);
	over.z = lua_tonumber(L, 5);

	bool ok = raycast(ctx, &start, &over, &result,NULL,NULL);
	if (!ok) {
		return 0;
	}

	lua_pushnumber(L, result.x);
	lua_pushnumber(L, result.z);
	return 2;
}

static int
meta_around_movable(struct lua_State* L) {
	struct scene_context* scene_ctx = (struct scene_context*)lua_touserdata(L, 1);
	struct nav_mesh_context* ctx = scene_ctx->ctx;

	double x = lua_tonumber(L, 2);
	double z = lua_tonumber(L, 3);
	int r = lua_tointeger(L, 4);

	struct vector3* pos = around_movable(ctx, x, z, r, NULL, NULL);
	if (pos == NULL)
		return 0;
	lua_pushnumber(L,pos->x);
	lua_pushnumber(L,pos->z);
	return 2;
}

static int
meta_movable(struct lua_State* L) {
	struct scene_context* scene_ctx = (struct scene_context*)lua_touserdata(L, 1);
	struct nav_mesh_context* ctx = scene_ctx->ctx;
	double x = lua_tonumber(L, 2);
	double z = lua_tonumber(L, 3);

	int val = point_movable(ctx,x,z);
	lua_pushboolean(L,val);
	return 1;
}

int
init_meta(struct lua_State* L,int scene,struct nav_mesh_context* ctx) {
	struct scene_context* scene_ctx = (struct scene_context*)lua_newuserdata(L, sizeof(struct scene_context));
	scene_ctx->ctx = ctx;
	scene_ctx->scene = scene;
	
	lua_newtable(L);

	lua_pushcfunction(L, meta_release);
	lua_setfield(L, -2, "__gc");

	luaL_Reg l[] = {
		{ "nav_info", meta_nav_info },
		{ "tile_info", meta_tile_info },
		{ "create_tile", meta_create_tile },
		{ "load_tile", meta_load_tile },
		{ "find", meta_find },
		{ "raycast", meta_raycast },
		{ "set_mask", meta_set_mask },
		{ "get_mask", meta_get_mask },
		{ "movable", meta_movable },
		{ "around_movable", meta_around_movable },
		{ NULL, NULL },
	};
	luaL_newlib(L,l);

	lua_setfield(L, -2, "__index");

	lua_setmetatable(L, -2);
	return 1;
}

int 
_create_nav(struct lua_State* L) {
	int scene = lua_tointeger(L, 1);
	
	lua_getfield(L,2,"v");
	size_t vsize = lua_rawlen(L,-1);
	double** vptr = malloc(sizeof(*vptr)*vsize);
	int i;
	for(i=0;i<vsize;i++) {
		vptr[i] = malloc(sizeof(double)*3);
		lua_rawgeti(L,-1,i+1);
		int j;
		for(j=1;j<=3;j++) {
			lua_rawgeti(L,-1,j);
			vptr[i][j-1] = lua_tonumber(L,-1);
			lua_pop(L,1);
		}
		lua_pop(L,1);
	}
	lua_pop(L,1);

	lua_getfield(L,2,"p");
	size_t psize = lua_rawlen(L,-1);
	int** pptr = malloc(sizeof(*pptr)*psize);
	for(i=0;i<psize;i++) {
		lua_rawgeti(L,-1,i+1);
		size_t size = lua_rawlen(L,-1);
		pptr[i] = malloc(sizeof(int)*size);
		
		int j;
		for(j=1;j<=size;j++) {
			lua_rawgeti(L,-1,j);
			pptr[i][j-1] = lua_tointeger(L,-1);
			lua_pop(L,1);
		}
		lua_pop(L,1);
	}

	struct nav_mesh_context* ctx = load_mesh(vptr,vsize,pptr,psize);
	
	for (i = 0; i < vsize; i++) {
		free(vptr[i]);
	}
	free(vptr);
	for (i = 0; i < psize; i++) {
		free(pptr[i]);
	}
	free(pptr);

	return init_meta(L,scene,ctx);
}

int 
_load_nav(struct lua_State* L) {
	int scene = lua_tointeger(L, 1);

	struct nav_mesh_context* ctx = malloc(sizeof(*ctx));

	lua_getfield(L,2,"lt");
	lua_getfield(L,-1,"x");
	ctx->lt.x = lua_tonumber(L,-1);
	lua_pop(L,1);
	lua_getfield(L,-1,"z");
	ctx->lt.z = lua_tonumber(L,-1);
	lua_pop(L,1);
	lua_pop(L,1);

	lua_getfield(L,2,"br");
	lua_getfield(L,-1,"x");
	ctx->br.x = lua_tonumber(L,-1);
	lua_pop(L,1);
	lua_getfield(L,-1,"z");
	ctx->br.z = lua_tonumber(L,-1);
	lua_pop(L,1);
	lua_pop(L,1);

	lua_getfield(L,2,"width");
	ctx->width = lua_tointeger(L,-1);
	lua_pop(L,1);

	lua_getfield(L,2,"heigh");
	ctx->heigh = lua_tointeger(L,-1);
	lua_pop(L,1);

	lua_getfield(L,2,"vertices");
	size_t len = lua_rawlen(L,-1);
	ctx->len = len;
	ctx->vertices = malloc(sizeof(struct vector3) * len);

	int i;
	for(i=0;i<len;i++) {
		lua_rawgeti(L,-1,i+1);
		lua_getfield(L,-1,"x");
		ctx->vertices[i].x = lua_tonumber(L,-1);
		lua_pop(L,1);
		lua_getfield(L,-1,"z");
		ctx->vertices[i].z = lua_tonumber(L,-1);
		lua_pop(L,1);

		lua_pop(L,1);
	}
	lua_pop(L,1);
	
	lua_getfield(L,2,"border");
	len = lua_rawlen(L,-1);

	ctx->border_ctx.border_cap = ctx->border_ctx.border_offset = len;
	ctx->border_ctx.borders = malloc(sizeof(struct nav_border)*len);
	memset(ctx->border_ctx.borders,0,sizeof(struct nav_border)*len);
	for(i=0;i<len;i++) {
		lua_rawgeti(L,-1,i+1);

		struct nav_border* border = &ctx->border_ctx.borders[i];

		lua_getfield(L,-1,"id");
		border->id = lua_tointeger(L,-1);
		lua_pop(L,1);

		lua_getfield(L,-1,"opposite");
		border->opposite = lua_tointeger(L,-1);
		lua_pop(L,1);

		lua_getfield(L,-1,"a");
		border->a = lua_tointeger(L,-1);
		lua_pop(L,1);

		lua_getfield(L,-1,"b");
		border->b = lua_tointeger(L,-1);
		lua_pop(L,1);

		lua_getfield(L,-1,"node");
		lua_rawgeti(L,-1,1);
		border->node[0] = lua_tointeger(L,-1);
		lua_pop(L,1);
		lua_rawgeti(L,-1,2);
		border->node[1] = lua_tointeger(L,-1);
		lua_pop(L,1);
		lua_pop(L,1);

		lua_getfield(L,-1,"center");
		lua_getfield(L,-1,"x");
		border->center.x = lua_tonumber(L,-1);
		lua_pop(L,1);
		lua_getfield(L,-1,"z");
		border->center.z = lua_tonumber(L,-1);
		lua_pop(L,1);
		lua_pop(L,1);

		lua_pop(L,1);
	}
	lua_pop(L,1);
	

	lua_getfield(L,2,"node");
	len = lua_rawlen(L,-1);
	ctx->size = len;
	ctx->node = malloc(sizeof(struct nav_node)*ctx->size);
	memset(ctx->node,0,sizeof(struct nav_node)*ctx->size);
	for(i=0;i<len;i++) {
		lua_rawgeti(L,-1,i+1);
		struct nav_node* node = &ctx->node[i];
		node->id = i;
		node->link_border = -1;
		node->link_parent = NULL;
		
		lua_getfield(L,-1,"mask");
		node->mask = lua_tointeger(L,-1);
		lua_pop(L,1);

		lua_getfield(L,-1,"center");
		lua_getfield(L,-1,"x");
		node->center.x = lua_tonumber(L,-1);
		lua_pop(L,1);
		lua_getfield(L,-1,"z");
		node->center.z = lua_tonumber(L,-1);
		lua_pop(L,1);
		lua_pop(L,1);

		lua_getfield(L,-1,"poly");
		size_t size = lua_rawlen(L,-1);

		node->poly = malloc(sizeof(int)*size);
		node->border = malloc(sizeof(int)*size);
		node->size = size;

		int j;
		for(j=0;j<size;j++) {
			lua_rawgeti(L,-1,j+1);
			node->poly[j] = lua_tointeger(L,-1);
			lua_pop(L,1);
		}
		lua_pop(L,1);

		lua_getfield(L,-1,"border");
		for(j=0;j<size;j++) {
			lua_rawgeti(L,-1,j+1);
			node->border[j] = lua_tointeger(L,-1);
			lua_pop(L,1);
		}
		lua_pop(L,1);
		
		lua_pop(L,1);
	}
	lua_pop(L,1);

	ctx->tile = NULL;
	
	load_mesh_done(ctx);

	return init_meta(L,scene,ctx);
}


int 
luaopen_nav_core(lua_State *L) {
	luaL_Reg l[] =  {
		{"create", _create_nav},
		{"load", _load_nav},
		{ NULL, NULL }
	};

	lua_createtable(L, 0, (sizeof(l)) / sizeof(luaL_Reg) - 1);
	luaL_setfuncs(L, l, 0);
	return 1;
}
