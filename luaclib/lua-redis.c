#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <sys/time.h>
#include <stdint.h>
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "convert.h"

#define BUFFER_SIZE 1024

#define PHASE_BEGIN 		1
#define PHASE_STATUS		2
#define PHASE_NUMBER		3
#define PHASE_STRING		4
#define PHASE_ARRAY			5

struct write_buffer {
	char* ptr;
	size_t size;
	size_t offset;
	char init[BUFFER_SIZE];
};

struct read_buffer {
	char* data;
	int size;
	int offset;
	struct read_buffer* prev;
	struct read_buffer* next;
};

struct read_buffer_cursor {
	struct read_buffer* rb;
	int offset;
	int total;
};

struct reply_string {
	size_t size;
};

struct reply_array {
	uint32_t count;
	uint32_t index;
	size_t reserve;
	int ref;
};

struct data_collector {
	struct read_buffer* head;
	struct read_buffer* tail;
	struct read_buffer* freelist;
	struct read_buffer_cursor cursor;

	int total;

	int phase;
	union {
		struct reply_string string;
		struct reply_array array;
	} reply;

	char* reserve;
	size_t size;
};

static size_t
conv_number(char* str,double d) {
	size_t len = 0;
	int32_t i32 = (int32_t)d;
	int64_t i64 = (int64_t)d;

	if ((double)i32 == d) {
		char* end = i32toa_fast(i32, str);
		*end = 0;
		len = end - str;
	}
	else if ((double)i64 == d) {
		char* end = i64toa_fast(i64, str);
		*end = 0;
		len = end - str;
	} else {
		dtoa_fast(d, str);
		len = strlen(str);
	}
	return len;
}

static inline lua_Integer
strtonumber(lua_State* L,char* line) {
	char* end = NULL;
	lua_Number integer = strtod(line, &end);
	if (line == end) {
		luaL_error(L, "parse number error:%s",line);
	}
	return integer;
}

static inline void
wb_init(struct write_buffer* buffer) {
	buffer->ptr = buffer->init;
	buffer->size = BUFFER_SIZE;
	buffer->offset = 0;
}

static inline void 
wb_reservce(struct write_buffer* buffer, size_t len) {
	if (buffer->offset + len <= buffer->size) {
		return;
	}
	size_t nsize = buffer->size * 2;
	while (nsize < buffer->offset + len) {
		nsize = nsize * 2;
	}
	char* nptr = (char*)malloc(nsize);
	memcpy(nptr, buffer->ptr, buffer->size);
	buffer->size = nsize;

	if (buffer->ptr != buffer->init)
		free(buffer->ptr);
	buffer->ptr = nptr;
}

static inline void 
wb_addchar(struct write_buffer* buffer, char c) {
	wb_reservce(buffer, 1);
	buffer->ptr[buffer->offset++] = c;
}

static inline void 
wb_addlstring(struct write_buffer* buffer, const char* str,size_t len) {
	wb_reservce(buffer, len);
	memcpy(buffer->ptr + buffer->offset, str, len);
	buffer->offset += len;
}

static inline void 
wb_addstring(struct write_buffer* buffer, const char* str) {
	int len = strlen(str);
	wb_addlstring(buffer,str,len);
}

static inline void
wb_addnumber(struct write_buffer* buffer, double d) {
	char str[64] = {0};
	int str_len = conv_number(str,d);
	wb_addlstring(buffer, str, str_len);
}

static inline void 
wb_release(struct write_buffer* buffer) {
	if (buffer->ptr != buffer->init)
		free(buffer->ptr);
}

static inline char*
eat(struct data_collector* collector,int count) {
	assert(collector->total >= count);
	if (collector->size < count + 1) {
		size_t nsize = collector->size * 2;
		while(nsize < count + 1) {
			nsize = nsize * 2;
		}
		collector->reserve = realloc(collector->reserve,nsize);
		collector->size = nsize;
	}

	int offset = 0;
	int left = count;
	while (left > 0) {
		struct read_buffer* rb = collector->head;
		if (rb->offset + left < rb->size) {
			memcpy(collector->reserve+offset,rb->data+rb->offset,left);
			offset += left;

			rb->offset += left;
			left = 0;
		} else {
			memcpy(collector->reserve+offset,rb->data+rb->offset,rb->size - rb->offset);
			offset += rb->size - rb->offset;

			left -= (rb->size - rb->offset);
			free(rb->data);
			struct read_buffer* cur = collector->head;
			collector->head = collector->head->next;
			if (collector->head == NULL) {
				collector->head = collector->tail = NULL;
				assert(left == 0);
			}
			cur->next = collector->freelist;
			collector->freelist = cur;
		}
	}
	collector->total -= count;
	collector->reserve[count] = 0;
	return collector->reserve;
}

static inline int
check_eol(struct read_buffer* rb,int from,const char* sep,size_t sep_len) {
	while (rb) {
		int sz = rb->size - from;
		if (sz >= sep_len) {
			return memcmp(rb->data+from,sep,sep_len) == 0;
		}

		if (sz > 0) {
			if (memcmp(rb->data+from,sep,sz)) {
				return 0;
			}
		}
		rb = rb->next;
		sep += sz;
		sep_len -= sz;
		from = 0;
	}
	assert(0);
}

static inline size_t
search_eol(struct data_collector* collector,const char* sep,size_t sep_len) {
	if (!collector->cursor.rb) {
		collector->cursor.rb = collector->head;
		collector->cursor.offset = collector->head->offset;
		collector->cursor.total = 0;
	}
	struct read_buffer* current = collector->cursor.rb;
	int offset = collector->cursor.offset;
	while(current) {
		int i = offset;
		for(; i < current->size;i++) {
			if (collector->cursor.total + sep_len > collector->total) {
				collector->cursor.offset = i;
				return 0;
			}
			int ret = check_eol(current,i,sep,sep_len);
			if (ret == 1) {
				collector->cursor.rb = NULL;
				return collector->cursor.total + sep_len;
			}
			++collector->cursor.total;
		}
		current = current->next;
		offset = 0;
	}

	assert(0);
}

static inline char*
eat_until_eol(struct data_collector* collector,const char* sep,size_t sep_len) {
	if (collector->total == 0) {
		return NULL;
	}
	size_t offset = search_eol(collector,sep,sep_len);
	if (offset == 0) {
		return NULL;
	}
	char* line = eat(collector,offset);
	line[offset - 1] = 0;
	line[offset - 2] = 0;
	return line;
}

static inline int
eat_status(lua_State* L,struct data_collector* collector) {
	char* line = eat_until_eol(collector,"\r\n",2);
	if (!line) {
		return 0;
	}
	lua_pushstring(L,line);
	collector->phase = PHASE_BEGIN;
	return 1;
}

static inline int
eat_number(lua_State* L,struct data_collector* collector) {
	char* line = eat_until_eol(collector,"\r\n",2);
	if (!line) {
		return 0;
	}
	lua_Number integer = strtonumber(L,line);
	lua_pushinteger(L, integer);
	collector->phase = PHASE_BEGIN;
	return 1;
}

static inline int
eat_string(lua_State* L,struct data_collector* collector) {
	if (collector->reply.string.size == 0) {
		char* line = eat_until_eol(collector,"\r\n",2);
		if (!line) {
			return 0;
		}
		collector->reply.string.size = strtonumber(L,line);
	}
	
	if (collector->total < collector->reply.string.size + 2) {
		return 0;
	}
	char* str = eat(collector,collector->reply.string.size);
	lua_pushlstring(L,str,collector->reply.string.size);
	eat(collector,2);
	collector->phase = PHASE_BEGIN;
	return 1;
}

static inline int
eat_array(lua_State* L,struct data_collector* collector) {
	if (collector->reply.array.count == 0) {
		char* line = eat_until_eol(collector,"\r\n",2);
		if (!line) {
			return 0;
		}
		collector->reply.array.count = strtonumber(L,line);
	}
	
	lua_rawgeti(L, LUA_REGISTRYINDEX, collector->reply.array.ref);

	while(collector->reply.array.index < collector->reply.array.count) {
		short type = 0;
		if (collector->reply.array.reserve == 0) {
			char* line = eat_until_eol(collector,"\r\n",2);
			if (!line) {
				lua_pop(L,1);
				return 0;
			}
			switch(line[0]) {
				case '$':
				{
					type = 0;
					break;
				}
				case ':':
				{
					type = 1;
					break;
				}
				default:
					assert(0);
			}
			collector->reply.array.reserve = strtonumber(L,&line[1]);
		}

		if (type == 0) {
			if (collector->reply.array.reserve == -1) {
				lua_pushinteger(L,-1);
			} else {
				if (collector->total < collector->reply.array.reserve + 2) {
					lua_pop(L,1);
					return 0;
				}
				char* str = eat(collector,collector->reply.array.reserve);
				lua_pushlstring(L,str,collector->reply.array.reserve);
			}
			eat(collector,2);
		} else {
			lua_pushinteger(L,collector->reply.array.reserve);
		}
		
		
		lua_rawseti(L,-2,collector->reply.array.index + 1);

		collector->reply.array.index++;
		collector->reply.array.reserve = 0;
	}

	luaL_unref(L, LUA_REGISTRYINDEX, collector->reply.array.ref);
	collector->phase = PHASE_BEGIN;
	return 1;
}

int
_push(lua_State* L) {
	struct data_collector* collector = lua_touserdata(L,1);
	
	size_t size;
	const char* str = lua_tolstring(L,2,&size);

	collector->total += size;

	struct read_buffer* rb;
	if (collector->freelist) {
		rb = collector->freelist;
		collector->freelist = rb->next;
	} else {
		rb = malloc(sizeof(*rb));
	}
	memset(rb,0,sizeof(*rb));
	rb->data = strdup(str);
	rb->size = size;
	rb->offset = 0;

	if (collector->head == NULL) {
		assert(collector->tail == NULL);
		collector->head = collector->tail = rb;
	} else {
		collector->tail->next = rb;
		rb->prev = collector->tail;
		collector->tail = rb;
	}
	return 0;
}

int
_pop(lua_State* L) {
	struct data_collector* collector = lua_touserdata(L,1);
	for(;;) {
		switch(collector->phase) {
			case PHASE_BEGIN: {
				struct read_buffer* rb = collector->head;
				if (!rb)
					return 0;
				switch(rb->data[rb->offset]) {
					case '+':
					case '-': {
						collector->phase = PHASE_STATUS;
						break;
					}
					case ':': {
						collector->phase = PHASE_NUMBER;
						break;
					}
					case '$': {
						collector->phase = PHASE_STRING;
						collector->reply.string.size = 0;
						break;
					}
					case '*': {
						collector->phase = PHASE_ARRAY;
						collector->reply.array.count = 0;
						collector->reply.array.index = 0;
						collector->reply.array.reserve = 0;
						lua_newtable(L);
						collector->reply.array.ref = luaL_ref(L,LUA_REGISTRYINDEX);
						break;
					}
					default: {
						luaL_error(L,"error char:%c",rb->data[rb->offset]);
					}
				}
				eat(collector,1);
				break;
			}
			case PHASE_STATUS: {
				return eat_status(L,collector);
			}
			case PHASE_NUMBER: {
				return eat_number(L,collector);
			}
			case PHASE_STRING: {
				return eat_string(L,collector);
			}
			case PHASE_ARRAY: {
				return eat_array(L,collector);
			}
			default: {
				luaL_error(L,"unknown phase:%d",collector->phase);
			}
		}
	}
}

int
_release(lua_State* L) {
	struct data_collector* collector = lua_touserdata(L,1);
	free(collector->reserve);

	struct read_buffer* cursor = collector->head;
	while (cursor) {
		struct read_buffer* rb = cursor;
		cursor = cursor->next;
		free(rb->data);
		free(rb);
	}

	cursor = collector->freelist;
	while (cursor) {
		struct read_buffer* rb = cursor;
		cursor = cursor->next;
		free(rb);
	}

	return 0;
}

int
_create_collector(lua_State* L) {
	struct data_collector* collector = lua_newuserdata(L,sizeof(*collector));
	memset(collector,0,sizeof(*collector));
	collector->phase = PHASE_BEGIN;
	collector->reserve = malloc(64);
	collector->size = 64;
	if (luaL_newmetatable(L, "redis_meta")) {
		const luaL_Reg meta[] = {
			{ "push", _push },
			{ "pop", _pop },
			{ NULL, NULL },
		};
		luaL_newlib(L,meta);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, _release);
		lua_setfield(L, -2, "__gc");
	}
	
	lua_setmetatable(L, -2);
	return 1;
}

int
_cmd(lua_State* L) {
	struct write_buffer wb;
	wb_init(&wb);

	int top = lua_gettop(L);

	wb_addchar(&wb,'*');
	wb_addnumber(&wb,top);
	wb_addstring(&wb,"\r\n");

	int i=1;
	for(;i<= top;i++) {
		wb_addchar(&wb,'$');

		int type = lua_type(L, i);
		switch (type) {
			case LUA_TNUMBER:
			{
				char str[64] = {0};
				size_t len = conv_number(str,lua_tonumber(L,i));
				wb_addnumber(&wb,len);
				wb_addstring(&wb,"\r\n");
				wb_addlstring(&wb,str,len);
				break;
			}
			case LUA_TBOOLEAN:
			{
				wb_addchar(&wb,'1');
				wb_addstring(&wb,"\r\n");
				int val = lua_toboolean(L, i);
				if (val)
					wb_addnumber(&wb,1);
				else
					wb_addnumber(&wb,0);
				break;
			}
			case LUA_TSTRING:
			{
				size_t sz = 0;
				const char *str = lua_tolstring(L, i, &sz);
				wb_addnumber(&wb,sz);
				wb_addstring(&wb,"\r\n");
				wb_addlstring(&wb,str,sz);
				break;
			}
			default:
				luaL_error(L, "value not support type %s", lua_typename(L, type));
				break;
		}
		wb_addstring(&wb,"\r\n");
	}

	lua_pushlstring(L, wb.ptr, wb.offset);
	wb_release(&wb);
	return 1;
}

int
luaopen_redis_core(lua_State *L) {
	luaL_checkversion(L);
	luaL_Reg l[] ={
		{ "cmd", _cmd },
		{ "create_collector", _create_collector },
		{ NULL, NULL },
	};

	luaL_newlib(L,l);
	return 1;
}
