#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#include "http_parser.h"

struct lua_http_parser {
	struct http_parser parser;
	lua_State* L;
	int more;
};

#define META_PARSER "http_parser"

int
parser_message_begin(struct http_parser* parser) {
	return 0;
}

int
parser_message_complete(struct http_parser* parser) {
	struct lua_http_parser* lparser = (struct lua_http_parser*)parser;
	lparser->more = 0;
	return 0;
}

int
parser_headers_complete(struct http_parser* parser) {
	return 0;
}

int
parser_chunk_header(struct http_parser* parser) {
	return 0;
}

int
parser_chunk_complete(struct http_parser* parser) {
	return 0;
}

int
parser_url(struct http_parser* parser,const char* at,size_t length) {
	struct lua_http_parser* lparser = (struct lua_http_parser*)parser;
	lua_pushlstring(lparser->L,at,length);
	lua_setfield(lparser->L,2,"url");
	return 0;
}

int
parser_status(struct http_parser* parser,const char* at,size_t length) {
	struct lua_http_parser* lparser = (struct lua_http_parser*)parser;
	lua_pushlstring(lparser->L,at,length);
	lua_setfield(lparser->L,2,"status");
	return 0;
}

int
parser_header_field(struct http_parser* parser,const char* at,size_t length) {
	struct lua_http_parser* lparser = (struct lua_http_parser*)parser;
	lua_getfield(lparser->L,2,"header");
	lua_pushlstring(lparser->L,at,length);
	return 0;
}

int
parser_header_value(struct http_parser* parser,const char* at,size_t length) {
	struct lua_http_parser* lparser = (struct lua_http_parser*)parser;
	lua_pushlstring(lparser->L,at,length);
	lua_settable(lparser->L,-3);
	return 0;
}

int
parser_body(struct http_parser* parser,const char* at,size_t length) {
	struct lua_http_parser* lparser = (struct lua_http_parser*)parser;
	
	lua_getfield(lparser->L,2,"body");
	size_t size = lua_rawlen(lparser->L,-1);

	lua_pushlstring(lparser->L,at,length);
	lua_seti(lparser->L,-2,size + 1);
	
	lua_pop(lparser->L,1);
	return 0;
}


static const http_parser_settings settings = {
        parser_message_begin,
        parser_url,
        parser_status,
        parser_header_field,
        parser_header_value,
        parser_headers_complete,
        parser_body,
        parser_message_complete,
        parser_chunk_header,
        parser_chunk_complete
    };

int
meta_execute(lua_State* L) {
	struct lua_http_parser* lparser = (struct lua_http_parser*)lua_touserdata(L, 1);
	luaL_checktype(L,2,LUA_TTABLE);
	size_t length;
	const char* data = lua_tolstring(L,3,&length);

	lparser->L = L;
	lparser->more = 1;

	http_parser_execute(&lparser->parser,&settings,data,length);
	if (HTTP_PARSER_ERRNO(&lparser->parser) != HPE_OK) {
		lua_pushboolean(L,0);
		lua_pushstring(L,http_errno_name(HTTP_PARSER_ERRNO(&lparser->parser)));
		return 2;
	}

	if (!lparser->more) {
		lua_pushinteger(L,lparser->parser.upgrade);
		lua_setfield(L,2,"upgrade");


		lua_pushstring(L,http_method_str(lparser->parser.method));
		lua_setfield(L,2,"method");

		lua_createtable(L,0,2);
		lua_pushinteger(L,lparser->parser.http_major);
		lua_setfield(L,-2,"major");
		lua_pushinteger(L,lparser->parser.http_minor);
		lua_setfield(L,-2,"minor");
		lua_setfield(L,2,"version");

		lua_pushboolean(L, http_should_keep_alive(&lparser->parser));
		lua_setfield(L,2,"keepalive");
	}

	lua_pushboolean(L,1);
	lua_pushboolean(L,lparser->more);
	return 2;
}

int
parser_new(lua_State* L) {
	struct lua_http_parser* lparser = lua_newuserdata(L, sizeof(*lparser));
	int parser_type = lua_tointeger(L,1);
	switch(parser_type) {
		case 0:
		{
			http_parser_init(&lparser->parser,HTTP_REQUEST);
			break;
		}
		case 1:
		{
			http_parser_init(&lparser->parser,HTTP_RESPONSE);
			break;
		}
		default:
		{
			luaL_error(L,"unknown httpd parser type");
		}
	}

	luaL_newmetatable(L,META_PARSER);
 	lua_setmetatable(L, -2);
	
	return 1;
}

int
luaopen_http_parser(lua_State* L) {

	luaL_newmetatable(L, META_PARSER);
	const luaL_Reg meta_parser[] = {
		{ "execute", meta_execute },
		{ NULL, NULL },
	};
	luaL_newlib(L,meta_parser);
	lua_setfield(L, -2, "__index");
	lua_pop(L,1);

	const luaL_Reg l[] = {
		{ "new", parser_new },
		{ NULL, NULL },
	};
	luaL_newlib(L, l);
	return 1;
}
