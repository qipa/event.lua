
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <setjmp.h>

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"


#define TRY(l) if (setjmp((l)->exception) == 0)
#define THROW(l) longjmp((l)->exception, 1)

#define TYPE_INT				0
#define TYPE_FLOAT				1
#define TYPE_DOUBLE				2
#define TYPE_STRING				3
#define TYPE_PROTOCOL			4

#define MAX_DEPTH	32

static const char* BUILTIN_TYPE[] = { "int", "float", "double", "string"};

struct field {
	char* name;
	int type;
	int array;
	struct protocol* protocol;
};

struct protocol_table;

struct protocol {
	struct protocol* next;
	struct protocol* parent;
	struct protocol_table* children;

	char* file;
	char* name;

	struct field** field;
	int cap;
	int size;
};

struct protocol_table {
	struct protocol** slots;
	int size;
};

struct lexer {
	char* cursor;
	char* data;
	int line;

	char* file;

	struct lexer* parent;

	jmp_buf exception;
};

struct parser_context {
	struct protocol* root;
	lua_State* L;
	struct lexer** importer;
	int offset;
	int size;
	char* path;
	char* token;
	int token_size;
};

size_t 
strhash(const char *str) {
	size_t hash = 0;
	size_t ch;
	long i;
	for (i = 0; (ch = (size_t)*str++); i++) {
		if ((i & 1) == 0)
			hash ^= ((hash << 7) ^ ch ^ (hash >> 3));
		else
			hash ^= (~((hash << 11) ^ ch ^ (hash >> 5)));
	}
	return hash;
}

char* 
stralloc(struct parser_context* parser,const char* str,size_t size) {
	lua_getfield(parser->L, 1, str);
	if (!lua_isnil(parser->L,2)) {
		char* result = lua_touserdata(parser->L,-1);
		lua_pop(parser->L,1);
		return result;
	}
	lua_pop(parser->L,1);

	lua_pushlstring(parser->L,str,size);
	char* ptr = (char*)lua_tostring(parser->L,-1);
	lua_pushlightuserdata(parser->L,ptr);
	lua_settable(parser->L,1);
	return ptr;
}

struct protocol_table* 
create_table(int size) {
	struct protocol_table* table = (struct protocol_table*)malloc(sizeof(*table));
	table->size = size;

	table->slots = (struct protocol**)malloc(sizeof(*table->slots) * size);
	memset(table->slots, 0, sizeof(*table->slots) * size);

	return table;
}

void free_protocol(struct protocol* proto);

void
free_table(struct protocol_table* table) {
	int i;
	for (i = 0; i < table->size; i++) {
		struct protocol* cursor = table->slots[i];
		while (cursor) {
			struct protocol* proto = cursor;
			cursor = cursor->next;
			free_protocol(proto);
		}
	}
	free(table->slots);
	free(table);
}

struct protocol* 
query_protocol(struct protocol_table* table, const char* name) {
	size_t hash = strhash(name);
	int index = hash % table->size;
	struct protocol* slot = table->slots[index];
	if (slot != NULL) {
		while (slot) {
			if ((memcmp(slot->name, name,strlen(name)) == 0))
				return slot;
			slot = slot->next;
		}
	}
	return NULL;
}

void 
rehash_table(struct protocol_table* table, int nsize) {
	struct protocol** nslots = (struct protocol**)malloc(sizeof(*nslots) * nsize);
	memset(nslots, 0, sizeof(*nslots) * nsize);

	int i;
	for (i = 0; i < table->size; i++) {
		struct protocol* proto = table->slots[i];
		while (proto) {
			size_t hash = strhash(proto->name);
			int index = hash % nsize;
			struct protocol* slot = nslots[index];
			if (slot == NULL) {
				struct protocol* tmp = proto->next;
				proto->next = NULL;
				nslots[index] = proto;
				proto = tmp;
			} else {
				struct protocol* tmp = proto->next;
				proto->next = slot;
				nslots[index] = proto;
				proto = tmp;
				continue;
			}
		}
	}
	free(table->slots);
	table->slots = nslots;
	table->size = nsize;
}

void 
add_protocol(struct protocol_table* table, struct protocol* protocol) {
	size_t hash = strhash(protocol->name);
	int index = hash % table->size;
	struct protocol* slot = table->slots[index];
	if (slot == NULL)
		table->slots[index] = protocol;
	else {
		protocol->next = slot;
		table->slots[index] = protocol;
		int size = 0;
		while (protocol) {
			size++;
			protocol = protocol->next;
		}
		if (size >= (table->size / 8))
			rehash_table(table, table->size * 2);
	}
}

void 
add_field(struct protocol* protocol, struct field* f) {
	if (protocol->size == protocol->cap) {
		int ncap = protocol->cap * 2;
		struct field** nptr = (struct field**)malloc(sizeof(*nptr) * ncap);
		memset(nptr, 0, sizeof(*nptr) * ncap);
		memcpy(nptr, protocol->field, sizeof(*nptr) * protocol->cap);
		free(protocol->field);
		protocol->field = nptr;
		protocol->cap = ncap;
	}
	protocol->field[protocol->size++] = f;
}

struct field* 
create_field(struct protocol* proto,int array,int field_type,struct protocol* reference, char* field_name) {
	struct field* f = (struct field*)malloc(sizeof(*f));
	memset(f, 0, sizeof(*f));
	f->name = field_name;
	f->type = field_type;
	f->protocol = reference;
	f->array = array;

	add_field(proto, f);
	return f;
}

struct protocol* 
create_protocol(struct parser_context* parser, const char* file, const char* name) {
	struct protocol* proto = (struct protocol*)malloc(sizeof(*proto));
	proto->next = NULL;
	proto->name = stralloc(parser, name, strlen(name));
	proto->file = stralloc(parser, file, strlen(file));
	proto->parent = NULL;
	proto->children = create_table(4);
	proto->cap = 4;
	proto->size = 0;
	proto->field = (struct field**)malloc(sizeof(struct field*) * proto->cap);
	memset(proto->field, 0, sizeof(struct field*) * proto->cap);
	return proto;
}

void
free_protocol(struct protocol* proto) {
	free_table(proto->children);
	int i;
	for(i=0;i<proto->size;i++) {
		free(proto->field[i]);
	}
	free(proto->field);
	free(proto);
}

void
parser_init(struct parser_context* parser, const char* path) {
	parser->L = luaL_newstate();
	lua_settop(parser->L,0);
	lua_newtable(parser->L);

	parser->root = create_protocol(parser, "root", "root");
	
	parser->path = stralloc(parser, path, strlen(path));
	parser->token_size = 64;
	parser->token = malloc(parser->token_size);

	parser->offset = 0;
	parser->size = 4;
	parser->importer = malloc(sizeof(*parser->importer)* parser->size);
	memset(parser->importer, 0, sizeof(*parser->importer) * parser->size);
}

void
parser_release(struct parser_context* parser) {
	free_protocol(parser->root);
	lua_close(parser->L);
	free(parser->token);
	free(parser->importer);
}

void 
protocol_export(lua_State* L,struct protocol* root,int index,int depth) {
	depth++;

	int i;

	lua_newtable(L);

	struct protocol_table* table = root->children;
	lua_newtable(L);
	int has_children = 0;
	for (i = 0; i < table->size; i++) {
		struct protocol* proto = table->slots[i];
		while (proto) {
			has_children = 1;
			protocol_export(L,proto,lua_gettop(L),depth);
			lua_setfield(L,-2,proto->name);
			proto = proto->next;
		}
	}

	if (has_children)
		lua_setfield(L,-2,"children");
	else
		lua_pop(L,1);

	lua_pushstring(L,root->file);
	lua_setfield(L,-2,"file");

	lua_newtable(L);

	int has_field = 0;
	for (i = 0; i < root->size; ++i)
	{
		has_field = 1;
		struct field* f = root->field[i];
		
		lua_newtable(L);

		lua_pushinteger(L,f->type);
		lua_setfield(L,-2,"type");

		lua_pushboolean(L,f->array);
		lua_setfield(L,-2,"array");

		if (f->type == TYPE_PROTOCOL) {
			lua_pushstring(L,f->protocol->name);
		} else {
			lua_pushstring(L,BUILTIN_TYPE[f->type]);
		}
		lua_setfield(L,-2,"type_name");

		lua_pushstring(L,f->name);
		lua_setfield(L,-2,"name");

		lua_pushinteger(L,i);
		lua_setfield(L,-2,"index");

		lua_setfield(L,-2,f->name);
	}

	if (has_field) {
		lua_setfield(L,-2,"fields");
	} else {
		lua_pop(L,1);
	}
}

struct lexer* 
lexer_create(struct parser_context* parser,struct lexer* parent) {
	struct lexer* lex = malloc(sizeof(*lex));
	lex->parent = parent;
	lex->cursor = lex->data = lex->file = NULL;
	lex->line = 0;
	return lex;
}

void 
lexer_release(struct lexer* lex) {
	if (lex->data != NULL)
		free(lex->data);
	free(lex);
}

int
has_import(struct parser_context* parser,const char* file) {
	int i;
	for(i=0;i < parser->offset;i++) {
		struct lexer* lex = parser->importer[i];
		if (strncmp(lex->file, file, strlen(file)) == 0)
			return 1;
	}
	return 0;
}

void 
parse_end(struct parser_context* parser, struct lexer* lex) {
	if (parser->offset == parser->size) {
		int nsize = parser->size;
		struct lexer** nptr = (struct lexer**)malloc(sizeof(*nptr) * nsize);
		memset(nptr, 0, sizeof(*nptr)*nsize);
		memcpy(nptr, parser->importer, parser->size * sizeof(*nptr));
		free(parser->importer);
		parser->size = nsize;
		parser->importer = nptr;
	}
	parser->importer[parser->offset++] = lex;
}

static inline int 
eos(struct lexer *l, int n) {
	if (*(l->cursor + n) == 0)
		return 1;
	else
		return 0;
}

static inline void skip_space(struct lexer *l);

static inline void 
next_line(struct lexer *l) {
	char *n = l->cursor;
	while (*n != '\n' && *n)
		n++;
	if (*n == '\n')
		n++;
	l->line++;
	l->cursor = n;
	skip_space(l);
	return;
}

static inline void 
skip_space(struct lexer *l) {
	char *n = l->cursor;
	while (isspace(*n) && *n) {
		if (*n == '\n')
			l->line++;
		n++;
	}

	l->cursor = n;
	if (*n == '#' && *n)
		next_line(l);
	return;
}

static inline void 
skip(struct lexer* l, int size) {
	char *n = l->cursor;
	int index = 0;
	while (!eos(l,0) && index < size) {
		n++;
		index++;
	}
	l->cursor = n;
}

static inline int 
expect(struct lexer* l, char ch) {
	return *l->cursor == ch;
}

static inline int 
expect_space(struct lexer* l) {
	return isspace(*l->cursor);
}

static inline char*
next_token(struct parser_context* parser,struct lexer* l,size_t* size) {
	char ch = *l->cursor;
	int index = 0;
	while(ch != 0 && (ch == '_' || (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9'))) {
		if (index >= parser->token_size) {
			parser->token_size *= 2;
			if (index > parser->token_size)
				parser->token_size = index;
			parser->token = realloc(parser->token,parser->token_size);
		}
		parser->token[index] = ch;
		index++;
		++l->cursor;
		ch = *l->cursor;
	}
	skip_space(l);
	*size = index;
	parser->token[index] = '\0';
	return parser->token;
}

void 
lexer_execute(struct parser_context* parser,struct lexer* l, struct protocol* parent);

int 
parse_begin(struct parser_context* parser, struct lexer* lex, const char* file) {
	char fullname[64];
	memset(fullname, 0, 64);
	snprintf(fullname, 64, "%s%s", parser->path, file);

	FILE* F = fopen(fullname, "r");
	if (F == NULL) {
		if (lex->parent)
			fprintf(stderr, "%s@line:%d syntax error:no such file:%s\n", lex->parent->file, lex->parent->line, file);
		else
			fprintf(stderr, "no such file:%s\n", file);
		return -1;
	}
	fseek(F, 0, SEEK_END);
	int len = ftell(F);
	lex->cursor = (char*)malloc(len + 1);
	memset(lex->cursor, 0, len + 1);
	rewind(F);
	fread(lex->cursor, 1, len, F);
	lex->cursor[len] = 0;
	fclose(F);

	lex->data = lex->cursor;
	lex->line = 1;
	lex->file = stralloc(parser,file,strlen(file));

	TRY(lex) {
		skip_space(lex);
		while (!eos(lex, 0))
			lexer_execute(parser, lex, parser->root);
		parse_end(parser, lex);
		return 0;
	}
	return -1;
}

void 
protocol_import(struct parser_context* parser, struct lexer* parent, char* name) {
	char file[64];
	memset(file, 0, 64);
	snprintf(file, 64, "%s.protocol", name);

	struct lexer* lex = lexer_create(parser, parent);
	if (parse_begin(parser, lex, file) < 0)
		THROW(parent);
}

void 
protocol_parse(struct parser_context* parser, struct lexer* lex, struct protocol* parent) {
	size_t len = 0;
	char* name = next_token(parser, lex, &len);

	if (!expect(lex,'{')) {
		fprintf(stderr, "%s@line:%d syntax error:protocol must start with {\n", lex->file, lex->line);
		THROW(lex);
	}

	struct protocol* oproto = query_protocol(parent->children,name);
	if (oproto) {
		fprintf(stderr, "%s@line:%d syntax error:protocol name:%s already define in file:%s\n", lex->file, lex->line, name,oproto->file);
		THROW(lex);
	}

	struct protocol* proto = create_protocol(parser, lex->file, name);
	proto->parent = parent;
	add_protocol(parent->children, proto);

	//跳过{
	skip(lex, 1);
	skip_space(lex);
	while (!expect(lex, '}')) {
		name = next_token(parser, lex, &len);
		if (len == 0) {
			fprintf(stderr, "%s@line:%d syntax error\n", lex->file, lex->line);
			THROW(lex);
		}

		if (strncmp(name, "protocol", len) == 0) {
			protocol_parse(parser, lex, proto);
			continue;
		}

		int field_type = TYPE_PROTOCOL;
		int i;
		for (i = 0; i < sizeof(BUILTIN_TYPE) / sizeof(void*); i++) {
			if (strncmp(name, BUILTIN_TYPE[i], len) == 0) {
				field_type = i;
				break;
			}
		}

		int isarray = 0;
		if (strncmp(lex->cursor,"[]",2) == 0) {
			isarray = 1;
			skip(lex, 2);
			if (!expect_space(lex)) {
				fprintf(stderr, "%s@line:%d syntax error,expect space\n", lex->file, lex->line);
				THROW(lex);
			}
			skip_space(lex);
		}

		struct protocol* ref_proto = NULL;
		//不是内置类型，必然是protocol
		if (field_type == TYPE_PROTOCOL) {
			struct protocol* cursor = proto;
			while (cursor) {
				ref_proto = query_protocol(cursor->children, name);
				if (ref_proto != NULL)
					break;
				cursor = cursor->parent;
			}
			if (ref_proto == NULL) {
				fprintf(stderr, "%s@line:%d syntax error:unknown type:%s\n", lex->file, lex->line, name);
				THROW(lex);
			}
		}
		
		name = next_token(parser, lex, &len);
		if (len == 0) {
			fprintf(stderr, "%s@line:%d syntax error\n", lex->file, lex->line);
			THROW(lex);
		}

		create_field(proto,isarray,field_type,ref_proto,stralloc(parser, name, len+1));
	}
	skip(lex, 1);
	skip_space(lex);
}

void 
lexer_execute(struct parser_context* parser,struct lexer* lex, struct protocol* parent) {
	size_t len = 0;
	char* name = next_token(parser, lex, &len);
	if (len == 0) {
		fprintf(stderr, "%s@line:%d syntax error\n", lex->file, lex->line);
		THROW(lex);
	}

	if (strncmp(name, "protocol", len) == 0) {
		return protocol_parse(parser, lex, parent);
	
	} else if (strncmp(name, "import", len) == 0) {
		if (!expect(lex, '\"')) {
			fprintf(stderr, "%s@line:%d syntax error\n", lex->file, lex->line);
			THROW(lex);
		}

		skip(lex,1);
		name = next_token(parser, lex, &len);
		if (len == 0) {
			fprintf(stderr, "%s@line:%d syntax error\n", lex->file, lex->line);
			THROW(lex);
		}
		skip(lex,1);

		if (has_import(parser, name) == 0)
			protocol_import(parser, lex, name);

		skip_space(lex);
		return;
	}
	fprintf(stderr, "%s@line:%d syntax error:unknown %s\n", lex->file, lex->line, name);
	THROW(lex);
}


static int 
_parse(lua_State* L) {
	const char* path = luaL_checkstring(L,1);
	const char* file = luaL_checkstring(L,2);

	struct parser_context parser;
	parser_init(&parser, path);

	struct lexer* lex = lexer_create(&parser, NULL);

	if (parse_begin(&parser, lex, file) < 0) {
		parser_release(&parser);
		lexer_release(lex);
		luaL_error(L,"parse error");
	}

	lua_pop(L,2);
	lua_newtable(L);
	int depth = 0;
	luaL_checkstack(L, MAX_DEPTH*2 + 8, NULL);
	protocol_export(L,parser.root,lua_gettop(L),depth);
	lua_setfield(L,1,"root");

	lexer_release(lex);
	parser_release(&parser);
	return 1;
}

int
luaopen_protocolparser(lua_State* L){
	luaL_Reg l[] = {
		{ "parse", _parse },
		{ NULL, NULL },
	};
	luaL_checkversion(L);
	luaL_newlib(L,l);
	return 1;
}