#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
#include <math.h>

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "lstate.h"

#include "ev.h"
#include "socket_event.h"
#include "socket_util.h"
#include "object_container.h"
#include "common.h"

#define CACHED_SIZE 		1024 * 1024
#define MAX_FREQ 			100
#define ALIVE_TIME 			60
#define WARN_OUTPUT_FLOW 	10 * 1024
#define MAX_SEQMENT 		64 * 1024

typedef void (*accept_callback)(void* ud,int id,const char* addr);
typedef void (*close_callback)(void* ud,int id);
typedef void (*data_callback)(void* ud,int client_id,int message_id,void* data,size_t size);

__thread uint8_t CACHED_BUFFER[CACHED_SIZE];

struct client_manager {
	struct ev_loop_ctx* loop_ctx;
	struct ev_listener* listener;
	struct object_container* container;
	size_t max_client;
	size_t max_offset;
	uint32_t index;
	accept_callback accept_func;
	close_callback close_func;
	data_callback data_func;
	void* ud;
};

struct ev_client {
	struct client_manager* manager;
	struct ev_session* session;
	struct ev_timer timer;
	uint64_t id;
	int need;
	int freq;
	int execute;
	int mark;
	uint8_t seed;
	uint16_t order;
	double tick;
};

static inline struct ev_client*
get_client(struct client_manager* manager,uint64_t id) {
	uint64_t slot = id - (id / manager->max_offset) * manager->max_offset;
	struct ev_client* client = container_get(manager->container,slot);
	if (!client) {
		return NULL;
	}
	if (client->id != id) {
		return NULL;
	}
	return client;
}

static void 
close_client(int id,void* data) {
	struct ev_client* client = data;
	ev_session_free(client->session);
	ev_timer_stop(loop_ctx_get(client->manager->loop_ctx),(struct ev_timer*)&client->timer);
	uint64_t slot = client->id - (client->id / client->manager->max_offset) * client->manager->max_offset;
	container_remove(client->manager->container,slot);
	free(client);
}

static void
error_happen(struct ev_session* session,void* ud) {
	struct ev_client* client = ud;
	int id = client->id;
	struct client_manager* manager = client->manager;
	close_client(0,client);
	manager->close_func(manager->ud,id);
}

static void
read_complete(struct ev_session* ev_session, void* ud) {
	struct ev_client* client = ud;
	client->execute = 1;
	while (client->mark == 0) {
		size_t total = ev_session_input_size(ev_session);
		if (client->need == 0) {
			if (total >= 2) {
				uint8_t header[2];
				ev_session_read(ev_session,(char*)header,2);
				client->need = header[0] | header[1] << 8;
				client->need -= 2;
				assert(client->need > 0);
				if (client->need > MAX_SEQMENT) {
					error_happen(ev_session, client);
					return;
				}
			} else {
				break;
			}
		}

		total = ev_session_input_size(ev_session);
		if (client->need > 0) {
			if (total >= client->need) {
				uint8_t* data = CACHED_BUFFER;
				if (client->need > CACHED_SIZE) {
					data = malloc(client->need);
				}
				ev_session_read(ev_session,(char*)data,client->need);
				
				int i;
			    for (i = 0; i < client->need; ++i) {
			        data[i] = data[i] ^ client->seed;
			        client->seed += data[i];
			    }

			    uint16_t sum = checksum((uint16_t*)data,client->need);
			    uint16_t order = data[2] | data[3] << 8;
			    uint16_t id = data[4] | data[5] << 8;

			    if (sum != 0 || order != client->order) {
			    	error_happen(ev_session, client);
				    if (data != CACHED_BUFFER)
				    	free(data);
					return;
			    } else {
			    	client->order++;
			    }
			    client->freq++;
			    client->tick = loop_ctx_now(client->manager->loop_ctx);
			    client->manager->data_func(client->manager->ud,client->id,id,&data[6],client->need - 6);

			    if (data != CACHED_BUFFER)
			    	free(data);

			    client->need = 0;
			} else {
				break;
			}
		}
	}
	if (client->mark) {
		close_client(0,client);
	}
	client->execute = 0;
}	

static void
timeout(struct ev_loop* loop,struct ev_timer* io,int revents) {
	struct ev_client* client = io->data;
	if (client->freq > MAX_FREQ) {
		error_happen(NULL, client);
	}
	client->freq = 0;
	if (client->tick != 0 && loop_ctx_now(client->manager->loop_ctx) - client->tick > ALIVE_TIME) {
		error_happen(NULL, client);
	}
}

static void 
accept_client(struct ev_listener *listener, int fd, const char* addr, void *ud) {
	struct client_manager* manager = ud;

	socket_nonblock(fd);
	socket_keep_alive(fd);
	socket_closeonexec(fd);

	struct ev_client* client = malloc(sizeof(*client));
	memset(client,0,sizeof(*client));

	struct ev_session* session = ev_session_bind(manager->loop_ctx,fd);
	int slot = container_add(manager->container,client);
	if (slot == -1) {
		ev_session_free(session);
		free(client);
		return;
	}
	
	uint32_t index = manager->index++;

	client->manager = manager;
	client->session = session;
	client->id = index * manager->max_offset + slot;

	ev_session_setcb(client->session,read_complete,NULL,error_happen,client);
	ev_session_enable(client->session,EV_READ);

	client->timer.data = client;
	ev_timer_init(&client->timer,timeout,1,1);
	ev_timer_start(loop_ctx_get(manager->loop_ctx),&client->timer);

	manager->accept_func(manager->ud,client->id,addr);
}

static void
close_complete(struct ev_session* ev_session, void* ud) {
	struct ev_client* client = ud;
	close_client(0,client);
}

struct client_manager*
client_manager_create(struct ev_loop_ctx* loop_ctx,size_t max,void* ud) {
	struct client_manager* manager = malloc(sizeof(*manager));
	memset(manager,0,sizeof(*manager));
	manager->container = container_create(max);
	manager->loop_ctx = loop_ctx;
	manager->ud = ud;
	manager->max_client = max;
	manager->max_offset = 0;
	manager->index = 1;

	int offset = 0;
	for (; max > 0; offset++)
        max /= 10;
    manager->max_offset = pow(10,offset);
	
	return manager;
}

int
client_manager_start(struct client_manager* manager,const char* ip,int port) {
	struct sockaddr_in si;
	si.sin_family = AF_INET;
	si.sin_addr.s_addr = inet_addr(ip);
	si.sin_port = htons(port);

	int flag = SOCKET_OPT_NOBLOCK | SOCKET_OPT_CLOSE_ON_EXEC | SOCKET_OPT_REUSEABLE_ADDR;
	manager->listener = ev_listener_bind(manager->loop_ctx,(struct sockaddr*)&si,sizeof(si),16,flag,accept_client,manager);
	if (!manager->listener) {
		return -1;
	}
	if (port == 0) {
		char addr[INET6_ADDRSTRLEN] = {0};
		if (ev_listener_addr(manager->listener,addr,INET6_ADDRSTRLEN,&port) < 0) {
			return port;
		}
	}
	return port;
} 

void
client_manager_callback(struct client_manager* manager,accept_callback accept,close_callback close,data_callback data) {
	manager->accept_func = accept;
	manager->close_func = close;
	manager->data_func = data;
}

int
client_manager_stop(struct client_manager* manager) {
	if (manager->listener) {
		ev_listener_free(manager->listener);
		manager->listener = NULL;
		return 0;
	}
	return -1;
}

int
client_manager_close(struct client_manager* manager,int client_id,int grace) {
	struct ev_client* client = get_client(manager,client_id);
	if (!client) {
		return -1;
	}
	if (!grace) {
		if (client->execute) {
			client->mark = 1;
		} else {
			close_client(0,client);
		}
	}
	else {
		ev_session_setcb(client->session, NULL, close_complete, error_happen, client);
		ev_session_disable(client->session,EV_READ);
		ev_session_enable(client->session, EV_WRITE);
	}
	return 0;
}

void
client_manager_send(struct client_manager* manager,int client_id,int message_id,void* data,size_t size) {
	struct ev_client* client = get_client(manager,client_id);

	size_t total = size + sizeof(short) * 2;

    uint8_t* mb = malloc(total);
    memset(mb,0,total);
    memcpy(mb,&total,2);
    memcpy(mb+2,&message_id,2);
    memcpy(mb+4,data,size);

	ev_session_write(client->session,(char*)mb,total);
	if (ev_session_output_size(client->session) > WARN_OUTPUT_FLOW) {
		fprintf(stderr,"client:%d more then %dkb flow need to send out",client_id,WARN_OUTPUT_FLOW/1024);
	}
}

void
client_manager_release(struct client_manager* manager) {
	client_manager_stop(manager);
	container_foreach(manager->container,close_client);
	container_release(manager->container);
	free(manager);
}

struct lclient_manager {
	struct client_manager* manager;
	int alive;
	lua_State* L;
	int ref;
	int accept_ref;
	int close_ref;
	int data_ref;
};

static void 
laccept(void* ud,int id,const char* addr) {
	struct lclient_manager* client_manager = ud;
	lua_rawgeti(client_manager->L, LUA_REGISTRYINDEX, client_manager->accept_ref);
	lua_pushinteger(client_manager->L, id);
	lua_pushstring(client_manager->L, addr);
	lua_pcall(client_manager->L, 2, 0, 0);
}

static void 
lclose(void* ud,int id) {
	struct lclient_manager* client_manager = ud;
	lua_rawgeti(client_manager->L, LUA_REGISTRYINDEX, client_manager->close_ref);
	lua_pushinteger(client_manager->L, id);
	lua_pcall(client_manager->L, 1, 0, 0);
}

static void
ldata(void* ud,int client_id,int message_id,void* data,size_t size) {
	struct lclient_manager* client_manager = ud;
	lua_rawgeti(client_manager->L, LUA_REGISTRYINDEX, client_manager->data_ref);
	lua_pushinteger(client_manager->L, client_id);
	lua_pushinteger(client_manager->L, message_id);
	lua_pushlightuserdata(client_manager->L, data);
	lua_pushinteger(client_manager->L, size);
	lua_pcall(client_manager->L, 4, 0, 0);
}

static int
lclient_manager_start(lua_State* L){
	struct lclient_manager* client_manager = lua_touserdata(L, 1);
	const char* ip = luaL_checkstring(L, 2);
	int port = luaL_checkinteger(L, 3);
	if (!client_manager->manager->accept_func) {
		luaL_error(L,"client manager start error,should set accept callback first");
	}
	if (!client_manager->manager->close_func) {
		luaL_error(L,"client manager start error,should set close callback first");
	}
	if (!client_manager->manager->data_func) {
		luaL_error(L,"client manager start error,should set data callback first");
	}
	client_manager_stop(client_manager->manager);
	int real_port = client_manager_start(client_manager->manager,ip,port);
	if (real_port == -1) {
		lua_pushboolean(L, 0);
		lua_pushstring(L, strerror(errno));
		return 2;
	}
	lua_pushinteger(L, real_port);
	return 1;
}

static int
lclient_manager_stop(lua_State* L) {
	struct lclient_manager* client_manager = lua_touserdata(L, 1);
	if (client_manager_stop(client_manager->manager) < 0) {
		luaL_error(L,"client manager already stop");
	}
	return 0;
}

static int
lclient_manager_close(lua_State* L) {
	struct lclient_manager* client_manager = lua_touserdata(L, 1);
	int client_id = luaL_checkinteger(L, 2);
	int grace = luaL_optinteger(L, 3, 0);
	struct ev_client* client = get_client(client_manager->manager,client_id);
	if (!client)
		luaL_error(L,"client manager send failed,no such client:%d",client_id);
	client_manager_close(client_manager->manager,client_id,grace);
	return 0;
}

static int
lclient_manager_send(lua_State* L) {
	struct lclient_manager* client_manager = lua_touserdata(L, 1);
	int client_id = luaL_checkinteger(L, 2);
	int message_id = luaL_checkinteger(L, 3);

	struct ev_client* client = get_client(client_manager->manager,client_id);
	if (!client)
		luaL_error(L,"client manager send failed,no such client:%d",client_id);

	int need_free = 0;
	size_t size;
  	void* data = NULL;
    switch(lua_type(L,4)) {
        case LUA_TSTRING: {
            data = (void*)lua_tolstring(L, 4, &size);
            break;
        }
        case LUA_TLIGHTUSERDATA:{
            data = lua_touserdata(L, 4);
            size = lua_tointeger(L, 5);
            need_free = 1;
            break;
        }
        default:
            luaL_error(L,"unkown type:%s",lua_typename(L,lua_type(L,4)));
    }

    if (size == 0)
    	luaL_error(L,"client manager send failed,size is zero");

    client_manager_send(client_manager->manager,client_id,message_id,data,size);
    if (need_free)
    	free(data);
    return 0;
}

static int
lclient_manager_set_callback(lua_State* L) {
	struct lclient_manager* client_manager = lua_touserdata(L, 1);
	luaL_checktype(L, 2, LUA_TFUNCTION);
	luaL_checktype(L, 3, LUA_TFUNCTION);
	luaL_checktype(L, 4, LUA_TFUNCTION);

	client_manager->data_ref = luaL_ref(L, LUA_REGISTRYINDEX);
	client_manager->close_ref = luaL_ref(L, LUA_REGISTRYINDEX);
	client_manager->accept_ref = luaL_ref(L, LUA_REGISTRYINDEX);

	client_manager_callback(client_manager->manager,laccept,lclose,ldata);
	return 0;
}

int
lclient_manager_release(lua_State* L) {
	struct lclient_manager* client_manager = lua_touserdata(L, 1);
	if (client_manager->alive == 1) {
		client_manager->alive = 0;
		client_manager_release(client_manager->manager);
		luaL_unref(L, LUA_REGISTRYINDEX, client_manager->ref);
		luaL_unref(L, LUA_REGISTRYINDEX, client_manager->accept_ref);
		luaL_unref(L, LUA_REGISTRYINDEX, client_manager->close_ref);
		luaL_unref(L, LUA_REGISTRYINDEX, client_manager->data_ref);
	}
	return 0;
}

int
lcreate_client_manager(lua_State* L,struct ev_loop_ctx* loop_ctx,size_t max) {
	struct lclient_manager* client_manager = lua_newuserdata(L, sizeof(*client_manager));
	client_manager->manager = client_manager_create(loop_ctx,max,client_manager);
	client_manager->alive = 1;    
	client_manager->L = G(L)->mainthread;//callback must be run in main thread

	if (luaL_newmetatable(L, "meta_client_manager")) {
        const luaL_Reg meta_client_manager[] = {
            { "start", lclient_manager_start },
            { "stop", lclient_manager_stop },
            { "close", lclient_manager_close },
            { "send", lclient_manager_send },
            { "release", lclient_manager_release },
            { "set_callback", lclient_manager_set_callback },
            { NULL, NULL },
        };
        luaL_newlib(L,meta_client_manager);
        lua_setfield(L, -2, "__index");
    }
    lua_setmetatable(L, -2);

    lua_pushvalue(L, -1);
    client_manager->ref = luaL_ref(L, LUA_REGISTRYINDEX);

    return 1;
}