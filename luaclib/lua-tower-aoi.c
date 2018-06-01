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
	float x;
	float z;
} location_t;

typedef struct object {
	int uid;
	int id;
	uint8_t type;

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

	object_t* pool;
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
new_object(lua_State* L,aoi_t* aoi,int uid,int type,float x,float z) {
	if (aoi->freelist == NULL) {
		luaL_error(L,"new object error:object count limited:%d",aoi->max);
	}
	object_t* obj = aoi->freelist;
	aoi->freelist = obj->next;
	obj->next = obj->prev = NULL;
	obj->param.link_node.link_prev = obj->param.link_node.link_next = NULL;
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
	} else {
		tower->head = object->param.link_node.link_next;
	}
	if (object->param.link_node.link_next != NULL) {
		object->param.link_node.link_next->prev = object->param.link_node.link_prev;
	} else {
		tower->tail = object->param.link_node.link_prev;
	}
	object->param.link_node.link_prev = NULL;
	object->param.link_node.link_next = NULL;
}

static inline struct object*
get_object(lua_State* L, aoi_t* aoi, int id, int type) {
	if (id < 0 || id >= aoi->max)
		luaL_error(L,"error aoi id:%d,aoi max:%d",id,aoi->max);

	struct object* result = &aoi->pool[id];
	if (result->type != type)
		luaL_error(L,"error aoi type:%d,%d",result->type,type);

	return result;
}

static inline void
check_position(lua_State* L, aoi_t* aoi, float x, float z) {
	if (x < 0 || z < 0 || x > aoi->width || z > aoi->height) {
		luaL_error(L,"add object error:invalid local:[%f,%f]",x,z);
	}
}

static int
ladd_object(lua_State* L) {
	aoi_t* aoi = (aoi_t*)lua_touserdata(L, 1);

	int uid = luaL_checkinteger(L, 2);
	float x = luaL_checknumber(L, 3);
	float z = luaL_checknumber(L, 4);

	check_position(L, aoi, x, z);

	object_t* self = new_object(L,aoi,uid,TYPE_OBJECT,x,z);

	location_t out;
	translate(aoi,&self->local,&out);

	tower_t* tower = &aoi->towers[(uint32_t)out.x][(uint32_t)out.z];

	link_object(tower,self);

	lua_pushinteger(L,self->id);
	lua_newtable(L);
	int i = 1;

	khiter_t k;
	for(k = kh_begin(tower->hash);k != kh_end(tower->hash);++k) {
		if (kh_exist(tower->hash,k)) {
			object_t* other = kh_value(tower->hash,k);
			if (other->uid == self->uid)
				continue;
			lua_pushinteger(L,other->uid);
			lua_rawseti(L,-2,i++);
		}
	}

	return 2;
}

static int
lremove_object(lua_State* L) {
	aoi_t* aoi = (aoi_t*)lua_touserdata(L, 1);
	int id = luaL_checkinteger(L, 2);

	object_t* self = get_object(L, aoi, id, TYPE_OBJECT);

	location_t out;
	translate(aoi,&self->local,&out);

	tower_t* tower = &aoi->towers[(uint32_t)out.x][(uint32_t)out.z];

	unlink_object(tower,self);

	lua_newtable(L);
	int i = 1;

	khiter_t k;
	for(k = kh_begin(tower->hash);k != kh_end(tower->hash);++k) {
		if (kh_exist(tower->hash,k)) {
			object_t* other = kh_value(tower->hash,k);
			if (other->uid == self->uid)
				continue;
			lua_pushinteger(L,other->uid);
			lua_rawseti(L,-2,i++);
		}
	}

	free_object(aoi,self);

	return 1;
}

static int
lupdate_object(lua_State* L) {
	aoi_t* aoi = (aoi_t*)lua_touserdata(L, 1);
	int id = luaL_checkinteger(L, 2);
	float nx = luaL_checknumber(L, 3);
	float nz = luaL_checknumber(L, 4);

	check_position(L, aoi, nx, nz);

	object_t* self = get_object(L, aoi, id, TYPE_OBJECT);

	location_t out;
	translate(aoi,&self->local,&out);
	tower_t* otower = &aoi->towers[(uint32_t)out.x][(uint32_t)out.z];

	self->local.x = nx;
	self->local.z = nz;
	translate(aoi,&self->local,&out);
	tower_t* ntower = &aoi->towers[(uint32_t)out.x][(uint32_t)out.z];

	if (ntower == otower) {
		return 0;
	}

	unlink_object(otower,self);

	link_object(ntower,self);

	object_t* leave = NULL;
	object_t* enter = NULL;
	khiter_t k;
	for(k = kh_begin(otower->hash);k != kh_end(otower->hash);++k) {
		if (kh_exist(otower->hash,k)) {
			object_t* other = kh_value(otower->hash,k);
			if (other->uid == self->uid)
				continue;
			if (leave == NULL) {
				leave = other;
				other->next = NULL;
				other->prev = NULL;
			} else {
				other->prev = NULL;
				other->next = leave;
				leave = other;
			}
		}
	}

	for(k = kh_begin(ntower->hash);k != kh_end(ntower->hash);++k) {
		if (kh_exist(ntower->hash,k)) {
			object_t* other = kh_value(ntower->hash,k);
			if (other->uid == self->uid)
				continue;

			if (other->next != NULL || other->prev != NULL || other == leave) {
				if (other->prev != NULL) {
					other->prev->next = other->next;
				}
				if (other->next != NULL) {
					other->next->prev = other->prev;
				}
				if (other == leave) {
					leave = other->next;
				}
				other->next = other->prev = NULL;
				
			} else {
				if (enter == NULL) {
					enter = other;
					other->next = NULL;
					other->prev = NULL;
				} else {
					enter->next = enter;
					other->prev = enter;
					other->next = NULL;
					enter = other;
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
ladd_watcher(lua_State* L) {
	aoi_t* aoi = (aoi_t*)lua_touserdata(L, 1);
	int uid = luaL_checkinteger(L, 2);
	float x = luaL_checknumber(L, 3);
	float z = luaL_checknumber(L, 4);
	int range = luaL_checkinteger(L, 5);

	check_position(L, aoi, x, z);

	object_t* self = new_object(L,aoi,uid,TYPE_WATCHER,x,z);
	self->param.range = range;

	location_t out;
	translate(aoi,&self->local,&out);

	region_t region;
	get_region(aoi,&out,&region,range);

	lua_pushinteger(L,self->id);
	lua_newtable(L);
	int i = 1;

	uint32_t x_index;
	for(x_index = region.begin_x;x_index <= region.end_x;x_index++) {
		uint32_t z_index;
		for(z_index = region.begin_z;z_index <= region.end_z;z_index++) {
			tower_t* tower = &aoi->towers[x_index][z_index];
			int ok;
			khiter_t k = kh_put(watcher, tower->hash, self->uid, &ok);
			assert(ok == 1 || ok == 2);
			kh_value(tower->hash, k) = self;

			struct object* cursor = tower->head;
			while(cursor) {
				if (cursor->uid != self->uid) {
					lua_pushinteger(L,cursor->uid);
					lua_rawseti(L,-2,i++);
				}
				cursor = cursor->param.link_node.link_next;
			}
		}
	}
	return 2;
}

static int
lremove_watcher(lua_State* L) {
	aoi_t* aoi = (aoi_t*)lua_touserdata(L, 1);
	int id = luaL_checkinteger(L,2);

	object_t* self = get_object(L, aoi, id, TYPE_WATCHER);

	location_t out;
	translate(aoi,&self->local,&out);

	region_t region;
	get_region(aoi,&out,&region,self->param.range);

	uint32_t x_index;
	for(x_index = region.begin_x;x_index <= region.end_x;x_index++) {
		uint32_t z_index;
		for(z_index = region.begin_z;z_index <= region.end_z;z_index++) {
			tower_t* tower = &aoi->towers[x_index][z_index];

			khiter_t k = kh_get(watcher, tower->hash, self->uid);
			assert(k != kh_end(tower->hash));
			kh_del(watcher,tower->hash,k);
		}
	}

	free_object(aoi,self);
	return 0;
}

static int
lupdate_watcher(lua_State* L) {
	aoi_t* aoi = (aoi_t*)lua_touserdata(L, 1);
	int id = luaL_checkinteger(L, 2);
	float nx = luaL_checknumber(L, 3);
	float nz = luaL_checknumber(L, 4);

	check_position(L, aoi, nx, nz);

	object_t* self = get_object(L, aoi, id, TYPE_WATCHER);

	location_t oout;
	translate(aoi,&self->local,&oout);

	location_t nout;
	self->local.x = nx;
	self->local.z = nz;
	translate(aoi,&self->local,&nout);

	if (oout.x == nout.x && oout.z == nout.z) {
		return 0;
	}

	region_t oregion;
	get_region(aoi,&oout,&oregion,self->param.range);

	region_t nregion;
	get_region(aoi,&nout,&nregion,self->param.range);

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
			khiter_t k = kh_get(watcher, tower->hash, self->uid);
			assert(k != kh_end(tower->hash));
			kh_del(watcher,tower->hash,k);

			struct object* cursor = tower->head;
			while(cursor) {
				if (cursor->uid != self->uid) {
					lua_pushinteger(L,cursor->uid);
					lua_rawseti(L,-2,i++);
				}
				
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
			khiter_t k = kh_put(watcher, tower->hash, self->uid, &ok);
			assert(k != kh_end(tower->hash));
			kh_value(tower->hash, k) = self;

			struct object* cursor = tower->head;
			while(cursor) {
				if (cursor->uid != self->uid) {
					lua_pushinteger(L,cursor->uid);
					lua_rawseti(L,-2,i++);
				}
				cursor = cursor->param.link_node.link_next;
			}
		}
	}

	return 2;
}

static int
lget_viewers(lua_State* L) {
	aoi_t* aoi = (aoi_t*)lua_touserdata(L, 1);
	int id = luaL_checkinteger(L, 2);
	object_t* self = get_object(L, aoi, id, TYPE_OBJECT);

	location_t out;
	translate(aoi,&self->local,&out);

	tower_t* tower = &aoi->towers[(uint32_t)out.x][(uint32_t)out.z];

	lua_newtable(L);
	int i = 1;

	khiter_t k;
	for(k = kh_begin(tower->hash);k != kh_end(tower->hash);++k) {
		if (kh_exist(tower->hash,k)) {
			object_t* other = kh_value(tower->hash,k);
			if (self->uid != other->uid) {
				lua_pushinteger(L,other->uid);
				lua_rawseti(L,-2,i++);
			}
		}
	}

	return 1;
}

static int
lget_visible(lua_State* L) {
	aoi_t* aoi = (aoi_t*)lua_touserdata(L, 1);
	int id = luaL_checkinteger(L, 2);
	object_t* self = get_object(L, aoi, id, TYPE_WATCHER);

	location_t out;
	translate(aoi,&self->local,&out);

	region_t region;
	get_region(aoi,&out,&region,self->param.range);

	lua_newtable(L);
	int i = 1;

	uint32_t x_index;
	for(x_index = region.begin_x;x_index <= region.end_x;x_index++) {
		uint32_t z_index;
		for(z_index = region.begin_z;z_index <= region.end_z;z_index++) {
			tower_t* tower = &aoi->towers[x_index][z_index];
			struct object* cursor = tower->head;
			while(cursor) {
				if (self->uid != cursor->uid) {
					lua_pushinteger(L,cursor->uid);
					lua_rawseti(L,-2,i++);
				}
				cursor = cursor->param.link_node.link_next;
			}
		}
	}
	return 1;

}

static int
lrelease(lua_State* L) {
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
	free(aoi->pool);

	return 0;
}

static int
_init(lua_State* L,aoi_t* aoi) {
	lua_newtable(L);

	lua_pushcfunction(L, lrelease);
	lua_setfield(L, -2, "__gc");

	luaL_Reg l[] = {
		{ "add_object", ladd_object },
		{ "remove_object", lremove_object },
		{ "update_object", lupdate_object },
		{ "add_watcher", ladd_watcher },
		{ "remove_watcher", lremove_watcher },
		{ "update_watcher", lupdate_watcher },
		{ "get_viewers", lget_viewers },
		{ "get_visible", lget_visible },
		{ NULL, NULL },
	};
	luaL_newlib(L,l);

	lua_setfield(L, -2, "__index");

	lua_setmetatable(L, -2);
	return 1;
}

static int
lcreate(lua_State* L) {
	int max = luaL_checkinteger(L, 1);
	if (max <= 0)
		luaL_error(L, "create error size:%d", max);

	aoi_t* aoi_ctx = (aoi_t*)lua_newuserdata(L, sizeof(aoi_t));
	memset(aoi_ctx, 0, sizeof(*aoi_ctx));

	aoi_ctx->max = max;

	aoi_ctx->pool = malloc(sizeof(object_t) * aoi_ctx->max);
	memset(aoi_ctx->pool,0,sizeof(object_t) * aoi_ctx->max);

	aoi_ctx->width = luaL_checkinteger(L, 2);
	aoi_ctx->height = luaL_checkinteger(L, 3);
	aoi_ctx->range = luaL_checkinteger(L, 4);

	if (aoi_ctx->width < aoi_ctx->range)
		aoi_ctx->range = aoi_ctx->width;

	if (aoi_ctx->height < aoi_ctx->range)
		aoi_ctx->range = aoi_ctx->height;

	aoi_ctx->tower_x = aoi_ctx->width / aoi_ctx->range + 1;
	aoi_ctx->tower_z = aoi_ctx->height / aoi_ctx->range + 1;

	size_t i;
	for(i = 0; i < aoi_ctx->max; i++) {
		object_t* obj = &aoi_ctx->pool[i];
		obj->id = i;
		obj->next = aoi_ctx->freelist;
		aoi_ctx->freelist = obj;
	}

	aoi_ctx->towers = malloc(aoi_ctx->tower_x * sizeof(*aoi_ctx->towers));
	uint32_t x;
	for(x = 0;x < aoi_ctx->tower_x; x++) {
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
		{ "create", lcreate },
		{ NULL, NULL },
	};
	luaL_newlib(L,l);
	return 1;
}
