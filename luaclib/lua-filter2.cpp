#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <stdint.h>

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}



#include "WordFilterUtil.h"

int create(lua_State* L) 
{
	WordFilterUtil* filter = new WordFilterUtil();
	lua_pushlightuserdata(L, filter);
	return 1;
}

int add_keyword(lua_State* L)
{
	WordFilterUtil* filter = (WordFilterUtil*)lua_touserdata(L,1);
	size_t size;
	const char* word = lua_tolstring(L,2,&size);
	filter->addKeywords(word);
	return 0;
}

int word_filter(lua_State* L)
{
	WordFilterUtil* filter = (WordFilterUtil*)lua_touserdata(L,1);
	size_t size;
	const char* word = lua_tolstring(L,2,&size);

	int replaceCount = 0;
	std::string str = filter->filter(word,replaceCount);

	lua_pushlstring(L, str.c_str(),str.size());
	return 1;
}

extern "C" int 
luaopen_filter2_core(lua_State *L) {
	luaL_checkversion(L);

	const luaL_Reg l[] = {
		{ "create", create },
		{ "add", add_keyword },
		{ "filter", word_filter },
		{ NULL, NULL },
	};
	luaL_newlib(L, l);
	return 1;
}