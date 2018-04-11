#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#include <mysql/mysql.h>


struct lmysql {
	MYSQL conn;
	int ref;
};

static void
pushvalue (lua_State* L,void* data,int length,enum enum_field_types type) {

	switch (type) {
		case MYSQL_TYPE_VAR_STRING: 
		case MYSQL_TYPE_STRING:
		{
			lua_pushlstring(L,(char*)data,strlen(data));
			break;
		}
		
		case MYSQL_TYPE_SHORT: 
		case MYSQL_TYPE_LONG:
		case MYSQL_TYPE_LONGLONG:
		case MYSQL_TYPE_INT24: 
		case MYSQL_TYPE_YEAR: 
		case MYSQL_TYPE_TINY: 
		{
			lua_pushinteger(L,atoi(data));
			break;
		}

		case MYSQL_TYPE_DECIMAL: 
		case MYSQL_TYPE_FLOAT: 
		case MYSQL_TYPE_DOUBLE:
		{
			lua_pushinteger(L,atoi((const char*)atof));
			break;
		}

		case MYSQL_TYPE_TINY_BLOB: 
		case MYSQL_TYPE_MEDIUM_BLOB:
		case MYSQL_TYPE_LONG_BLOB: 
		case MYSQL_TYPE_BLOB:
		{
			lua_pushlstring(L,(char*)data,length);
			break;
		}
		case MYSQL_TYPE_DATE: 
		case MYSQL_TYPE_NEWDATE:
		case MYSQL_TYPE_DATETIME:
		case MYSQL_TYPE_TIME:
		case MYSQL_TYPE_TIMESTAMP:
		case MYSQL_TYPE_ENUM: 
		case MYSQL_TYPE_SET:
		{
			lua_pushlstring(L,data,strlen(data));
			break;
		}
		case MYSQL_TYPE_NULL:
		{
			lua_pushnil(L);
			break;
		}
		default:
		{
			lua_pushstring(L,"undefined");
			break;
		}
			
	}
}

static int
lrelease(lua_State* L) {
	struct lmysql* mysql = lua_touserdata(L,1);
	mysql_close(&mysql->conn);
	luaL_unref(L, LUA_REGISTRYINDEX, mysql->ref);
	return 0;
}

static int
lexecute(lua_State* L) {
	struct lmysql* mysql = lua_touserdata(L,1);

	size_t size;
	const char* sql = luaL_checklstring(L,2,&size);
	if (mysql_real_query(&mysql->conn,sql,size)){
		lua_pushboolean(L,0);
		lua_pushstring(L,mysql_error(&mysql->conn));
		return 2;
	}

	MYSQL_RES* res = mysql_store_result(&mysql->conn);
	if (!res) {
		if (mysql_field_count(&mysql->conn) == 0) {
			lua_pushboolean(L,1);
			return 1;
		}
		lua_pushboolean(L,0);
		lua_pushstring(L,mysql_error(&mysql->conn));
		return 2;
	}

	MYSQL_FIELD *fields = mysql_fetch_fields(res);
	int nrows = mysql_num_rows(res);
	int ncols = mysql_field_count(&mysql->conn);
	lua_createtable(L,nrows,0);

	int r;
	int c;
	for(r = 0;r < nrows;r++) {
		lua_createtable(L,0,ncols);
		MYSQL_ROW row = mysql_fetch_row(res);
		unsigned long* lengths = mysql_fetch_lengths(res);
		for(c = 0;c < ncols;c++) {
			lua_pushlstring(L,fields[c].name,fields[c].name_length);
			pushvalue(L,row[c],lengths[c],fields[c].type);
			lua_settable(L,-3);
		}
		lua_seti(L,-2,r + 1);
	}
	return 1;
}

static int
lconnect(lua_State* L) {
	const char* host = luaL_checkstring(L,1);
	const char* user = luaL_checkstring(L,2);
	const char* passwd = luaL_checkstring(L,3);
	const char* db = luaL_checkstring(L,4);
	int port = luaL_checkinteger(L,5);

	const char* unix_socket = NULL;
	if (!lua_isnoneornil(L,6))
		unix_socket = luaL_checkstring(L,6);

	int client_flag = 0;
	if (!lua_isnoneornil(L,7))
		client_flag = luaL_checkinteger(L,7);

	struct lmysql* mysql = lua_newuserdata(L,sizeof(*mysql));

	mysql_init(&mysql->conn);
	if (!mysql_real_connect(&mysql->conn,host,user,passwd,db,port,unix_socket,client_flag)) {
		lua_pushboolean(L,0);
		lua_pushstring(L,mysql_error(&mysql->conn));
		return 2;
	}

	lua_pushvalue(L,-1);
	mysql->ref = luaL_ref(L, LUA_REGISTRYINDEX);

	luaL_newmetatable(L,"meta_mysql");
 	lua_setmetatable(L, -2);

 	return 1;
}

int
luaopen_mysql_core(lua_State* L) {
	luaL_newmetatable(L, "meta_mysql");
	const luaL_Reg meta_mysql[] = {
		{ "execute", lexecute },
		{ "release", lrelease },
		{ NULL, NULL },
	};
	luaL_newlib(L,meta_mysql);
	lua_setfield(L, -2, "__index");
	lua_pop(L,1);

	const luaL_Reg l[] = {
		{ "connect", lconnect },
		{ NULL, NULL },
	};
	luaL_newlib(L, l);
	return 1;
}
