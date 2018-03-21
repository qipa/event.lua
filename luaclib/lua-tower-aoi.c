#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <stdint.h>

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#include "khash.h"


#define TYPE_OBJECT 	0
#define TYPE_WATCHER  	1

typedef struct location {
	double x;
	double z;
} location_t;

typedef struct object {
	int uid;
	int id;
	int type;

	union {
		int range;
		struct {
			struct object* link_next; 
			struct object* link_prev; 
		} link_node;
	} param;

	location_t local;
	struct object* next;
	struct object* prev;
} object_t;

typedef struct region {
	int32_t begin_x;
	int32_t begin_z;
	int32_t end_x;
	int32_t end_z;
} region_t;

KHASH_MAP_INIT_INT(watcher,object_t*);

typedef struct tower {
	struct object* head;
	struct object* tail;
	khash_t(watcher)* hash;
} tower_t;

typedef struct aoi {
	uint32_t width;
	uint32_t height;

	uint32_t range;
	
	uint32_t tower_x;
	uint32_t tower_z;

	object_t* objs;
	size_t max;

	object_t* freelist;

	tower_t** towers;
} aoi_t;


static inline void
translate(aoi_t* aoi,location_t* in,location_t* out) {
	out->x = in->x / aoi->range;
	out->z = in->z / aoi->range;
}

static inline void
get_region(aoi_t* aoi,location_t* local,region_t* out,uint32_t range) {
	if (local->x - range < 0) {
		out->begin_x = 0;
		out->end_x = 2 * range;
		if (out->end_x >= aoi->tower_x) {
			out->end_x = aoi->tower_x - 1;
		}
	} else if (local->x + range >= aoi->tower_x) {
		out->end_x = aoi->tower_x - 1;
		out->begin_x = out->end_x - 2 * range;
		if (out->begin_x < 0) {
			out->begin_x = 0;
		}
	} else {
		out->begin_x = local->x - range;
		out->end_x = local->x + range;
	}

	if (local->z - range < 0) {
		out->begin_z = 0;
		out->end_z = 2 * range;
		if (out->end_z >= aoi->tower_z) {
			out->end_z = aoi->tower_z - 1;
		}
	} else if (local->z + range >= aoi->tower_z) {
		out->end_z = aoi->tower_z - 1;
		out->begin_z = out->end_z - 2 * range;
	} else {
		out->begin_z = local->z - range;
		out->end_z = local->z + range;
		if (out->begin_z < 0) {
			out->begin_z = 0;
		}
	}
}

static inline object_t* 
new_object(lua_State* L,aoi_t* aoi,int uid,int type,double x,double z) {
	if (aoi->freelist == NULL) {
		luaL_error(L,"new object error:object count limited:%d",aoi->max);
	}
	object_t* obj = aoi->freelist;
	aoi->freelist = obj->next;
	obj->next = obj->prev = NULL;
	obj->uid = uid;
	obj->type = type;
	obj->local.x = x;
	obj->local.z = z;
	return obj;
}

static inline void
free_object(aoi_t* aoi,object_t* obj) {
	obj->next = aoi->freelist;
	aoi->freelist = obj;
}

static inline void
link_object(tower_t* tower,object_t* object) {
	if (tower->head == NULL) {
		tower->head = tower->tail = object;
	} else {
		tower->tail->next = object;
		object->param.link_node.link_prev = tower->tail;
		object->param.link_node.link_next = NULL;
		tower->tail = object;
	}
}

static inline void
unlink_object(tower_t* tower,object_t* object) {
	if (object->param.link_node.link_prev != NULL) {
		object->param.link_node.link_prev->next = object->param.link_node.link_next;
		object->param.link_node.link_prev = NULL;
	} else {
		tower->head = object->param.link_node.link_next;
	}
	if (object->param.link_node.link_next != NULL) {
		object->param.link_node.link_next->prev = object->param.link_node.link_prev;
		object->param.link_node.link_next = NULL;
	} else {
		tower->tail = object->param.link_node.link_prev;
	}
}

static int
_add_object(lua_State* L) {
	aoi_t* aoi = (aoi_t*)lua_touserdata(L, 1);

	int uid = lua_tointeger(L,2);
	double x = lua_tonumber(L,3);
	double z = lua_tonumber(L,4);

	if (x >= aoi->width || z >= aoi->height) {
		luaL_error(L,"add object error:invalid local:[%f,%f]",x,z);
	}

	object_t* obj = new_object(L,aoi,uid,TYPE_OBJECT,x,z);
	obj->param.link_node.link_prev = obj->param.link_node.link_next = NULL;

	location_t out;
	translate(aoi,&obj->local,&out);

	tower_t* tower = &aoi->towers[(uint32_t)out.x][(uint32_t)out.z];

	link_object(tower,obj);

	lua_pushinteger(L,obj->id);
	lua_newtable(L);
	int i = 1;

	khiter_t k;
	for(k = kh_begin(tower->hash);k != kh_end(tower->hash);++k) {
		if (kh_exist(tower->hash,k)) {
			object_t* obj = kh_value(tower->hash,k);
			lua_pushinteger(L,obj->uid);
			lua_rawseti(L,-2,i++);
		}
	}

	return 2;
}

static int
_remove_object(lua_State* L) {
	aoi_t* aoi = (aoi_t*)lua_touserdata(L, 1);
	int id = lua_tointeger(L,2);
	if (id >= aoi->max) {
		luaL_error(L,"error id:%d",id);
	}
	object_t* obj = &aoi->objs[id];
	assert(obj->type == TYPE_OBJECT);

	location_t out;
	translate(aoi,&obj->local,&out);

	tower_t* tower = &aoi->towers[(uint32_t)out.x][(uint32_t)out.z];

	unlink_object(tower,obj);

	free_object(aoi,obj);

	lua_newtable(L);
	int i = 1;

	khiter_t k;
	for(k = kh_begin(tower->hash);k != kh_end(tower->hash);++k) {
		if (kh_exist(tower->hash,k)) {
			object_t* obj = kh_value(tower->hash,k);
			lua_pushinteger(L,obj->uid);
			lua_rawseti(L,-2,i++);
		}
	}

	return 1;
}

static int
_update_object(lua_State* L) {
	aoi_t* aoi = (aoi_t*)lua_touserdata(L, 1);
	int id = lua_tointeger(L,2);
	double nx = lua_tonumber(L,3);
	double nz = lua_tonumber(L,4);

	if (nx >= aoi->width || nz >= aoi->height) {
		luaL_error(L,"update object error:invalid local:[%f,%f]",nx,nz);
	}

	object_t* obj = &aoi->objs[id];
	assert(obj->type == TYPE_OBJECT);

	location_t out;
	translate(aoi,&obj->local,&out);
	tower_t* otower = &aoi->towers[(uint32_t)out.x][(uint32_t)out.z];

	obj->local.x = nx;
	obj->local.z = nz;
	translate(aoi,&obj->local,&out);
	tower_t* ntower = &aoi->towers[(uint32_t)out.x][(uint32_t)out.z];

	if (ntower == otower) {
		return 0;
	}

	unlink_object(otower,obj);

	link_object(ntower,obj);

	object_t* leave = NULL;
	object_t* enter = NULL;
	khiter_t k;
	for(k = kh_begin(otower->hash);k != kh_end(otower->hash);++k) {
		if (kh_exist(otower->hash,k)) {
			object_t* obj = kh_value(otower->hash,k);
			if (leave == NULL) {
				leave = obj;
				obj->next = NULL;
				obj->prev = NULL;
			} else {
				obj->prev = NULL;
				obj->next = leave;
				leave = obj;
			}
		}
	}

	for(k = kh_begin(ntower->hash);k != kh_end(ntower->hash);++k) {
		if (kh_exist(ntower->hash,k)) {
			object_t* obj = kh_value(ntower->hash,k);
			if (obj->next != NULL || obj->prev != NULL || obj == leave) {
				if (obj->prev != NULL) {
					obj->prev->next = obj->next;
				}
				if (obj->next != NULL) {
					obj->next->prev = obj->prev;
				}
				if (obj == leave) {
					leave = obj->next;
				}
				obj->next = obj->prev = NULL;
				
			} else {
				if (enter == NULL) {
					enter = obj;
					obj->next = NULL;
					obj->prev = NULL;
				} else {
					enter->next = enter;
					obj->prev = enter;
					obj->next = NULL;
					enter = obj;
				}
			}
		}
	}

	lua_newtable(L);
	int i = 1;
	object_t* cursor = leave;
	while(cursor != NULL) {
		object_t* obj = cursor;
		cursor = cursor->next;
		lua_pushinteger(L,obj->uid);
		lua_rawseti(L,-2,i++);
		obj->next = obj->prev = NULL;
	}

	lua_newtable(L);
	i = 1;
	cursor = enter;
	while(cursor != NULL) {
		object_t* obj = cursor;
		cursor = cursor->next;
		lua_pushinteger(L,obj->uid);
		lua_rawseti(L,-2,i++);
		obj->next = obj->prev = NULL;
	}

	return 2;
}

static int
_add_watcher(lua_State* L) {
	aoi_t* aoi = (aoi_t*)lua_touserdata(L, 1);
	int uid = lua_tointeger(L,2);
	double x = lua_tonumber(L,3);
	double z = lua_tonumber(L,4);
	int range = lua_tointeger(L,5);

	object_t* obj = new_object(L,aoi,uid,TYPE_WATCHER,x,z);
	obj->param.range = range;

	location_t out;
	translate(aoi,&obj->local,&out);

	region_t region;
	get_region(aoi,&out,&region,range);

	lua_pushinteger(L,obj->id);
	lua_newtable(L);
	int i = 1;

	uint32_t x_index;
	for(x_index = region.begin_x;x_index <= region.end_x;x_index++) {
		uint32_t z_index;
		for(z_index = region.begin_z;z_index <= region.end_z;z_index++) {
			tower_t* tower = &aoi->towers[x_index][z_index];
			int ok;
			khiter_t k = kh_put(watcher, tower->hash, obj->uid, &ok);
			assert(ok == 1 || ok == 2);
			kh_value(tower->hash, k) = obj;

			struct object* cursor = tower->head;
			while(cursor) {
				lua_pushinteger(L,cursor->uid);
				lua_rawseti(L,-2,i++);
				cursor = cursor->param.link_node.link_next;
			}
		}
	}
	return 2;
}

static int
_remove_watcher(lua_State* L) {
	aoi_t* aoi = (aoi_t*)lua_touserdata(L, 1);
	int uid = lua_tointeger(L,2);

	object_t* obj = &aoi->objs[uid];
	assert(obj->type == TYPE_WATCHER);

	location_t out;
	translate(aoi,&obj->local,&out);

	region_t region;
	get_region(aoi,&out,&region,obj->param.range);

	uint32_t x_index;
	for(x_index = region.begin_x;x_index <= region.end_x;x_index++) {
		uint32_t z_index;
		for(z_index = region.begin_z;z_index <= region.end_z;z_index++) {
			tower_t* tower = &aoi->towers[x_index][z_index];

			khiter_t k = kh_get(watcher, tower->hash, obj->uid);
			assert(k != kh_end(tower->hash));
			kh_del(watcher,tower->hash,k);
		}
	}

	free_object(aoi,obj);
	return 0;
}

static int
_update_watcher(lua_State* L) {
	aoi_t* aoi = (aoi_t*)lua_touserdata(L, 1);
	int id = lua_tointeger(L,2);
	double nx = lua_tonumber(L,3);
	double nz = lua_tonumber(L,4);

	object_t* obj = &aoi->objs[id];
	assert(obj->type == TYPE_WATCHER);

	location_t oout;
	translate(aoi,&obj->local,&oout);

	location_t nout;
	obj->local.x = nx;
	obj->local.z = nz;
	translate(aoi,&obj->local,&nout);

	if (oout.x == nout.x && oout.z == nout.z) {
		return 0;
	}

	region_t oregion;
	get_region(aoi,&oout,&oregion,obj->param.range);

	region_t nregion;
	get_region(aoi,&nout,&nregion,obj->param.range);

	lua_newtable(L);
	int i = 1;

	uint32_t x_index;
	for(x_index = oregion.begin_x;x_index <= oregion.end_x;x_index++) {
		uint32_t z_index;
		for(z_index = oregion.begin_z;z_index <= oregion.end_z;z_index++) {
			if (x_index >= nregion.begin_x && x_index <= nregion.end_x && z_index >= nregion.begin_z && z_index <= nregion.end_z) {
				continue;
			}
			tower_t* tower = &aoi->towers[x_index][z_index];
			khiter_t k = kh_get(watcher, tower->hash, obj->uid);
			assert(k != kh_end(tower->hash));
			kh_del(watcher,tower->hash,k);

			struct object* cursor = tower->head;
			while(cursor) {
				lua_pushinteger(L,cursor->uid);
				lua_rawseti(L,-2,i++);
				cursor = cursor->param.link_node.link_next;
			}
		}
	}

	lua_newtable(L);
	i = 1;
	for(x_index = nregion.begin_x;x_index <= nregion.end_x;x_index++) {
		uint32_t z_index;
		for(z_index = nregion.begin_z;z_index <= nregion.end_z;z_index++) {
			if (x_index >= oregion.begin_x && x_index <= oregion.end_x && z_index >= oregion.begin_z && z_index <= oregion.end_z) {
				continue;
			}
			tower_t* tower = &aoi->towers[x_index][z_index];

			int ok;
			khiter_t k = kh_put(watcher, tower->hash, obj->uid, &ok);
			assert(k != kh_end(tower->hash));
			kh_value(tower->hash, k) = obj;

			struct object* cursor = tower->head;
			while(cursor) {
				lua_pushinteger(L,cursor->uid);
				lua_rawseti(L,-2,i++);
				cursor = cursor->param.link_node.link_next;
			}
		}
	}

	return 2;
}

static int
_get_viewers(lua_State* L) {
	aoi_t* aoi = (aoi_t*)lua_touserdata(L, 1);
	int id = lua_tointeger(L,2);
	object_t* obj = &aoi->objs[id];

	if (obj->type != TYPE_OBJECT) {
		luaL_error(L,"get viewers error,object type must be object");
	}

	location_t out;
	translate(aoi,&obj->local,&out);

	tower_t* tower = &aoi->towers[(uint32_t)out.x][(uint32_t)out.z];

	lua_pushinteger(L,obj->id);
	lua_newtable(L);
	int i = 1;

	khiter_t k;
	for(k = kh_begin(tower->hash);k != kh_end(tower->hash);++k) {
		if (kh_exist(tower->hash,k)) {
			object_t* obj = kh_value(tower->hash,k);
			lua_pushinteger(L,obj->uid);
			lua_rawseti(L,-2,i++);
		}
	}

	return 1;
}

static int
_get_visible(lua_State* L) {
	aoi_t* aoi = (aoi_t*)lua_touserdata(L, 1);
	int id = lua_tointeger(L,2);
	object_t* obj = &aoi->objs[id];

	if (obj->type != TYPE_WATCHER) {
		luaL_error(L,"get visiable error,object type must be watcher");
	}

	location_t out;
	translate(aoi,&obj->local,&out);

	region_t region;
	get_region(aoi,&out,&region,obj->param.range);

	lua_newtable(L);
	int i = 1;

	uint32_t x_index;
	for(x_index = region.begin_x;x_index <= region.end_x;x_index++) {
		uint32_t z_index;
		for(z_index = region.begin_z;z_index <= region.end_z;z_index++) {
			tower_t* tower = &aoi->towers[x_index][z_index];
			struct object* cursor = tower->head;
			while(cursor) {
				lua_pushinteger(L,cursor->uid);
				lua_rawseti(L,-2,i++);
				cursor = cursor->param.link_node.link_next;
			}
		}
	}
	return 1;

}

static int
_release(lua_State* L) {
	aoi_t* aoi = (aoi_t*)lua_touserdata(L, 1);

	uint32_t x;
	for(x = 0;x < aoi->tower_x;x++) {
		uint32_t z;
		for(z = 0;z < aoi->tower_z;z++) {
			tower_t* tower = &aoi->towers[x][z];
			kh_destroy(watcher,tower->hash);
		}
		free(aoi->towers[x]);
	}
	free(aoi->towers);
	free(aoi->objs);

	return 0;
}

static int
_init(lua_State* L,aoi_t* aoi) {
	lua_newtable(L);

	lua_pushcfunction(L, _release);
	lua_setfield(L, -2, "__gc");

	luaL_Reg l[] = {
		{ "add_object", _add_object },
		{ "remove_object", _remove_object },
		{ "update_object", _update_object },
		{ "add_watcher", _add_watcher },
		{ "remove_watcher", _remove_watcher },
		{ "update_watcher", _update_watcher },
		{ "get_viewers", _get_viewers },
		{ "get_visible", _get_visible },
		{ NULL, NULL },
	};
	luaL_newlib(L,l);

	lua_setfield(L, -2, "__index");

	lua_setmetatable(L, -2);
	return 1;
}

static int
_create(lua_State* L) {
	aoi_t* aoi_ctx = (aoi_t*)lua_newuserdata(L, sizeof(aoi_t));
	memset(aoi_ctx,0,sizeof(*aoi_ctx));

	aoi_ctx->max = lua_tointeger(L,1);

	aoi_ctx->objs = malloc(sizeof(object_t) * aoi_ctx->max);
	memset(aoi_ctx->objs,0,sizeof(object_t) * aoi_ctx->max);

	size_t i;
	for(i=0;i<aoi_ctx->max;i++) {
		object_t* obj = &aoi_ctx->objs[i];
		obj->id = i;
		obj->next = aoi_ctx->freelist;
		aoi_ctx->freelist = obj;
	}

	aoi_ctx->width = lua_tointeger(L,2);
	aoi_ctx->height = lua_tointeger(L,3);
	aoi_ctx->range = lua_tointeger(L,4);

	aoi_ctx->tower_x = aoi_ctx->width / aoi_ctx->range;
	aoi_ctx->tower_z = aoi_ctx->height / aoi_ctx->range;

	aoi_ctx->towers = malloc(aoi_ctx->tower_x * sizeof(*aoi_ctx->towers));
	uint32_t x;
	for(x = 0;x < aoi_ctx->tower_x;x++) {
		aoi_ctx->towers[x] = malloc(aoi_ctx->tower_z * sizeof(tower_t));
		memset(aoi_ctx->towers[x],0,aoi_ctx->tower_z * sizeof(tower_t));
		uint32_t z;
		for(z = 0;z < aoi_ctx->tower_z;z++) {
			tower_t* tower = &aoi_ctx->towers[x][z];
			tower->head = tower->tail = NULL;
			tower->hash = kh_init(watcher);
		}
	}
	return _init(L,aoi_ctx);
}

int
luaopen_toweraoi_core(lua_State* L){
	luaL_Reg l[] = {
		{ "create", _create },
		{ NULL, NULL },
	};
	luaL_newlib(L,l);
	return 1;
}
