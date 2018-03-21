#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#include "ikcp.h"

#define META_IKCP   "meta_ikcp"
#define RECV_BUFFER 1024

struct lua_ikcp {
	lua_State* L;
	ikcpcb* kcp;
	int ref;
};

int 
udp_output(const char *buf, int len, ikcpcb *kcp, void *ud) {
	struct lua_ikcp* likcp = ud;
	lua_pushvalue(likcp->L,-1);
	lua_rawgeti(likcp->L, LUA_REGISTRYINDEX, likcp->ref);
	lua_pushlstring(likcp->L,buf,len);
	lua_pcall(likcp->L, 2, 0, 0);
	return 0;
}

static int
likcp_wndsize(lua_State* L) {
	struct lua_ikcp* likcp = (struct lua_ikcp*)lua_touserdata(L, 1);
	int sndwnd = lua_tointeger(L,1);
	int rcvwnd = lua_tointeger(L,2);
	ikcp_wndsize(likcp->kcp, sndwnd, rcvwnd);
	return 0;
}

static int
likcp_nodelay(lua_State* L) {
	struct lua_ikcp* likcp = (struct lua_ikcp*)lua_touserdata(L, 1);
	int nodelay = luaL_checkinteger(L,2);
	int interval = luaL_checkinteger(L,3);
	int resend = luaL_checkinteger(L,4);
	int nc = luaL_checkinteger(L,5);

	ikcp_nodelay(likcp->kcp, nodelay, interval, resend, nc);
	return 0;
}

static int
likcp_release(lua_State* L) {
	struct lua_ikcp* likcp = (struct lua_ikcp*)lua_touserdata(L, 1);
	ikcp_release(likcp->kcp);
	luaL_unref(L, LUA_REGISTRYINDEX, likcp->ref);
	return 0;
}


static int
likcp_update(lua_State* L) {
	struct lua_ikcp* likcp = (struct lua_ikcp*)lua_touserdata(L, 1);
	int now = lua_tointeger(L,2);
	luaL_checktype(L,3,LUA_TFUNCTION);
	ikcp_update(likcp->kcp,now);
	return 0;
}

static int
likcp_input(lua_State* L) {
	struct lua_ikcp* likcp = (struct lua_ikcp*)lua_touserdata(L, 1);
	size_t size;
	const char* data = lua_tolstring(L,2,&size);
	ikcp_input(likcp->kcp,data,size);
	return 0;
}

static int
likcp_send(lua_State* L) {
	struct lua_ikcp* likcp = (struct lua_ikcp*)lua_touserdata(L, 1);
	size_t size;
	const char* data = lua_tolstring(L,2,&size);
	ikcp_send(likcp->kcp,data,size);
	return 0;
}

static int
likcp_recv(lua_State* L) {
	struct lua_ikcp* likcp = (struct lua_ikcp*)lua_touserdata(L, 1);
	size_t size = luaL_checkinteger(L,2);

	char stack_buffer[RECV_BUFFER] = {0};
	char* buffer = stack_buffer;
	if (size > RECV_BUFFER) {
		buffer = malloc(size);
	}

	int sz = ikcp_recv(likcp->kcp,buffer,size);
	if (sz < 0) {
		if (buffer != stack_buffer)
			free(buffer);
		return 0;
	}

	lua_pushlstring(L,buffer,sz);
	if (buffer != stack_buffer)
		free(buffer);
	return 1;
}

static int
likcp_new(lua_State* L) {
	int conv = lua_tointeger(L,1);

	struct lua_ikcp* likcp = lua_newuserdata(L, sizeof(struct lua_ikcp));
	likcp->L = L;
	likcp->kcp = ikcp_create(conv,likcp);
	likcp->kcp->output = udp_output;

	luaL_newmetatable(L,META_IKCP);
 	lua_setmetatable(L, -2);
	lua_pushvalue(L, -1);
	likcp->ref = luaL_ref(L, LUA_REGISTRYINDEX);

	return 1;
}

int
luaopen_ikcp_core(lua_State* L) {
	luaL_newmetatable(L, META_IKCP);
	const luaL_Reg meta_ikcp[] = {
		{ "update", likcp_update },
		{ "input", likcp_input },
		{ "send", likcp_send },
		{ "recv", likcp_recv },
		{ "nodelay", likcp_nodelay },
		{ "wndsize", likcp_wndsize },
		{ "release", likcp_release },
		{ NULL, NULL },
	};
	luaL_newlib(L,meta_ikcp);
	lua_setfield(L, -2, "__index");
	lua_pop(L,1);

	const luaL_Reg l[] = {
		{ "new", likcp_new },
		{ NULL, NULL },
	};
	luaL_newlib(L, l);
	return 1;
}
