#include <lua.h>
#include <lauxlib.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>

#define BUFFER_SIZE 128

#define MAX_DEPTH	32

#define TYPE_NUMBER_ZERO 	0
#define TYPE_NUMBER_BYTE 	1
#define TYPE_NUMBER_WORD 	2
#define TYPE_NUMBER_DWORD 	3
#define TYPE_NUMBER_QWORD 	4

#define FTYPE_INT 		0
#define FTYPE_FLOAT 	1
#define FTYPE_DOUBLE 	2
#define FTYPE_STRING 	3
#define FTYPE_PROTOCOL 	4


struct message_writer {
	char* ptr;
	int offset;
	int size;
	char init[BUFFER_SIZE];
};

struct message_reader {
	char* ptr;
	int offset;
	int size;
};

struct field {
	char* name;
	int array;
	int type;
	char* type_name;
	struct field* nest;
	int cap;
	int size;
};

struct protocol {
	char* name;
	struct field* field;
	int cap;
	int size;
};

struct context {
	struct protocol** slots;
	int cap;
	lua_State* L;
};

inline static void
writer_reserve(struct message_writer* writer,size_t sz) {
	if (writer->offset + sz > writer->size) {
		size_t nsize = writer->size * 2;
		while (nsize < writer->offset + sz)
			nsize = nsize * 2;

		char* nptr = (char*)malloc(nsize);
		memcpy(nptr, writer->ptr, writer->size);
		writer->size = nsize;

		if (writer->ptr != writer->init)
			free(writer->ptr);
		writer->ptr = nptr;
	}
}

inline static void
writer_init(struct message_writer* writer) {
	writer->ptr = writer->init;
	writer->offset = 0;
	writer->size = BUFFER_SIZE;
}

inline static void
writer_release(struct message_writer* writer) {
	if (writer->ptr != writer->init) {
		free(writer->ptr);
	}
}

inline static void
writer_push(struct message_writer* writer,void* data,size_t size) {
	writer_reserve(writer,size);
	memcpy(writer->ptr+writer->offset,data,size);
	writer->offset += size;
}

inline static void
write_byte(struct message_writer* writer,uint8_t val) {
	writer_push(writer,&val,sizeof(uint8_t));
}

inline static void
write_short(struct message_writer* writer,ushort val) {
	writer_push(writer,&val,sizeof(ushort));
}

inline static void
write_int(struct message_writer* writer,lua_Integer val) {
	if (val == 0) {
		write_byte(writer,TYPE_NUMBER_ZERO);
	} else if (val != (int32_t)val) {
		int64_t v64 = val;
		write_byte(writer,TYPE_NUMBER_QWORD);
		writer_push(writer, &v64, sizeof(v64));
	} else if (val < 0) {
		int32_t v32 = (int32_t)val;
		write_byte(writer,TYPE_NUMBER_DWORD);
		writer_push(writer, &v32, sizeof(v32));
	} else if (val<0x100) {
		write_byte(writer,TYPE_NUMBER_BYTE);
		uint8_t byte = (uint8_t)val;
		writer_push(writer, &byte, sizeof(byte));
	} else if (val<0x10000) {
		write_byte(writer,TYPE_NUMBER_WORD);
		uint16_t word = (uint16_t)val;
		writer_push(writer, &word, sizeof(word));
	} else {
		write_byte(writer,TYPE_NUMBER_DWORD);
		uint32_t v32 = (uint32_t)val;
		writer_push(writer, &v32, sizeof(v32));
	}
}

inline static void
write_float(struct message_writer* writer,float val) {
	writer_push(writer,&val,sizeof(float));
}

inline static void
write_double(struct message_writer* writer,double val) {
	writer_push(writer,&val,sizeof(double));
}

inline static void
write_string(struct message_writer* writer,const char* str,size_t size) {
	write_short(writer,size);
	writer_push(writer,(void*)str,size);
}

inline static void
reader_pop(lua_State* L,struct message_reader* reader,uint8_t* data,size_t size) {
	if (reader->size - reader->offset < size) {
		luaL_error(L,"decode error:invalid mesasge");
	}
	memcpy(data,reader->ptr+reader->offset,size);
	reader->offset += size;
}

inline static int
reader_left(struct message_reader* reader) {
	return reader->size - reader->offset;
}

inline static ushort
read_short(lua_State* L,struct message_reader* reader) {
	short val;
	reader_pop(L,reader,(uint8_t*)&val,sizeof(ushort));
	return val;
}

inline static lua_Integer
read_int(lua_State* L,struct message_reader* reader) {
	
	uint8_t type;
	reader_pop(L,reader,&type,sizeof(uint8_t));

	switch (type) {
	case TYPE_NUMBER_ZERO:
		return 0;
	case TYPE_NUMBER_BYTE: {
		uint8_t n;
		reader_pop(L,reader,&n,sizeof(uint8_t));
		return n;
	}
	case TYPE_NUMBER_WORD: {
		uint16_t n;
		reader_pop(L,reader,(uint8_t*)&n,sizeof(uint16_t));
		return n;
	}
	case TYPE_NUMBER_DWORD: {
		uint32_t n;
		reader_pop(L,reader,(uint8_t*)&n,sizeof(uint32_t));
		return n;
	}
	case TYPE_NUMBER_QWORD: {
		uint64_t n;
		reader_pop(L,reader,(uint8_t*)&n,sizeof(uint64_t));
		return n;
	}
	default:
		luaL_error(L,"decode error:invalid int type:%d",type);
		return 0;
	}
}

inline static float
read_float(lua_State* L,struct message_reader* reader) {
	float val;
	reader_pop(L,reader,(uint8_t*)&val,sizeof(float));
	return val;
}

inline static double
read_double(lua_State* L,struct message_reader* reader) {
	double val;
	reader_pop(L,reader,(uint8_t*)&val,sizeof(double));
	return val;
}

inline static char*
read_string(lua_State* L,struct message_reader* reader,size_t* size) {
	char* result;
	*size = read_short(L,reader);
	if (reader_left(reader) < *size) {
		luaL_error(L,"decode error:invalid mesasge");
	}
	result = reader->ptr + reader->offset;
	reader->offset += *size;
	return result;
}

inline void
pack_int(lua_State* L,struct message_writer* writer,struct field* f,int index) {
	int vt = lua_type(L,index);

	if (f->array) {
		if (vt != LUA_TTABLE) {
			writer_release(writer);
			luaL_error(L,"field:%s expect table,not %s",f->name,lua_typename(L,vt));
		}

		int array_size = lua_rawlen(L, index);
		write_short(writer,array_size);
		int i;
		for (i = 1; i <= array_size; i++) {
			lua_rawgeti(L, index, i);
			vt = lua_type(L,-1);
			if (vt != LUA_TNUMBER) {
				writer_release(writer);
				luaL_error(L,"field:%s array member expect int,not %s",f->name,lua_typename(L,vt));
			}
			write_int(writer,lua_tointeger(L,-1));
			lua_pop(L, 1);
		}
	} else {
		if (vt != LUA_TNUMBER) {
			writer_release(writer);
			luaL_error(L,"field:%s expect int,not %s",f->name,lua_typename(L,vt));
		}
		write_int(writer,lua_tointeger(L,index));
	}
}  

inline void
pack_float(lua_State* L,struct message_writer* writer,struct field* f,int index) {
	int vt = lua_type(L,index);

	if (f->array) {
		if (vt != LUA_TTABLE) {
			writer_release(writer);
			luaL_error(L,"field:%s expect table,not %s",f->name,lua_typename(L,vt));
		}

		int array_size = lua_rawlen(L, index);
		write_short(writer,array_size);
		int i;
		for (i = 1; i <= array_size; i++) {
			lua_rawgeti(L, index, i);
			vt = lua_type(L,-1);
			if (vt != LUA_TNUMBER) {
				writer_release(writer);
				luaL_error(L,"field:%s array member expect float,not %s",f->name,lua_typename(L,vt));
			}
			write_float(writer,lua_tonumber(L,-1));
			lua_pop(L, 1);
		}
	} else {
		if (vt != LUA_TNUMBER) {
			writer_release(writer);
			luaL_error(L,"field:%s expect float,not %s",f->name,lua_typename(L,vt));
		}
		write_float(writer,lua_tonumber(L,index));
	}
} 

inline void
pack_double(lua_State* L,struct message_writer* writer,struct field* f,int index) {
	int vt = lua_type(L,index);
	if (f->array) {
		if (vt != LUA_TTABLE) {
			writer_release(writer);
			luaL_error(L,"field:%s expect table,not %s",f->name,lua_typename(L,vt));
		}

		int array_size = lua_rawlen(L, index);
		write_short(writer,array_size);
		int i;
		for (i = 1; i <= array_size; i++) {
			lua_rawgeti(L, index, i);
			vt = lua_type(L,-1);
			if (vt != LUA_TNUMBER) {
				writer_release(writer);
				luaL_error(L,"field:%s array member expect float,not %s",f->name,lua_typename(L,vt));
			}
			write_double(writer,lua_tonumber(L,-1));
			lua_pop(L, 1);
		}
	} else {
		if (vt != LUA_TNUMBER) {
			writer_release(writer);
			luaL_error(L,"field:%s expect float,not %s",f->name,lua_typename(L,vt));
		}
		write_double(writer,lua_tonumber(L,index));
	}
}

inline void
pack_string(lua_State* L,struct message_writer* writer,struct field* f,int index) {
	int vt = lua_type(L,index);
	if (f->array) {
		if (vt != LUA_TTABLE) {
			writer_release(writer);
			luaL_error(L,"field:%s expect table,not %s",f->name,lua_typename(L,vt));
		}

		int array_size = lua_rawlen(L, index);
		write_short(writer,array_size);
		int i;
		for (i = 1; i <= array_size; i++) {
			lua_rawgeti(L, index, i);
			vt = lua_type(L,-1);
			if (vt != LUA_TSTRING) {
				writer_release(writer);
				luaL_error(L,"field:%s array member expect string,not %s",f->name,lua_typename(L,vt));
			}
			size_t size;
			const char* str = lua_tolstring(L,-1,&size);
			write_string(writer,str,size);
			lua_pop(L, 1);
		}
	} else {
		if (vt != LUA_TSTRING) {
			writer_release(writer);
			luaL_error(L,"field:%s expect string,not %s",f->name,lua_typename(L,vt));
		}
		size_t size;
		const char* str = lua_tolstring(L,index,&size);
		write_string(writer,str,size);
	}
}

void pack_field(lua_State* L,struct message_writer* writer,struct field* f,int index,int depth);

static inline void
pack_one(lua_State* L,struct message_writer* writer,struct field* f,int index,int depth) {
	switch(f->type) {
		case FTYPE_INT: {
			pack_int(L,writer,f,index);
			break;
		}
		case FTYPE_FLOAT: {
			pack_float(L,writer,f,index);
			break;
		}
		case FTYPE_DOUBLE: {
			pack_double(L,writer,f,index);
			break;
		}
		case FTYPE_STRING: {
			pack_string(L,writer,f,index);
			break;
		}
		case FTYPE_PROTOCOL: {
			pack_field(L,writer,f,index,depth);
			break;
		}
		default: {
			writer_release(writer);
			luaL_error(L,"pack error:invalid name:%s,type:%d",f->name,f->type);
		}
	}
}

void
pack_field(lua_State* L,struct message_writer* writer,struct field* parent,int index,int depth) {
	depth++;
	if (depth > MAX_DEPTH) {
		writer_release(writer);
		luaL_error(L,"message pack too depth");
	}

	// printf("pack depth:%d,%d\n",depth,lua_gettop(L));
	
	int vt = lua_type(L,index);
	if (vt != LUA_TTABLE) {
		writer_release(writer);
		luaL_error(L,"field:%s expect table,not %s",parent->name,lua_typename(L,vt));
	}

	if (parent->array) {
		int array_size = lua_rawlen(L,index);
		write_short(writer,array_size);

		int i;
		for (i = 0; i < array_size; i++) {
			lua_rawgeti(L, index, i+1);
			vt = lua_type(L,-1);
			if (vt != LUA_TTABLE) {
				writer_release(writer);
				luaL_error(L,"field:%s array member expect table,not %s",parent->name,lua_typename(L,vt));
			}

			int j;
			for(j=0;j < parent->size;j++) {
				struct field* f = &parent->nest[j];
				lua_getfield(L, -1, f->name);
				pack_one(L,writer,f,index+2,depth);
				lua_pop(L,1);
			}
			lua_pop(L,1);
		}
	} else {
		int i;
		for(i=0;i < parent->size;i++) {
			struct field* f = &parent->nest[i];
			lua_getfield(L, index, f->name);
			pack_one(L,writer,f,index+1,depth);
			lua_pop(L,1);
		}
	}
}


inline void
unpack_int(lua_State* L,struct message_reader* reader,struct field* f,int index) {
	if (f->array) {
		int array_size = read_short(L,reader);
		lua_createtable(L,0,0);
		int i;
		for (i = 1; i <= array_size; i++) {
			lua_Integer val = read_int(L,reader);
			lua_pushinteger(L,val);
			lua_rawseti(L,-2,i);
		}
		lua_setfield(L,index,f->name);
	} else {
		lua_Integer val = read_int(L,reader);
		lua_pushinteger(L,val);
		lua_setfield(L,index,f->name);
	}
}  

inline void
unpack_float(lua_State* L,struct message_reader* reader,struct field* f,int index) {
	if (f->array) {
		int array_size = read_short(L,reader);
		lua_createtable(L,0,0);
		int i;
		for (i = 1; i <= array_size; i++) {
			float val = read_float(L,reader);
			lua_pushnumber(L,val);
			lua_rawseti(L,-2,i);
		}
		lua_setfield(L,index,f->name);
	} else {
		float val = read_float(L,reader);
		lua_pushnumber(L,val);
		lua_setfield(L,index,f->name);
	}
} 

inline void
unpack_double(lua_State* L,struct message_reader* reader,struct field* f,int index) {
	if (f->array) {
		int array_size = read_short(L,reader);
		lua_createtable(L,0,0);
		int i;
		for (i = 1; i <= array_size; i++) {
			double val = read_double(L,reader);
			lua_pushnumber(L,val);
			lua_rawseti(L,-2,i);
		}
		lua_setfield(L,index,f->name);
	} else {
		double val = read_double(L,reader);
		lua_pushnumber(L,val);
		lua_setfield(L,index,f->name);
	}
}

inline void
unpack_string(lua_State* L,struct message_reader* reader,struct field* f,int index) {
	if (f->array) {
		int array_size = read_short(L,reader);
		lua_createtable(L,0,0);
		int i;
		for (i = 1; i <= array_size; i++) {
			size_t size;
			char* val = read_string(L,reader,&size);
			lua_pushlstring(L,val,size);
			lua_rawseti(L,-2,i);
		}
		lua_setfield(L,index,f->name);
	} else {
		size_t size;
		char* val = read_string(L,reader,&size);
		lua_pushlstring(L,val,size);
		lua_setfield(L,index,f->name);
	}
}

static inline void unpack_one(lua_State* L,struct message_reader* reader,struct field* f,int index,int depth);

void
unpack_field(lua_State* L,struct message_reader* reader,struct field* parent,int index,int depth) {
	depth++;
	if (depth > MAX_DEPTH) {
		luaL_error(L,"message unpack too depth");
	}
	// printf("unpack depth:%d,%d\n",depth,lua_gettop(L));
	
	if (parent->array) {
		int array_size = read_short(L,reader);
		lua_createtable(L,0,0);
		int i;
		for (i = 1; i <= array_size; i++) {
			int j;
			lua_createtable(L,0,parent->size);
			for(j=0;j < parent->size;j++) {
				struct field* f = &parent->nest[j];
				unpack_one(L,reader,f,index + 2,depth);
			}
			lua_seti(L,-2,i);
		}
		lua_setfield(L,index,parent->name);
	} else {
		lua_createtable(L,0,parent->size);
		int i;
		for(i=0;i < parent->size;i++) {
			struct field* f = &parent->nest[i];
			unpack_one(L,reader,f,index + 1,depth);
		}
		lua_setfield(L,index,parent->name);
	}
}

static inline void
unpack_one(lua_State* L,struct message_reader* reader,struct field* f,int index,int depth) {
	switch(f->type) {
		case FTYPE_INT: {
			unpack_int(L,reader,f,index);
			break;
		}
		case FTYPE_FLOAT: {
			unpack_float(L,reader,f,index);
			break;
		}
		case FTYPE_DOUBLE: {
			unpack_double(L,reader,f,index);
			break;
		}
		case FTYPE_STRING: {
			unpack_string(L,reader,f,index);
			break;
		}
		case FTYPE_PROTOCOL: {
			unpack_field(L,reader,f,index,depth);
			break;
		}
		default: {
			luaL_error(L,"unpack error:invalid name:%s,type:%d",f->name,f->type);
		}
	}
}

int
lencode_protocol(lua_State* L) {
	struct message_writer writer;
	writer_init(&writer);

	struct context* ctx = lua_touserdata(L,1);
	int index = luaL_checkinteger(L,2);
	if (index >= ctx->cap || ctx->slots[index] == NULL) {
		luaL_error(L,"index:%d error",index);
	}

	int depth = 1;
	luaL_checkstack(L, MAX_DEPTH*2 + 8, NULL);

	struct protocol* ptl = ctx->slots[index];
	luaL_checktype(L, 3, LUA_TTABLE);
	int i;
	for(i=0;i < ptl->size;i++) {
		struct field* f = &ptl->field[i];
		lua_getfield(L, 3, f->name);
		pack_one(L,&writer,f,4,depth);
		lua_pop(L,1);
	}

	lua_pushlstring(L,writer.ptr,writer.offset);

	writer_release(&writer);
	return 1;
}

int
ldecode_protocol(lua_State* L) {
	struct context* ctx = lua_touserdata(L,1);
	int index = luaL_checkinteger(L,2);
	if (index >= ctx->cap || ctx->slots[index] == NULL) {
		luaL_error(L,"index:%d error",index);
	}
	size_t size;
	const char* str = NULL;
	switch(lua_type(L,3)) {
		case LUA_TSTRING: {
			str = lua_tolstring(L, 3, &size);
			break;
		}
		case LUA_TLIGHTUSERDATA:{
			str = lua_touserdata(L, 3);
			size = lua_tointeger(L, 4);
			break;
		}
		default:
			luaL_error(L,"unkown type:%s",lua_typename(L,lua_type(L,3)));
	}

	struct message_reader reader;
	reader.ptr = (char*)str;
	reader.offset = 0;
	reader.size = size;

	int depth = 1;
	luaL_checkstack(L, MAX_DEPTH*2 + 8, NULL);

	struct protocol* ptl = ctx->slots[index];
	lua_createtable(L,0,ptl->size);
	int top = lua_gettop(L);
	int i;
	for(i=0;i < ptl->size;i++) {
		struct field* f = &ptl->field[i];
		unpack_one(L,&reader,f,top,depth);
	}
	
	if (reader.offset != reader.size) {
		luaL_error(L,"decode error");
	}
	return 1;
}

char* 
str_alloc(struct context* ctx,const char* str,size_t size) {
	lua_getfield(ctx->L, 1, str);
	if (!lua_isnoneornil(ctx->L,-1)) {
		char* result = lua_touserdata(ctx->L,-1);
		lua_pop(ctx->L,1);
		return result;
	}
	lua_pop(ctx->L,1);

	lua_pushlstring(ctx->L,str,size);
	char* ptr = (char*)lua_tostring(ctx->L,-1);
	lua_pushlightuserdata(ctx->L,ptr);
	lua_settable(ctx->L,1);
	return ptr;
}

struct protocol* 
create_protocol(struct context* ctx,int id,char* name,size_t size) {
	struct protocol* ptl = malloc(sizeof(*ptl));
	memset(ptl,0,sizeof(*ptl));
	ptl->name = str_alloc(ctx,name,size);
	ptl->cap = 4;
	ptl->size = 0;
	ptl->field = malloc(sizeof(*ptl->field) * ptl->cap);
	memset(ptl->field,0,sizeof(*ptl->field) * ptl->cap);

	if (id >= ctx->cap) {
		int ncap = ctx->cap * 2;
		if (id >= ncap)
			ncap = id + 1;
		struct protocol** nslots = malloc(sizeof(*nslots) * ncap);
		memset(nslots,0,sizeof(*nslots) * ncap);
		memcpy(nslots,ctx->slots,sizeof(*ctx->slots) * ctx->cap);
		free(ctx->slots);
		ctx->slots = nslots;
		ctx->cap = ncap;
	}

	ctx->slots[id] = ptl;
	return ptl;
}

struct field* 
create_field(struct context* ctx,struct protocol* ptl,const char* name,int array,int type,char* type_name) {
	if (ptl->size >= ptl->cap) {
		int ncap = ptl->cap * 2;
		struct field* nf = malloc(sizeof(*nf) * ncap);
		memset(nf,0,sizeof(*nf) * ncap);
		memcpy(nf,ptl->field,sizeof(*ptl->field) * ptl->cap);
		free(ptl->field);
		ptl->field = nf;
		ptl->cap = ncap;
	}
	struct field* f = &ptl->field[ptl->size];
	f->name = str_alloc(ctx,name,strlen(name));
	f->array = array;
	f->type = type;
	f->type_name = str_alloc(ctx,type_name,strlen(type_name));

	ptl->size++;
	return f;
}

struct field* 
create_nest_field(struct context* ctx,struct field* parent,const char* name,int array,int type,char* type_name) {
	if (parent->nest == NULL) {
		parent->cap = 4;
		parent->size = 0;
		parent->nest = malloc(sizeof(*parent->nest) * parent->cap);
		memset(parent->nest,0,sizeof(*parent->nest) * parent->cap);
	}

	if (parent->size >= parent->cap) {
		int ncap = parent->cap * 2;
		struct field* nf = malloc(sizeof(*nf) * ncap);
		memset(nf,0,sizeof(*nf) * ncap);
		memcpy(nf,parent->nest,sizeof(*parent->nest) * parent->cap);
		free(parent->nest);
		parent->nest = nf;
		parent->cap = ncap;
	}

	struct field* f = &parent->nest[parent->size];
	f->name = str_alloc(ctx,name,strlen(name));
	f->array = array;
	f->type = type;
	f->type_name = str_alloc(ctx,type_name,strlen(type_name));

	parent->size++;
	return f;
}

void
import_nest_field(lua_State* L,struct context* ctx,struct field* parent,int index,int depth) {
	// printf("depth:%d,%d\n",depth,lua_gettop(L));
	int array_size = lua_rawlen(L, index);
	int i;
	for (i = 1; i <= array_size; i++) {
		lua_rawgeti(L, index, i);
		
		lua_getfield(L,-1,"type");
		int type = lua_tointeger(L,-1);
		lua_pop(L, 1);

		lua_getfield(L,-1,"array");
		int array = lua_toboolean(L,-1);
		lua_pop(L, 1);

		lua_getfield(L,-1,"name");
		const char* name = lua_tostring(L,-1);
		lua_pop(L, 1);

		lua_getfield(L,-1,"type_name");
		const char* type_name = lua_tostring(L,-1);
		lua_pop(L, 1);

		struct field* f = create_nest_field(ctx,parent,(char*)name,array,type,(char*)type_name);
		if (type == FTYPE_PROTOCOL) {
			lua_getfield(L,-1,"fields");
			import_nest_field(L,ctx,f,lua_gettop(L),++depth);
			lua_pop(L, 1);
		}

		lua_pop(L, 1);
	}
}

void
import_field(lua_State* L,struct context* ctx,struct protocol* ptl,int index,int depth) {
	int array_size = lua_rawlen(L, index);
	int i;
	for (i = 1; i <= array_size; i++) {
		lua_rawgeti(L, index, i);
		
		lua_getfield(L,-1,"type");
		int type = lua_tointeger(L,-1);
		lua_pop(L, 1);

		lua_getfield(L,-1,"array");
		int array = lua_toboolean(L,-1);
		lua_pop(L, 1);

		lua_getfield(L,-1,"name");
		const char* name = lua_tostring(L,-1);
		lua_pop(L, 1);

		lua_getfield(L,-1,"type_name");
		const char* type_name = lua_tostring(L,-1);
		lua_pop(L, 1);

		struct field* f = create_field(ctx,ptl,(char*)name,array,type,(char*)type_name);
		if (type == FTYPE_PROTOCOL) {
			lua_getfield(L,-1,"fields");
			import_nest_field(L,ctx,f,lua_gettop(L),++depth);
			lua_pop(L, 1);
		}

		lua_pop(L, 1);
	}
}

int
lload_protocol(lua_State* L) {
	struct context* ctx = lua_touserdata(L, 1);
	int id = lua_tointeger(L, 2);
	size_t size;
	const char* name = lua_tolstring(L, 3, &size);
	luaL_checktype(L, 4, LUA_TTABLE);

	if (id < ctx->cap && ctx->slots[id] != NULL) {
		luaL_error(L, "id:%d already load", id);
	}

	luaL_checkstack(L, MAX_DEPTH * 2 + 8, NULL);

	struct protocol* ptl = create_protocol(ctx, id, (char*)name, size + 1);
	int depth = 0;
	lua_getfield(L,-1,"fields");
	if (!lua_isnil(L,-1))
		import_field(L,ctx,ptl,lua_gettop(L),++depth);

	return 0;
}

void
free_nest(struct field* f) {
	if (f->nest != NULL)
		free_nest(f->nest);
	free(f);
}

int
lprotocol_ctx_release(lua_State* L) {
	struct context* ctx = lua_touserdata(L,1);
	int i;
	for(i=0;i<ctx->cap;i++) {
		struct protocol* ptl = ctx->slots[i];
		if (ptl) {
			int j;
			for(j=0;j< ptl->size;j++) {
				struct field* f = &ptl->field[j];
				if (f->nest != NULL) {
					free_nest(f->nest);
				}
			}
			free(ptl->field);
			free(ptl);
		}
	}
	free(ctx->slots);
	lua_close(ctx->L);

	return 0;
}

int
_dump_all_protocol(lua_State* L) {
	struct context* ctx = lua_touserdata(L,1);
	lua_newtable(L);
	int i;
	for(i=0;i<ctx->cap;i++) {
		struct protocol* ptl = ctx->slots[i];
		if (ptl) {
			lua_pushstring(L,ptl->name);
			lua_pushinteger(L,i);
			lua_settable(L,-3);
		}
	}
	return 1;
}

void
dump_nest(lua_State* L,struct field* parent) {
	lua_newtable(L);
	int i;
	for(i=0;i<parent->size;i++) {
		lua_newtable(L);
		struct field* f = &parent->nest[i];
		lua_pushstring(L,f->name);
		lua_setfield(L,-2,"name");
		lua_pushinteger(L,f->array);
		lua_setfield(L,-2,"array");
		lua_pushinteger(L,f->type);
		lua_setfield(L,-2,"type");
		lua_pushstring(L,f->type_name);
		lua_setfield(L,-2,"type_name");
		if (f->type == FTYPE_PROTOCOL) {
			dump_nest(L,f);
			lua_setfield(L,-2,"fields");
		}
		lua_seti(L,-2,i+1);
	}
}

int
_dump_protocol(lua_State* L) {
	struct context* ctx = lua_touserdata(L,1);
	int index = lua_tointeger(L,2);
	if (index >= ctx->cap) {
		return 0;
	}

	struct protocol* ptl = ctx->slots[index];
	if (!ptl)
		return 0;

	lua_newtable(L);

	lua_pushstring(L,ptl->name);
	lua_setfield(L,-2,"name");

	lua_newtable(L);

	int i;
	for(i=0;i<ptl->size;i++) {
		lua_newtable(L);
		struct field* f = &ptl->field[i];
		lua_pushstring(L,f->name);
		lua_setfield(L,-2,"name");
		lua_pushinteger(L,f->array);
		lua_setfield(L,-2,"array");
		lua_pushinteger(L,f->type);
		lua_setfield(L,-2,"type");
		lua_pushstring(L,f->type_name);
		lua_setfield(L,-2,"type_name");
		if (f->type == FTYPE_PROTOCOL) {
			dump_nest(L,f);
			lua_setfield(L,-2,"fields");
		}

		lua_seti(L,-2,i+1);
	}
	lua_setfield(L,-2,"fields");

	return 1;
}

int
lprotocol_ctx_new(lua_State* L) {
	struct context* ctx = lua_newuserdata(L, sizeof(*ctx));
	memset(ctx,0,sizeof(*ctx));
	ctx->cap = 64;
	ctx->slots = malloc(sizeof(*ctx->slots) * ctx->cap);
	memset(ctx->slots,0,sizeof(*ctx->slots) * ctx->cap);

	ctx->L = luaL_newstate();
	lua_settop(ctx->L,0);
	lua_newtable(ctx->L);

	if (luaL_newmetatable(L, "meta_protocol")) {
        const luaL_Reg meta_protocol[] = {
            { "load", lload_protocol },
            { "encode", lencode_protocol },
			{ "decode", ldecode_protocol },
			{ "dump_all", _dump_all_protocol },
			{ "dump", _dump_protocol },
            { NULL, NULL },
        };
        luaL_newlib(L,meta_protocol);
        lua_setfield(L, -2, "__index");

        lua_pushcfunction(L, lprotocol_ctx_release);
        lua_setfield(L, -2, "__gc");
    }
    lua_setmetatable(L, -2);
    return 1;
}

int
luaopen_protocolcore(lua_State* L) {
	luaL_checkversion(L);
	luaL_Reg l[] = {
		{ "new", lprotocol_ctx_new },
		{ NULL, NULL },
	};
	luaL_newlib(L,l);
	return 1;
}