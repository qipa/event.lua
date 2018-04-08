#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#include "ev.h"
#include "socket_event.h"
#include "socket_util.h"
#include "mail_box.h"

#define LUA_EV_ERROR    0
#define LUA_EV_TIMEOUT	1
#define LUA_EV_ACCEPT   2
#define LUA_EV_CONNECT  3
#define LUA_EV_DATA     4

#define META_EVENT 			"meta_event"
#define META_BUFFER 		"meta_buffer"
#define META_TIMER			"meta_timer"
#define META_LISTENER 		"meta_listener"
#define META_UDP 			"meta_udp"
#define META_MAILBOX		"meta_mailbox"

#define STATE_HEAD 0
#define STATE_BODY 1

#define HEADER_TYPE_WORD 	2
#define HEADER_TYPE_DWORD 	4

#define THREAD_CACHED_SIZE 1024 * 1024

__thread char THREAD_CACHED_BUFFER[THREAD_CACHED_SIZE];

struct lua_ev_timer;

struct lua_ev {
	struct ev_loop* loop;
	lua_State* main;
	int ref;
	int callback;
	struct lua_ev_timer* freelist;
};

struct lua_ev_session {
	struct lua_ev* lev;
	int ref;
	struct ev_session* session;
	int closed;
	int wait;

	int header;
	int state;
	int need;
};

struct lua_ev_listener {
	struct lua_ev* lev;
	int ref;
	struct ev_listener* listener;
	int closed;
	int header;
};

struct lua_ev_timer {
	struct lua_ev* lev;
	int ref;
	struct ev_timer io;
	int cancel;
	struct lua_ev_timer* next;
};

struct lua_udp_session {
	struct lua_ev* lev;
	struct ev_io rio;
	int fd;

	int ref;
	int closed;
	int callback;

	char* recv_buffer;
	size_t recv_size;
};

struct lua_mailbox {
	struct lua_ev* lev;
	struct ev_io io;
	int recv_mail_fd;
	int send_mail_fd;
	int ref;
	int callback;
	int closed;
};

static int
meta_init(lua_State* L,const char* meta) {
	luaL_newmetatable(L,meta);
 	lua_setmetatable(L, -2);
	lua_pushvalue(L, -1);
	return luaL_ref(L, LUA_REGISTRYINDEX);
}

static struct lua_ev_session*
session_create(lua_State* L, struct lua_ev* lev,int fd,int header) {
	struct lua_ev_session* lev_session = lua_newuserdata(L, sizeof(struct lua_ev_session));
	memset(lev_session, 0, sizeof(*lev_session));
	lev_session->lev = lev;
	lev_session->closed = 0;
	lev_session->header = header;
	lev_session->state = STATE_HEAD;
	
	if (fd > 0)
		lev_session->session = ev_session_bind(lev->loop, fd);

	lev_session->ref = meta_init(L,META_BUFFER);

	return lev_session;
}

static int
session_destroy(struct lua_ev_session* lev_session) {
	struct lua_ev* lev = lev_session->lev;
	luaL_unref(lev->main, LUA_REGISTRYINDEX, lev_session->ref);
	ev_session_free(lev_session->session);
	return 0;
}

static inline void
udp_destroy(struct lua_udp_session* udp_session) {
	struct lua_ev* lev = udp_session->lev;
	luaL_unref(lev->main, LUA_REGISTRYINDEX, udp_session->ref);
	luaL_unref(lev->main, LUA_REGISTRYINDEX, udp_session->callback);
	if (ev_is_active(&udp_session->rio))
		ev_io_stop(lev->loop, &udp_session->rio);

	udp_session->closed = 1;
	free(udp_session->recv_buffer);
	close(udp_session->fd);
}

static void
error_occur(struct ev_session* ev_session,void* ud) {
	struct lua_ev_session* lev_session = ud;
	struct lua_ev* lev = lev_session->lev;

	lua_rawgeti(lev->main, LUA_REGISTRYINDEX, lev->callback);
	lua_pushinteger(lev->main, LUA_EV_ERROR);
	lua_rawgeti(lev->main, LUA_REGISTRYINDEX, lev_session->ref);
	lua_pcall(lev->main, 2, 0, 0);
	lev_session->closed = 1;
	session_destroy(lev_session);
}

static void
read_complete(struct ev_session* ev_session, void* ud) {
	struct lua_ev_session* lev_session = ud;
	struct lua_ev* lev = lev_session->lev;

	if (lev_session->header == 0) {
		lua_rawgeti(lev->main, LUA_REGISTRYINDEX, lev->callback);
		lua_pushinteger(lev->main, LUA_EV_DATA);
		lua_rawgeti(lev->main, LUA_REGISTRYINDEX, lev_session->ref);
		lua_pcall(lev->main, 2, 0, 0);
	} else {
		for(;;) {
			size_t len = ev_session_input_size(lev_session->session);
			switch(lev_session->state) {
				case STATE_HEAD: {
					if (len < lev_session->header)
						return;

					if (lev_session->header == HEADER_TYPE_WORD) {
						uint8_t header[HEADER_TYPE_WORD];
						ev_session_read(lev_session->session,(char*)header,HEADER_TYPE_WORD);
						lev_session->need = header[0] | header[1] << 8;
					} else {
						assert(lev_session->header == HEADER_TYPE_DWORD);
						uint8_t header[HEADER_TYPE_DWORD];
						ev_session_read(lev_session->session,(char*)header,HEADER_TYPE_DWORD);
						lev_session->need = header[0] | header[1] << 8 | header[2] << 16 | header[3] << 24;
					}
					lev_session->need -= lev_session->header;
					lev_session->state = STATE_BODY;
					break;
				}
				case STATE_BODY: {
					if (len < lev_session->need)
						return;

					char* data = THREAD_CACHED_BUFFER;
					if (lev_session->need > THREAD_CACHED_SIZE)
						data = malloc(lev_session->need);

					ev_session_read(lev_session->session,data,lev_session->need);

					lua_rawgeti(lev->main, LUA_REGISTRYINDEX, lev->callback);
					lua_pushinteger(lev->main, LUA_EV_DATA);
					lua_rawgeti(lev->main, LUA_REGISTRYINDEX, lev_session->ref);
					lua_pushlightuserdata(lev->main,data);
					lua_pushinteger(lev->main,lev_session->need);
					lua_pcall(lev->main, 4, 0, 0);
					lev_session->state = STATE_HEAD;

					if (data != THREAD_CACHED_BUFFER)
						free(data);
					break;
				}
				default: {
					assert(0);
				}
			}
		}
	}
}	

static void
close_complete(struct ev_session* ev_session, void* ud) {
	struct lua_ev_session* lev_session = ud;
	struct lua_ev* lev = lev_session->lev;
	assert(lev_session->closed == 1);

	lua_rawgeti(lev->main, LUA_REGISTRYINDEX, lev->callback);
	lua_pushinteger(lev->main, LUA_EV_ERROR);
	lua_rawgeti(lev->main, LUA_REGISTRYINDEX, lev_session->ref);
	lua_pcall(lev->main, 2, 0, 0);

	session_destroy(lev_session);
}

static void
connect_complete(struct ev_session* session,void *userdata) {
	struct lua_ev_session* lev_session = userdata;
	struct lua_ev* lev = lev_session->lev;

	lua_rawgeti(lev->main, LUA_REGISTRYINDEX, lev->callback);
	lua_pushinteger(lev->main, LUA_EV_CONNECT);
	lua_pushinteger(lev->main, lev_session->wait);

	int fd = ev_session_fd(session);
	int error;
	socklen_t len = sizeof(error);  
	int code = getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len);  
	if (code < 0 || error) {  
		char* strerr;
		if (code >= 0) {
			strerr = strerror(error);
		} else {
			strerr = strerror(errno);
		}
		lua_pushboolean(lev->main,0);
		lua_pushstring(lev->main,strerr);

		session_destroy(lev_session);
	} else {
		socket_nonblock(fd);
		socket_keep_alive(fd);
		socket_closeonexec(fd);

		lua_pushboolean(lev->main,1);
		lua_rawgeti(lev->main, LUA_REGISTRYINDEX, lev_session->ref);

		ev_session_setcb(lev_session->session,read_complete,NULL,error_occur,lev_session);
		ev_session_disable(lev_session->session,EV_WRITE);
		ev_session_enable(lev_session->session,EV_READ);
	}
	lua_pcall(lev->main, 4, 0, 0);
}

static void 
accept_socket(struct ev_listener *listener, int fd, char* info, void *ud) {
	struct lua_ev_listener* lev_listener = ud;
	struct lua_ev* lev = lev_listener->lev;

	if (fd < 0) {
		fprintf(stderr,"accept fd error:%s\n",info);
		return;
	}

	socket_nonblock(fd);
	socket_keep_alive(fd);
	socket_closeonexec(fd);

	struct lua_ev_session* lev_session = session_create(lev->main, lev, fd, lev_listener->header);

	ev_session_setcb(lev_session->session,read_complete,NULL,error_occur,lev_session);
	ev_session_enable(lev_session->session,EV_READ);

	lua_rawgeti(lev->main, LUA_REGISTRYINDEX, lev->callback);
	lua_pushinteger(lev->main, LUA_EV_ACCEPT);
	lua_rawgeti(lev->main, LUA_REGISTRYINDEX, lev_listener->ref);
	lua_pushvalue(lev->main,-4);
	lua_pushstring(lev->main,info);

	lua_pcall(lev->main, 4, 0, 0);
}

static void
timeout(struct ev_loop* loop,struct ev_timer* io,int revents) {
	struct lua_ev_timer* lev_timer = io->data;
	struct lua_ev* lev = lev_timer->lev;
	lua_rawgeti(lev->main, LUA_REGISTRYINDEX, lev->callback);
	lua_pushinteger(lev->main, LUA_EV_TIMEOUT);
	lua_rawgeti(lev->main, LUA_REGISTRYINDEX, lev_timer->ref);
	lua_pcall(lev->main, 2, 0, 0);
}

static void
udp_recv(struct ev_loop* loop,struct ev_io* io,int revents) {
	struct lua_udp_session* udp_session = io->data;
	struct lua_ev* lev = udp_session->lev;

	for(;;) {
		struct sockaddr_in si;
		socklen_t slen = sizeof(si);
		int n = recvfrom(udp_session->fd, udp_session->recv_buffer, udp_session->recv_size, 0, (struct sockaddr*)&si, &slen);
		if (n<0) {
			switch(errno) {
				case EINTR:
					continue;
				case EAGAIN:
					return;
				default: {
					break;
				}
			}

			lua_rawgeti(lev->main, LUA_REGISTRYINDEX, udp_session->callback);
			lua_rawgeti(lev->main, LUA_REGISTRYINDEX, udp_session->ref);
			lua_pushboolean(lev->main,0);
			lua_pushstring(lev->main,strerror(errno));
			lua_pcall(lev->main, 3, 0, 0);

			udp_destroy(udp_session);
			return;
		}
		lua_rawgeti(lev->main, LUA_REGISTRYINDEX, udp_session->callback);
		lua_rawgeti(lev->main, LUA_REGISTRYINDEX, udp_session->ref);
		
		lua_pushlstring(lev->main,udp_session->recv_buffer,n);

        char tmp[INET6_ADDRSTRLEN];
        if (inet_ntop(si.sin_family, (void*)&si.sin_addr, tmp, sizeof(tmp))) {
        	lua_pushstring(lev->main,tmp);
        } else {
        	lua_pushstring(lev->main,"unknow");
        }
        lua_pushinteger(lev->main,ntohs(si.sin_port));
		
		lua_pcall(lev->main, 4, 0, 0);
	}
}

void
read_mail(struct ev_loop* loop,ev_io* io,int revents) {
	struct lua_mailbox* lmailbox = io->data;
	struct lua_ev* lev = lmailbox->lev;

	for (;;) {
		struct mail_message* mail = NULL;
		int n = read(io->fd, &mail, sizeof(mail));
		if (n < 0) {
			if (errno == EINTR)
				continue;
			else if (errno == EAGAIN) {
				return;
			} else {
				assert(0);
			}
		}

		assert(n == sizeof(mail));

		lua_rawgeti(lev->main, LUA_REGISTRYINDEX, lmailbox->callback);
		lua_rawgeti(lev->main, LUA_REGISTRYINDEX, lmailbox->ref);
		lua_pushinteger(lev->main, mail->source);
		lua_pushinteger(lev->main, mail->session);
		lua_pushlightuserdata(lev->main, mail->data);
		lua_pushinteger(lev->main, mail->size);
		lua_pcall(lev->main, 5, 0, 0);

		free(mail);
	}
}

static int
_session_read(lua_State* L) {
	struct lua_ev_session* lev_session = (struct lua_ev_session*)lua_touserdata(L, 1);
	size_t size = luaL_optinteger(L,2,0);
	if (lev_session->closed)
		luaL_error(L,"buffer already closed");
	
	size_t len = ev_session_input_size(lev_session->session);
	if ((size != 0 && size > len) || len == 0)
		return 0;

	if (size == 0)
		size = len;

	char* data = THREAD_CACHED_BUFFER;
	int needfree = 0;
	if (size > THREAD_CACHED_SIZE) {
		needfree = 1;
		data = malloc(size);
	}
			
	ev_session_read(lev_session->session,data,size);

	lua_pushlstring(L, data, size);
	
	if (needfree)
		free(data);

	return 1;
}

static int
_session_read_util(lua_State* L) {
	struct lua_ev_session* lev_session = (struct lua_ev_session*)lua_touserdata(L, 1);
	size_t size;
	const char* sep = lua_tolstring(L,2,&size);

	size_t length;
	char* data = ev_session_read_util(lev_session->session,sep,size,&length);
	if (!data) {
		return 0;
	}
	lua_pushlstring(L, data, length);
	free(data);
	return 1;
}

static int
_session_write(lua_State* L) {
	struct lua_ev_session* lev_session = (struct lua_ev_session*)lua_touserdata(L, 1);
	if (lev_session->closed)
		luaL_error(L,"session already closed");
	
	size_t size;
	char* data = NULL;;

	switch(lua_type(L,2)) {
		case LUA_TSTRING: {
			const char* str = lua_tolstring(L,2,&size);
			data = malloc(size);
			memcpy(data,str,size);
			break;
		}
		case LUA_TLIGHTUSERDATA:{
			data = lua_touserdata(L,2);
			size = lua_tointeger(L,3);
			break;
		}
		default:
			luaL_error(L,"session write error:unknow lua type:%s",lua_typename(L,lua_type(L,2)));
	}

	if (size == 0)
		luaL_error(L,"session write error:size is zero");

	if (ev_session_write(lev_session->session, data, size) == -1) {
		free(data);
		lua_pushboolean(L,0);
	} else {
		lua_pushboolean(L,1);
	}
	return 1;
}

static int
_session_close(lua_State* L) {
	struct lua_ev_session* lev_session = (struct lua_ev_session*)lua_touserdata(L, 1);
	if (lev_session->closed == 1)
		luaL_error(L, "session already closed");

	lev_session->closed = 1;
	ev_session_setcb(lev_session->session, NULL, close_complete, error_occur, lev_session);
	ev_session_disable(lev_session->session,EV_READ);
	ev_session_enable(lev_session->session, EV_WRITE);
	return 0;
}

static int
_session_close_immediately(lua_State* L) {
	struct lua_ev_session* lev_session = (struct lua_ev_session*)lua_touserdata(L, 1);
	if (lev_session->closed == 1) {
		luaL_error(L, "session already closed");
	}
	lev_session->closed = 1;
	session_destroy(lev_session);
	return 0;
}

static int
_session_alive(lua_State* L) {
	struct lua_ev_session* lev_session = (struct lua_ev_session*)lua_touserdata(L, 1);
	lua_pushboolean(L,lev_session->closed == 0);
	return 1;
}

static int
_listen_close(lua_State* L) {
	struct lua_ev_listener* lev_listener = (struct lua_ev_listener*)lua_touserdata(L, 1);
	if (lev_listener->closed)
		luaL_error(L, "listener alreay closed");

	lev_listener->closed = 1;
	luaL_unref(L, LUA_REGISTRYINDEX, lev_listener->ref);
	ev_listener_free(lev_listener->listener);
	return 0;
}

static int
_listen_alive(lua_State* L) {
	struct lua_ev_listener* lev_listener = (struct lua_ev_listener*)lua_touserdata(L, 1);
	lua_pushboolean(L,lev_listener->closed == 0);
	return 1;
}

static int
_listen(lua_State* L) {
	struct lua_ev* lev = (struct lua_ev*)lua_touserdata(L, 1);
	struct ev_loop* loop = lev->loop;
	
	int header = lua_tointeger(L, 2);
	if (header != 0) {
		if (header != HEADER_TYPE_WORD && header != HEADER_TYPE_DWORD)
			luaL_error(L,"error header size:%d",header);
	}
	
	int multi = lua_toboolean(L, 3);
	int ipc = lua_toboolean(L, 4);

	struct sockaddr* addr;
	if (ipc) {
		const char* file = lua_tostring(L, 5);
		unlink(file);

		struct sockaddr_un su;
		su.sun_family = AF_UNIX;  
		strcpy(su.sun_path, file);

		addr = (struct sockaddr*)&su;
	} else {
		const char* ip = lua_tostring(L, 5);
		int port = lua_tointeger(L, 6);

		struct sockaddr_in si;
		si.sin_family = AF_INET;
		si.sin_addr.s_addr = inet_addr(ip);
		si.sin_port = htons(port);

		addr = (struct sockaddr*)&si;
	}

	struct lua_ev_listener* lev_listener = lua_newuserdata(L, sizeof(*lev_listener));
	lev_listener->lev = lev;
	lev_listener->closed = 0;
	lev_listener->header = header;

	int flag = SOCKET_OPT_NOBLOCK | SOCKET_OPT_CLOSE_ON_EXEC | SOCKET_OPT_REUSEABLE_ADDR;
	if (multi)
		flag |= SOCKET_OPT_REUSEABLE_PORT;

	lev_listener->listener = ev_listener_bind(loop,addr,sizeof(*addr),16,flag,accept_socket,lev_listener);
	if (!lev_listener->listener) {
		lua_pushboolean(L, 0);
		lua_pushstring(L, strdup(strerror(errno)));
		return 2;
	}

	lev_listener->ref = meta_init(L,META_LISTENER);

	return 1;
}

static int
_connect(lua_State* L) {
	struct lua_ev* lev = (struct lua_ev*)lua_touserdata(L, 1);
	int block = lua_toboolean(L, 2);
	int header = lua_tointeger(L, 3);
	if (header != 0) {
		if (header != HEADER_TYPE_WORD && header != HEADER_TYPE_DWORD)
			luaL_error(L,"error header size:%d",header);
	}

	int wait = lua_tointeger(L, 4);
	int ipc = lua_toboolean(L, 5);
	struct sockaddr* addr;
	if (ipc) {
		const char* file = lua_tostring(L, 6);

		struct sockaddr_un su;
		su.sun_family = AF_UNIX;  
		strcpy(su.sun_path, file);

		addr = (struct sockaddr*)&su;
	} else {
		const char* ip = lua_tostring(L, 6);
		int port = lua_tointeger(L, 7);

		struct sockaddr_in si;
		si.sin_family = AF_INET;
		si.sin_addr.s_addr = inet_addr(ip);
		si.sin_port = htons(port);

		addr = (struct sockaddr*)&si;
	}

	int status;
	struct lua_ev_session* lev_session = session_create(L,lev,-1,header);
	lev_session->lev = lev;
	lev_session->wait = wait;
	lev_session->session = ev_session_connect(lev->loop,addr,sizeof(*addr),block,&status);

	if (status == CONNECT_STATUS_CONNECT_FAIL) {
		lua_pushboolean(L,0);
		lua_pushstring(L,strerror(errno));
		return 2;
	}

	if (!block) {
		ev_session_setcb(lev_session->session,NULL,connect_complete,NULL,lev_session);
		ev_session_enable(lev_session->session,EV_WRITE);
		lua_pushboolean(L,1);
	} 
	
	return 1;
}

static int
_bind(lua_State* L) {
	struct lua_ev* lev = (struct lua_ev*)lua_touserdata(L, 1);
	int fd = lua_tointeger(L, 2);
	
	struct lua_ev_session* lev_session = session_create(L,lev,fd,0);
	ev_session_setcb(lev_session->session, read_complete, NULL, error_occur, lev_session);
	ev_session_enable(lev_session->session, EV_READ);
	return 1;
}

static int
_timer_cancel(lua_State* L) {
	struct lua_ev_timer* lev_timer = (struct lua_ev_timer*)lua_touserdata(L, 1);
	if (lev_timer->cancel == 1) {
		luaL_error(L, "timer already cancel");
	}
	struct lua_ev* lev = lev_timer->lev;
	ev_timer_stop(lev->loop,(struct ev_timer*)&lev_timer->io);
	lev_timer->cancel = 1;
	lev_timer->next = lev->freelist;
	lev->freelist = lev_timer;
	return 0;
}

static int
_timer_alive(lua_State* L) {
	struct lua_ev_timer* lev_timer = (struct lua_ev_timer*)lua_touserdata(L, 1);
	lua_pushboolean(L,lev_timer->cancel == 0);
	return 1;
}

static int
_timer(lua_State* L) {
	struct lua_ev* lev = (struct lua_ev*)lua_touserdata(L, 1);

	double ti = lua_tonumber(L, 2);
	int once = lua_toboolean(L,3);
	double freq = 0;
	if (!once)
		freq = ti;

	struct lua_ev_timer* lev_timer = NULL;
	if (lev->freelist) {
		lev_timer = lev->freelist;
		lev->freelist = lev->freelist->next;
		lua_rawgeti(L, LUA_REGISTRYINDEX, lev_timer->ref);
	} else {
		lev_timer = lua_newuserdata(L, sizeof(*lev_timer));
		lev_timer->lev = lev;
		lev_timer->ref = meta_init(L,META_TIMER);
	}
	
	lev_timer->io.data = lev_timer;
	ev_timer_init((struct ev_timer*)&lev_timer->io,timeout,ti,freq);
	ev_timer_start(lev->loop,(struct ev_timer*)&lev_timer->io);
	lev_timer->cancel = 0;

	return 1;
}

static int
_udp_send(lua_State* L) {
	struct lua_udp_session* udp_session = (struct lua_udp_session*)lua_touserdata(L, 1);
	if (udp_session->closed == 1)
		luaL_error(L,"udp session:%d already closed",udp_session->fd);

	size_t length;
	const char* ip = luaL_checklstring(L,2,&length);
	int port = luaL_checkinteger(L,3);

	struct sockaddr_in si;
	si.sin_family = AF_INET;
	si.sin_addr.s_addr = inet_addr(ip);
	si.sin_port = htons(port);

	size_t size;
	char* data = NULL;
	int needfree = 0;

	switch(lua_type(L,4)) {
		case LUA_TSTRING: {
			data = (char*)lua_tolstring(L,4,&size);
			break;
		}
		case LUA_TUSERDATA:{
			data = (char*)lua_touserdata(L,4);
			size = lua_tointeger(L,5);
			needfree = 1;
			break;
		}
		default:
			luaL_error(L,"session write error:unknow lua type:%s",lua_typename(L,lua_type(L,2)));
	}

	if (size == 0)
		luaL_error(L,"udp session send error size");

	int total = socket_udp_write(udp_session->fd,data,size,(struct sockaddr *)&si,sizeof(si));

	if (needfree)
		free(data);

	if (total < 0) {
		udp_destroy(udp_session);
		lua_pushboolean(L,0);
		lua_pushstring(L,strerror(errno));
		return 2;
	}
	assert(total == size);
	lua_pushboolean(L,1);
	return 1;
}

static int
_udp_alive(lua_State* L) {
	struct lua_udp_session* udp_session = (struct lua_udp_session*)lua_touserdata(L, 1);
	lua_pushinteger(L,udp_session->closed);
	return 1;
}

static int
_udp_close(lua_State* L) {
	struct lua_udp_session* udp_session = (struct lua_udp_session*)lua_touserdata(L, 1);
	if (udp_session->closed == 1)
		luaL_error(L,"udp session:%d already closed",udp_session->fd);
	udp_destroy(udp_session);
	return 0;
}

static int
_udp_new(lua_State* L) {
	struct lua_ev* lev = (struct lua_ev*)lua_touserdata(L, 1);
	size_t recv_size = lua_tointeger(L,2);

	luaL_checktype(L,3,LUA_TFUNCTION);
	
	const char* ip = NULL;
	size_t size;
	int port = 0;
	if (!lua_isnil(L,4)) {
		ip = luaL_checklstring(L, 4, &size);
		port = luaL_checkinteger(L, 5);
	}

	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		lua_pushboolean(L,0);
		lua_pushstring(L,strerror(errno));
		return 2;
	}

	if (ip) {
		struct sockaddr_in si;
		si.sin_family = AF_INET;
		si.sin_addr.s_addr = inet_addr(ip);
		si.sin_port = htons(port);

		int status = bind(fd, (struct sockaddr*)&si, sizeof(si));
		if (status != 0) {
			close(fd);
			lua_pushboolean(L,0);
			lua_pushstring(L,strerror(errno));
			return 2;
		}
	}

	lua_pushvalue(L,3);
	int callback = luaL_ref(L,LUA_REGISTRYINDEX);

	struct lua_udp_session* udp_session = lua_newuserdata(L, sizeof(struct lua_udp_session));
	memset(udp_session,0,sizeof(*udp_session));

	udp_session->fd = fd;
	udp_session->lev = lev;
	udp_session->closed = 0;
	udp_session->callback = callback;
	udp_session->recv_size = recv_size;
	udp_session->recv_buffer = malloc(udp_session->recv_size);
	udp_session->ref = meta_init(L,META_UDP);

	socket_nonblock(udp_session->fd);
	// socket_recv_buffer(udp_session->fd,1024 * 1024);

	ev_io_init(&udp_session->rio,udp_recv,udp_session->fd,EV_READ);
	udp_session->rio.data = udp_session;
	ev_io_start(lev->loop,&udp_session->rio);

	return 1;
}

static int
_lmailbox_release(lua_State* L) {
	struct lua_mailbox* lmailbox = (struct lua_mailbox*)lua_touserdata(L, 1);
	if (lmailbox->closed)
		luaL_error(L,"mail box already closed");

	ev_io_stop(lmailbox->lev->loop, &lmailbox->io);
	close(lmailbox->recv_mail_fd);
	close(lmailbox->send_mail_fd);

	luaL_unref(L, LUA_REGISTRYINDEX, lmailbox->ref);
	luaL_unref(L, LUA_REGISTRYINDEX, lmailbox->callback);

	lmailbox->closed = 1;
	return 1;
}

static int
_lmailbox_alive(lua_State* L) {
	struct lua_mailbox* lmailbox = (struct lua_mailbox*)lua_touserdata(L, 1);
	lua_pushinteger(L, lmailbox->closed == 1);
	return 1;
}


static int
_lmailbox_new(lua_State* L) {
	struct lua_ev* lev = (struct lua_ev*)lua_touserdata(L, 1);

	luaL_checktype(L,2,LUA_TFUNCTION);
	int callback = luaL_ref(L,LUA_REGISTRYINDEX);

	struct lua_mailbox* lmailbox = lua_newuserdata(L, sizeof(struct lua_mailbox));
	memset(lmailbox,0,sizeof(*lmailbox));

	int fd[2];
	if (pipe(fd))
		return 0;

	socket_nonblock(fd[0]);
	socket_nonblock(fd[1]);

	lmailbox->lev = lev;
	lmailbox->recv_mail_fd = fd[0];
	lmailbox->send_mail_fd = fd[1];
	lmailbox->callback = callback;
	lmailbox->closed = 0;

	lmailbox->io.data = lmailbox;
	ev_io_init(&lmailbox->io,read_mail,lmailbox->recv_mail_fd,EV_READ);
	ev_io_start(lmailbox->lev->loop,&lmailbox->io);

	lmailbox->ref = meta_init(L,META_MAILBOX);
	lua_pushinteger(L, lmailbox->send_mail_fd);

	return 2;
}

extern int lcreate_client_manager(lua_State* L,struct ev_loop* loop,size_t max);

static int
_client_manager_new(lua_State* L) {
	struct lua_ev* lev = (struct lua_ev*)lua_touserdata(L, 1);
	int max = luaL_checkinteger(L, 2);
	if (max <= 0) {
		luaL_error(L,"error create client manager,size invalid");
	}
	return lcreate_client_manager(L,lev->loop,max);
}

static int
_dispatch(lua_State* L) {
	struct lua_ev* lev = (struct lua_ev*)lua_touserdata(L, 1);
	ev_loop(lev->loop,0);
	return 0;
}

static int
_release(lua_State* L) {
	struct lua_ev* lev = (struct lua_ev*)lua_touserdata(L, 1);
	ev_loop_destroy(lev->loop);
	luaL_unref(L, LUA_REGISTRYINDEX, lev->ref);
	return 0;
}

static int
_break(lua_State* L) {
	struct lua_ev* lev = (struct lua_ev*)lua_touserdata(L, 1);
	ev_break(lev->loop,EVBREAK_ALL);
	return 0;
}

static int
_event_new(lua_State* L) {

	luaL_checktype(L,1,LUA_TFUNCTION);
	int callback = luaL_ref(L,LUA_REGISTRYINDEX);

	struct lua_ev* lev = lua_newuserdata(L, sizeof(*lev));
	lev->loop = ev_loop_new(0);

	lev->main = L;
	lev->callback = callback;
	lev->freelist = NULL;
	lev->ref = meta_init(L,META_EVENT);

	lua_pushlightuserdata(L,lev->loop);
	lua_setfield(L,LUA_REGISTRYINDEX,"ev_loop");

	return 1;
}

int
luaopen_ev_core(lua_State* L) {
	luaL_checkversion(L);
	
	luaL_newmetatable(L, META_EVENT);
	const luaL_Reg meta_event[] = {
		{ "listen", _listen },
		{ "connect", _connect },
		{ "timer", _timer },
		{ "bind", _bind },
		{ "udp", _udp_new },
		{ "mailbox", _lmailbox_new },
		{ "client_manager", _client_manager_new },
		{ "breakout", _break },
		{ "dispatch", _dispatch },
		{ "release", _release },
		{ NULL, NULL },
	};
	luaL_newlib(L,meta_event);
	lua_setfield(L, -2, "__index");
	lua_pop(L,1);

	luaL_newmetatable(L, META_BUFFER);
	const luaL_Reg meta_buffer[] = {
		{ "write", _session_write },
		{ "read", _session_read },
		{ "read_util", _session_read_util },
		{ "alive", _session_alive },
		{ "close", _session_close },
		{ "close_immediately", _session_close_immediately },
		{ NULL, NULL },
	};
	luaL_newlib(L,meta_buffer);
	lua_setfield(L, -2, "__index");
	lua_pop(L,1);

	luaL_newmetatable(L, META_TIMER);
	const luaL_Reg meta_timer[] = {
		{ "cancel", _timer_cancel },
		{ "alive", _timer_alive },
		{ NULL, NULL },
	};
	luaL_newlib(L,meta_timer);
	lua_setfield(L, -2, "__index");
	lua_pop(L,1);

	luaL_newmetatable(L, META_LISTENER);
	const luaL_Reg meta_listener[] = {
		{ "close", _listen_close },
		{ "alive", _listen_alive },
		{ NULL, NULL },
	};
	luaL_newlib(L,meta_listener);
	lua_setfield(L, -2, "__index");
	lua_pop(L,1);

	luaL_newmetatable(L, META_UDP);
	const luaL_Reg meta_udp[] = {
		{ "send", _udp_send },
		{ "alive", _udp_alive },
		{ "close", _udp_close },
		{ NULL, NULL },
	};
	luaL_newlib(L,meta_udp);
	lua_setfield(L, -2, "__index");
	lua_pop(L,1);

	luaL_newmetatable(L, META_MAILBOX);
	const luaL_Reg meta_mailbox[] = {
		{ "alive", _lmailbox_alive },
		{ "release", _lmailbox_release },
		{ NULL, NULL },
	};
	luaL_newlib(L,meta_mailbox);
	lua_setfield(L, -2, "__index");
	lua_pop(L,1);

	const luaL_Reg l[] = {
		{ "new", _event_new },
		{ NULL, NULL },
	};
	luaL_newlib(L, l);
	return 1;
}
