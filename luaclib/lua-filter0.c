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

#define PHASE_SEARCH 0
#define PHASE_MATCH 1

struct word_tree;

KHASH_MAP_INIT_STR(word,struct word_tree*);

struct word_tree {
	khash_t(word)* hash;
	uint8_t tail;
};

static inline int
split_utf8(const char* word,size_t size,int offset) {
	int i;
	for(i = offset;i < size;i++) {
		char ch = word[i];
		if((ch & 0x80) == 0) {  
            return 1;
        } else if((ch & 0xF8) == 0xF8) {  
            return 5;  
        } else if((ch & 0xF0) == 0xF0) {  
            return 4;  
        } else if((ch & 0xE0) == 0xE0) {  
            return 3;  
        } else if((ch & 0xC0) == 0xC0) {  
            return 2;  
        }  
	}
	assert(0);
}

void
word_add(struct word_tree* root_tree, const char* word,size_t size) {
	struct word_tree* tree = root_tree;
	int i;
	for(i = 0;i < size;) {
		char ch[8] = {0};
		int length = split_utf8(word,size,i);
		memcpy(ch,&word[i],length);
		i += length;

		khiter_t k = kh_get(word, tree->hash, ch);
		int miss = (k == kh_end(tree->hash));
		if (miss) {
			struct word_tree* next_tree = NULL;
			int result;
			khiter_t k = kh_put(word, tree->hash, strdup(ch), &result);
			if (result == 1 || result == 2) {
				next_tree = malloc(sizeof(*next_tree));
				next_tree->tail = 0;
				next_tree->hash = kh_init(word);
				kh_value(tree->hash, k) = next_tree;
			}else {
				assert(0);
			}
			
			if (i == size)
				next_tree->tail = 1;
			tree = next_tree;
		} else {
			tree = kh_value(tree->hash, k);
			if (i == size)
				tree->tail = 1;
		}
	}
}

char*
word_filter(struct word_tree* root_tree,const char* source,size_t size,int replace,size_t* replace_offset) {
	
	char* dest = NULL;
	int dest_offset = 0;
	if (replace)
		dest = strdup(source);

	struct word_tree* tree = root_tree;

	int filter_start = 0;
	int filter_len = -1;
	int founded = 0;

	int phase = PHASE_SEARCH;

	int i;
	for(i = 0;i < size;) {
		char word[8] = {0};
		
		int length = split_utf8(source,size,i);
		memcpy(word,&source[i],length);
		i += length;

		switch(phase) {
			case PHASE_SEARCH: {
				khiter_t k = kh_get(word, tree->hash, word);
				if (k != kh_end(tree->hash)) {
					tree = kh_value(tree->hash, k);
					phase = PHASE_MATCH;
					filter_start = i - length;
					filter_len = 1;
					founded = 0;
					if (tree->tail) {
						founded = 1;
					}
				} else {
					if (replace) {
						memcpy(dest + dest_offset, word, length);
						dest_offset += length;
					}
				}
				break;
			}
			case PHASE_MATCH: {
				if (length == 1) {
					if (isspace(word[0]) || iscntrl(word[0]) || ispunct(word[0])) {
						++filter_len;
						continue;
					}
				}
				khiter_t k = kh_get(word, tree->hash, word);
				if (k != kh_end(tree->hash)) {
					tree = kh_value(tree->hash, k);
					++filter_len;
					if (tree->tail) {
						founded = 1;
					}
				} else {
					//回滚一个word
					i -= length;
					if (founded == 1) {
						if (replace) {
							//匹配成功
							memset(dest + dest_offset, '*', filter_len);
							dest_offset += filter_len;
						} else {
							return NULL;
						}
					} else {
						//匹配失败
						if (replace) {
							memcpy(dest + dest_offset, source + filter_start, i - filter_start);
							dest_offset += i - filter_start;
						}
					}
					
					tree = root_tree;
					phase = PHASE_SEARCH;
				}
				break;
			}
		}
	}

	if (!replace)
		return (char*)source;

	if (founded == 1) {
		memset(dest + dest_offset, '*', filter_len);
		dest_offset += filter_len;
	} else {
		memcpy(dest + dest_offset, source + filter_start, i - filter_start);
		dest_offset += i - filter_start;
	}

	*replace_offset = dest_offset;

	return dest;
}

static int
lcreate(lua_State* L) {
	struct word_tree* tree = lua_newuserdata(L,sizeof(*tree));
	tree->hash = kh_init(word);
	luaL_newmetatable(L,"meta_filter");
 	lua_setmetatable(L, -2);
	return 1;
}

void
release(char* word,struct word_tree* tree) {
	if (tree->hash) {
		struct word_tree* child = NULL;
		const char* word = NULL;
		kh_foreach(tree->hash, word, child, release((char*)word, child));
		kh_destroy(word,tree->hash);
	}
	free(tree);
	free(word);
}

static int
lrelease(lua_State* L) {
	struct word_tree* tree = lua_touserdata(L,1);

	struct word_tree* child = NULL;
	const char* word = NULL;
	kh_foreach(tree->hash, word, child, release((char*)word, child));
	kh_destroy(word,tree->hash);
	
	return 0;
}

static int
ladd(lua_State* L) {
	struct word_tree* tree = lua_touserdata(L,1);
	size_t size;
	const char* word = lua_tolstring(L,2,&size);
	word_add(tree,word,size);
	return 0;
}

static int
lfilter(lua_State* L) {
	struct word_tree* tree = lua_touserdata(L,1);
	size_t size;
	const char* word = lua_tolstring(L,2,&size);
	int replace = luaL_optinteger(L,3,1);

	size_t replace_offset = 0;
	char* dest = word_filter(tree,word,size,replace,&replace_offset); 
	if (!replace) {
		if (!dest)
			lua_pushboolean(L,0);
		else
			lua_pushboolean(L,1);
		return 1;
	}
	lua_pushlstring(L,dest,replace_offset);
	free(dest);
	return 1;
}

void
dump(const char* word,struct word_tree* tree,int depth) {
	int i;
	for(i=0;i<depth;i++)
		printf("  ");
	printf("%s\n",word);
	depth++;
	if (tree->hash) {
		struct word_tree* child = NULL;
		const char* word = NULL;
		kh_foreach(tree->hash, word, child, dump((char*)word, child,depth));
	}
}

static int
ldump(lua_State* L) {
	struct word_tree* tree = lua_touserdata(L,1);
	struct word_tree* child = NULL;
	const char* word = NULL;
	int depth = 0;
	kh_foreach(tree->hash, word, child, dump((char*)word, child, depth));
	return 0;
}

int 
luaopen_filter0_core(lua_State *L) {
	luaL_checkversion(L);

	luaL_newmetatable(L, "meta_filter");
	const luaL_Reg meta[] = {
		{ "add", ladd },
		{ "filter", lfilter },
		{ "dump", ldump },
		{ NULL, NULL },
	};
	luaL_newlib(L,meta);
	lua_setfield(L, -2, "__index");

	lua_pushcfunction(L,lrelease);
	lua_setfield(L, -2, "__gc");
	lua_pop(L,1);


	const luaL_Reg l[] = {
		{ "create", lcreate },
		{ NULL, NULL },
	};
	luaL_newlib(L, l);
	return 1;
}
