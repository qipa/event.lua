#include <lua.h>
#include <lauxlib.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "list.h"
#include "pool.h"
#include "object_container.h"

#define MAX_LEVEL 8
#define COMMON_LEVEL 0

struct location {
	float x;
	float z;
};

struct object {
	struct list_node node;
	struct location cur;
	struct location des;
	int id;
	int uid;
	int level;
};

struct tile {
	struct list **headers;
	int x;
	int z;
};

struct aoi_context {
	int width;
	int heigh;
	int max_x_index;
	int max_z_index;
	int range;
	int tile_range;
	int tile_size;
	
	struct pool_ctx* pool;
	struct object_container* container;

	struct tile *tiles;
};

static inline struct tile*
tile_withrc(struct aoi_context* ctx,int r,int c) {
	if (c > ctx->max_x_index || r > ctx->max_z_index)
		return NULL;
	return &ctx->tiles[r * (ctx->max_x_index + 1) + c];
}

static inline void
tile_init(struct aoi_context* ctx) {
	ctx->tiles = malloc(ctx->tile_size * sizeof(struct tile));
	int x, z;
	for (x = 0; x <= ctx->max_x_index; x++) {
		for (z = 0; z <= ctx->max_z_index; z++) {
			struct tile* tl = tile_withrc(ctx, z, x);
			tl->x = x;
			tl->z = z;
			tl->headers = malloc(MAX_LEVEL * sizeof(struct list *));
			memset(tl->headers,0,MAX_LEVEL * sizeof(struct list *));
		}
	}
}

static inline struct tile*
tile_withpos(struct aoi_context* ctx,struct location *pos) {
	int x = pos->x / ctx->tile_range;
	int z = pos->z / ctx->tile_range;
	if (x > ctx->max_x_index || z > ctx->max_z_index)
		return NULL;
	return tile_withrc(ctx,z,x);
}

static inline struct list*
tile_level(struct tile* tl,int level) {
	if (tl->headers[level] == NULL) {
		tl->headers[level] = malloc(sizeof(struct list));
		memset(tl->headers[level],0,sizeof(struct list));
		list_init(tl->headers[level]);
	}		
	return tl->headers[level];
}

static inline void 
tile_push(struct tile* tl,int level,struct list_node *node) {
	struct list *l = tile_level(tl, level);
	list_push(l,node);
}

static inline void 
tile_pop(struct list_node *node) {
	list_remove(node);
}

static inline int 
calc_rect(struct aoi_context* ctx, struct location *pos, int range, struct location *bl, struct location *tr) {
	struct tile *tl = tile_withpos(ctx, pos);
	if (tl == NULL)	
		return -1;

	bl->x = tl->x - range;
	bl->z = tl->z - range;
	tr->x = tl->x + range;
	tr->z = tl->z + range;

	if (bl->x < 0)
		bl->x = 0;
	if (bl->z < 0)
		bl->z = 0;
	if (tr->x > ctx->max_x_index)
		tr->x = ctx->max_x_index;
	if (tr->z > ctx->max_z_index)
		tr->z = ctx->max_z_index;

	return 0;
}

void 
make_table(lua_State *L,struct list *list,struct object *self,int index,int* array_index) {
	struct object *obj = (struct object*) list->head.next;
	while (obj != (struct object*) &list->tail) {
		if (obj == self) {
			obj = (struct object*) obj->node.next;
			continue;
		}

		lua_pushinteger(L, obj->id);
		lua_rawseti(L, index-1, (*array_index)++);

		obj = (struct object*) obj->node.next;
	}
}

int 
aoi_enter(lua_State *L,struct aoi_context *ctx,struct object *obj) {
	struct tile *tl = tile_withpos(ctx,&obj->cur);
	if (tl == NULL)
		luaL_error(L,"[aoi_enter]invalid pos[%d:%d]",obj->cur.x,obj->cur.z);

	struct location bl = {0,0};
	struct location tr = {0,0};
	if (calc_rect(ctx,&obj->cur,ctx->range,&bl,&tr) < 0)
		luaL_error(L,"[aoi_enter]invalid pos[%d:%d],range[%d]",obj->cur.x,obj->cur.z,ctx->range);

	int array_index = 1;

	int x,z;
	for(z = bl.z;z <= tr.z;z++) {
		for(x = bl.x;x <= tr.x;x++) {
			struct tile *tl = tile_withrc(ctx,z,x);
			if (tl == NULL)
				return -1;

			struct list *list = tile_level(tl, COMMON_LEVEL);
			make_table(L,list,obj,-1,&array_index);

			if (obj->level != COMMON_LEVEL) {
				list = tile_level(tl, obj->level);
				make_table(L,list,obj,-1,&array_index);
			}
		}
	}
	tile_push(tl, obj->level, &obj->node);
	return 0;
}

int 
aoi_leave(lua_State *L,struct aoi_context* ctx,struct object *obj) {
	struct tile *tl = tile_withpos(ctx,&obj->cur);
	if (tl == NULL)
		luaL_error(L,"[aoi_leave]invalid pos[%d:%d]",obj->cur.x,obj->cur.z);


	struct location bl = {0,0};
	struct location tr = {0,0};
	if (calc_rect(ctx,&obj->cur,ctx->range,&bl,&tr) < 0)
		luaL_error(L,"[aoi_leave]invalid pos[%d:%d],range[%d]",obj->cur.x,obj->cur.z,ctx->range);

	int array_index = 1;
	int x,z;
	for(z = bl.z;z <= tr.z;z++) {
		for(x = bl.x;x <= tr.x;x++) {
			struct tile *tl = tile_withrc(ctx,z,x);
			if (tl == NULL)
				return -1;

			struct list *list = tile_level(tl, COMMON_LEVEL);
			make_table(L,list,obj,-1,&array_index);

			if (obj->level != COMMON_LEVEL) {
				list = tile_level(tl, obj->level);
				make_table(L,list,obj,-1,&array_index);
			}
		}
	}

	tile_pop(&obj->node);
	return 0;
}

int 
aoi_update(lua_State *L,struct aoi_context* ctx,struct object *obj,struct location *np) {
	struct location op = obj->cur;
	obj->cur = *np;

	struct tile *otl = tile_withpos(ctx,&op);
	struct tile *ntl = tile_withpos(ctx,&obj->cur);
	if (otl == ntl)
		return 0;

	tile_pop(&obj->node);
	tile_push(ntl, obj->level, &obj->node);
	
	struct location obl, otr;
	if (calc_rect(ctx, &op, ctx->range, &obl, &otr) < 0)
		return -1;

	struct location nbl, ntr;
	if (calc_rect(ctx, &obj->cur, ctx->range, &nbl, &ntr) < 0)
		return -1;

	int array_index = 1;

	int x, z;
	for (z = nbl.z; z <= ntr.z; z++) {
		for (x = nbl.x; x <= ntr.x; x++) {
			if (x >= obl.x && x <= otr.x && z >= obl.z && z <= otr.z)
				continue;

			struct tile *tl = tile_withrc(ctx,z,x);
			if (tl == NULL)
				return -1;

			struct list *list = tile_level(tl, COMMON_LEVEL);
			make_table(L,list,obj,-2,&array_index);

			if (obj->level != COMMON_LEVEL) {
				list = tile_level(tl, obj->level);
				make_table(L,list,obj,-2,&array_index);
			}
		}
	}

	array_index = 1;

	for (z = obl.z; z <= otr.z; z++) {
		for (x = obl.x; x <= otr.x; x++) {
			if (x >= nbl.x && x <= ntr.x && z >= nbl.z && z <= ntr.z)
				continue;

			struct tile *tl = tile_withrc(ctx,z,x);
			if (tl == NULL)
				return -1;

			struct list *list = tile_level(tl, COMMON_LEVEL);
			make_table(L,list,obj,-1,&array_index);

			if (obj->level != COMMON_LEVEL) {
				list = tile_level(tl, obj->level);
				make_table(L,list,obj,-1,&array_index);
			}
		}
	}
	return 0;
}

int 
laoi_new(lua_State *L) {
	int scene_width = luaL_checkinteger(L, 1);
	int scene_heigh = luaL_checkinteger(L, 2);
	int tile_range = luaL_optinteger(L, 3, 2);
	int range = luaL_optinteger(L, 4, 3);
	int max_object = luaL_optinteger(L, 5, 100);

	int max_x_index = scene_width / tile_range - 1;
	int max_z_index = scene_heigh / tile_range - 1;

	int width = (max_x_index + 1) * tile_range;
	int heigh = (max_z_index + 1) * tile_range;

	struct aoi_context* ctx = lua_newuserdata(L,sizeof(*ctx));
	memset(ctx,0,sizeof(*ctx));

	ctx->pool = pool_create(sizeof(struct object));
	ctx->container = container_create(max_object);

	ctx->width = width;
	ctx->heigh = heigh;
	ctx->max_x_index = max_x_index;
	ctx->max_z_index = max_z_index;
	ctx->tile_range = tile_range;   //tile length
	ctx->tile_size = (max_x_index + 1) * (max_z_index + 1);   //amount of tiles in map
	ctx->range = range;

	tile_init(ctx);

	luaL_newmetatable(L,"meta_fast_aoi");
 	lua_setmetatable(L, -2);
	return 1;
}

int 
laoi_release(lua_State* L) {
	struct aoi_context* aoi = lua_touserdata(L, 1);
	pool_release(aoi->pool);
	container_release(aoi->container);
	int x, z;
	for (x = 0; x <= aoi->max_x_index; x++) {
		for (z = 0; z <= aoi->max_z_index; z++) {
			struct tile* tl = tile_withrc(aoi, z, x);
			int i;
			for(i=0;i < MAX_LEVEL;i++) {
				if (tl->headers[i])
					free(tl->headers[i]);
			}
			free(tl->headers);
		}
	}

	free(aoi->tiles);
	return 0;
}

int 
laoi_enter(lua_State *L) {
	struct aoi_context* aoi = lua_touserdata(L, 1);
	int uid = luaL_checkinteger(L, 2);
	float x = luaL_checknumber(L, 3);
	float z = luaL_checknumber(L, 4);
	int level = luaL_optinteger(L,5,0);

	if (x < 0 || z < 0 || x >= aoi->width || z >= aoi->heigh)
		luaL_error(L,"[laoi_enter]invalid cur pos[%d:%d]",x,z);

	if (level < 0 || level >= MAX_LEVEL)
		luaL_error(L,"[laoi_enter]level error:%d",level);

	struct object* obj = pool_malloc(aoi->pool);
	memset(obj,0,sizeof(*obj));

	int id = container_add(aoi->container,obj);
	if (id < 0) {
		pool_free(aoi->pool,obj);
		luaL_error(L,"too many object");
	}

	obj->id = id;
	obj->uid = uid;
	obj->cur.x = x;
	obj->cur.z = z;
	obj->level = level;

	lua_pushinteger(L,obj->id);
	lua_newtable(L);

	aoi_enter(L,aoi,obj);
	return 2;
}

int
laoi_leave(lua_State *L) {
	struct aoi_context* aoi = lua_touserdata(L, 1);
	int id = lua_tointeger(L, 2);

	struct object* obj = container_get(aoi->container,id);
	if (!obj)
		luaL_error(L,"error aoi id:%d",id);

	lua_newtable(L); //-1 leave other
	aoi_leave(L,aoi,obj);

	container_remove(aoi->container,id);

	pool_free(aoi->pool,obj);

	return 1;
}

int 
laoi_update(lua_State *L) {
	struct aoi_context *aoi = lua_touserdata(L, 1);
	int id = lua_tointeger(L, 2);

	struct object* obj = container_get(aoi->container,id);

	struct location np;
	np.x = luaL_checknumber(L, 3);
	np.z = luaL_checknumber(L, 4);

	if (np.x < 0 || np.z < 0 || np.x >= aoi->width || np.z >= aoi->heigh)
		luaL_error(L,"[_aoi_update]invalid pos[%d:%d].",np.x,np.z);

	lua_newtable(L);
	lua_newtable(L);

	int ret = 0;

	if ((ret = aoi_update(L,aoi, obj, &np)) < 0)
		luaL_error(L,"[_aoi_update]erro:%d.",ret);
	return 2;
}

int 
laoi_viewlist(lua_State *L) {
	struct aoi_context *aoi = lua_touserdata(L, 1);
	int id = lua_tointeger(L, 2);

	struct object* obj = container_get(aoi->container,id);

	struct tile *tl = tile_withpos(aoi,&obj->cur);
	if (tl == NULL)
		luaL_error(L,"[_aoi_viewlist]invalid pos[%d:%d]",obj->cur.x,obj->cur.z);

	struct location bl = {0,0};
	struct location tr = {0,0};
	if (calc_rect(aoi,&obj->cur,aoi->range,&bl,&tr) < 0)
		luaL_error(L,"[_aoi_viewlist]invalid pos[%d:%d],range[%d]",obj->cur.x,obj->cur.z,aoi->range);

	lua_newtable(L);
	int array_index = 1;
	int x,z;
	for(z = bl.z;z <= tr.z;z++) {
		for(x = bl.x;x <= tr.x;x++) {
			struct tile *tl = tile_withrc(aoi,z,x);
			if (tl == NULL)
				luaL_error(L,"[_aoi_viewlist]invalid rc[%d:%d],range[%d]",z,x);

			struct list *list = tile_level(tl, COMMON_LEVEL);
			make_table(L,list,obj,-1,&array_index);

			if (obj->level != COMMON_LEVEL) {
				list = tile_level(tl, obj->level);
				make_table(L,list,obj,-1,&array_index);
			}
		}
	}

	return 1;
}

int 
luaopen_fastaoi_core(lua_State *L) {
	luaL_checkversion(L);

	luaL_newmetatable(L, "meta_fast_aoi");
	const luaL_Reg meta[] = {
		{ "enter", laoi_enter },
		{ "leave", laoi_leave },
		{ "update", laoi_update },
		{ "viewlist", laoi_viewlist },
		{ NULL, NULL },
	};
	luaL_newlib(L,meta);
	lua_setfield(L, -2, "__index");

	lua_pushcfunction(L,laoi_release);
	lua_setfield(L, -2, "__gc");
	lua_pop(L,1);


	luaL_Reg l[] = {
		{ "new", laoi_new},
		{ NULL, NULL },
	};

	lua_createtable(L, 0, (sizeof(l)) / sizeof(luaL_Reg) - 1);
	luaL_setfuncs(L, l, 0);
	return 1;
}
