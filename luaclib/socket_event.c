
#include "socket_event.h"

#define MIN_BUFFER_SIZE 256
#define MAX_BUFFER_SIZE 1024*1024


typedef struct data_buffer {
	struct data_buffer* prev;
	struct data_buffer* next;
	void* data;
	int size;
	int wpos;
	int rpos;
} data_buffer_t;

typedef struct ev_buffer {
	data_buffer_t* head;
	data_buffer_t* tail;
	int total;
} ev_buffer_t;


typedef struct ev_listener {
	struct ev_loop* loop;
	struct ev_io rio;
	int fd;
	listener_callback accept_cb;
	void* userdata;
} ev_listener_t;

typedef struct ev_session {
	struct ev_loop* loop;
	struct ev_io rio;
	struct ev_io wio;
	
	int fd;
	int alive;
	
	int threshold;
	ev_buffer_t input;
	ev_buffer_t output;

	data_buffer_t* freelist;
	
	ev_session_callback read_cb;
	ev_session_callback write_cb;
	ev_session_callback event_cb;

	void* userdata;
} ev_session_t;

void ev_session_free(ev_session_t* ev_session);
void ev_session_disable(ev_session_t* ev_session,int ev);

static inline struct data_buffer*
buffer_next(ev_session_t* ev_session) {
	data_buffer_t* db = NULL;
	if (ev_session->freelist != NULL) {
		db = ev_session->freelist;
		ev_session->freelist = db->next;
	} else {
		db = malloc(sizeof(*db));
	}
	db->data = NULL;
	db->wpos = db->rpos = 0;
	db->size = 0;
	db->prev = NULL;
	db->next = NULL;

	return db;
}

static inline void
buffer_reclaim(ev_session_t* ev_session,data_buffer_t* db) {
	db->next = ev_session->freelist;
	ev_session->freelist = db;
}

static inline void
buffer_append(ev_buffer_t* ev_buffer,data_buffer_t* db) {
	ev_buffer->total += db->wpos - db->rpos;
	if (ev_buffer->head == NULL) {
		assert(ev_buffer->tail == NULL);
		ev_buffer->head = ev_buffer->tail = db;
	} else {
		ev_buffer->tail->next = db;
		db->prev = ev_buffer->tail;
		db->next = NULL;
		ev_buffer->tail = db;
	}
}

static inline void
buffer_release(ev_buffer_t* ev_buffer) {
	while(ev_buffer->head) {
		data_buffer_t* tmp = ev_buffer->head;
		ev_buffer->head = ev_buffer->head->next;
		free(tmp->data);
		free(tmp);
	}
}

static void
_ev_accept_cb(struct ev_loop* loop,struct ev_io* io,int revents) {
	ev_listener_t* listener = io->data;

	char info[HOST_SIZE] = {0};
	int accept_fd = socket_accept(listener->fd,info);
	listener->accept_cb(listener,accept_fd,info,listener->userdata);
}

static void
_ev_read_cb(struct ev_loop* loop,struct ev_io* io,int revents) {
	ev_session_t* ev_session = io->data;

	struct data_buffer* rdb = buffer_next(ev_session);
	rdb->data = malloc(ev_session->threshold);
	rdb->size = ev_session->threshold;

	int fail = 0;
	for(;;) {
		int n = (int)read(ev_session->fd, rdb->data + rdb->wpos, rdb->size - rdb->wpos);
		if (n < 0) {
			if (errno) {
				if (errno == EINTR) {
					continue;
				} else if (errno == EAGAIN) {
					break;
				} else {
					fail = 1;
					break;
				}
			} else {
				assert(0);
			}
		} else if (n == 0) {
			fail = 1;
			break;
		} else {
			rdb->wpos += n;
	
			if (rdb->wpos == rdb->size) {
				ev_session->threshold *= 2;
				if (ev_session->threshold > MAX_BUFFER_SIZE)
					ev_session->threshold = MAX_BUFFER_SIZE;

				buffer_append(&ev_session->input,rdb);

				struct data_buffer* next_rdb = buffer_next(ev_session);
				next_rdb->data = malloc(ev_session->threshold);
				next_rdb->size = ev_session->threshold;

				rdb = next_rdb;
			} else {
				ev_session->threshold /= 2;
				if (ev_session->threshold < MIN_BUFFER_SIZE)
					ev_session->threshold = MIN_BUFFER_SIZE;
				break;
			}

		}
	}

	buffer_append(&ev_session->input,rdb);

	if (fail) {
		ev_session_disable(ev_session,EV_READ | EV_WRITE);
		ev_session->alive = 0;
		if (ev_session->event_cb) {
			ev_session->event_cb(ev_session,ev_session->userdata);
		}
	} else {
		if (ev_session->read_cb) {
			ev_session->read_cb(ev_session,ev_session->userdata);
		}
	}
}

static void
_ev_write_cb(struct ev_loop* loop,struct ev_io* io,int revents) {
	ev_session_t* ev_session = io->data;

	while(ev_session->output.head != NULL) {
		struct data_buffer* wdb = ev_session->output.head;
		int left = wdb->wpos - wdb->rpos;
		int total = socket_write(ev_session->fd,wdb->data + wdb->rpos,left);
		if (total < 0) {
			ev_session_disable(ev_session,EV_READ | EV_WRITE);
			ev_session->alive = 0;
			if (ev_session->event_cb)
				ev_session->event_cb(ev_session,ev_session->userdata);
			return;
		} else {
			ev_session->output.total -= total;
			if (total == left) {
				free(wdb->data);
				ev_session->output.head = wdb->next;
				buffer_reclaim(ev_session,wdb);
				if (ev_session->output.head == NULL) {
					ev_session->output.head = ev_session->output.tail = NULL;
					break;
				}
			} else {
				wdb->rpos += total;
				return;
			}
		}
	}

	ev_session_disable(ev_session,EV_WRITE);
	assert(ev_session->output.total == 0);
	if (ev_session->write_cb) {
		ev_session->write_cb(ev_session,ev_session->userdata);
	}
}

ev_listener_t*
ev_listener_bind(struct ev_loop* loop,struct sockaddr* addr, int addrlen,int backlog,int flag,listener_callback accept_cb,void* userdata) {
	int fd = socket_listen(addr, addrlen, backlog, flag);
	if (fd < 0) {
		return NULL;
	}
	ev_listener_t* listener = malloc(sizeof(*listener));
	listener->loop = loop;
	listener->fd = fd;
	listener->accept_cb = accept_cb;
	listener->userdata = userdata;

	listener->rio.data = listener;
	ev_io_init(&listener->rio,_ev_accept_cb,fd,EV_READ);
	ev_io_start(loop,&listener->rio);

	return listener;
}

int
ev_listener_fd(ev_listener_t* listener) {
	return listener->fd;
}

void
ev_listener_free(ev_listener_t* listener) {
	if (ev_is_active(&listener->rio)) {
		ev_io_stop(listener->loop, &listener->rio);
	}
	close(listener->fd);
	free(listener);
}

ev_session_t*
ev_session_bind(struct ev_loop* loop,int fd) {
	ev_session_t* ev_session = malloc(sizeof(*ev_session));
	memset(ev_session,0,sizeof(*ev_session));
	ev_session->loop = loop;
	ev_session->fd = fd;
	ev_session->threshold = MIN_BUFFER_SIZE;

	ev_session->rio.data = ev_session;
	ev_io_init(&ev_session->rio,_ev_read_cb,ev_session->fd,EV_READ);

	ev_session->wio.data = ev_session;
	ev_io_init(&ev_session->wio,_ev_write_cb,ev_session->fd,EV_WRITE);

	ev_session->alive = 1;

	return ev_session;
}

ev_session_t*
ev_session_connect(struct ev_loop* loop,struct sockaddr* addr, int addrlen, int block,int* status) {
	int result = 0;
	int fd = socket_connect(addr,addrlen,block,&result);
	if (fd < 0) {
		*status = CONNECT_STATUS_CONNECT_FAIL;
		return NULL;
	}
	ev_session_t* ev_session = ev_session_bind(loop,fd);

	//if connect op is block,result here must be true
	*status = CONNECT_STATUS_CONNECTING;
	if (result)
		*status = CONNECT_STATUS_CONNECTED;

	return ev_session;
}

void
ev_session_free(ev_session_t* ev_session) {
	ev_session->alive = 0;
	close(ev_session->fd);
	ev_session_disable(ev_session,EV_READ | EV_WRITE);

	while(ev_session->freelist) {
		data_buffer_t* tmp = ev_session->freelist;
		ev_session->freelist = ev_session->freelist->next;
		free(tmp);
	}
	buffer_release(&ev_session->input);
	buffer_release(&ev_session->output);

	free(ev_session);
}

void
ev_session_setcb(ev_session_t* ev_session,ev_session_callback read_cb,ev_session_callback write_cb,ev_session_callback event_cb,void* userdata) {
	ev_session->read_cb = read_cb;
	ev_session->write_cb = write_cb;
	ev_session->event_cb = event_cb;
	ev_session->userdata = userdata;
}

void
ev_session_enable(ev_session_t* ev_session,int ev) {
	if (ev & EV_READ) {
		if (!ev_is_active(&ev_session->rio)) {
			ev_io_start(ev_session->loop, &ev_session->rio);
		}
	} 
	if (ev & EV_WRITE) {
		if (!ev_is_active(&ev_session->wio)) {
			ev_io_start(ev_session->loop, &ev_session->wio);
		}
	} 
}

void
ev_session_disable(ev_session_t* ev_session,int ev) {
	if (ev & EV_READ) {
		if (ev_is_active(&ev_session->rio)) {
			ev_io_stop(ev_session->loop, &ev_session->rio);
		}
	} 
	if (ev & EV_WRITE) {
		if (ev_is_active(&ev_session->wio)) {
			ev_io_stop(ev_session->loop, &ev_session->wio);
		}
	} 
}

int
ev_session_fd(ev_session_t* ev_session) {
	return ev_session->fd;
}

size_t 
ev_session_input_size(ev_session_t* ev_session) {
	return ev_session->input.total;
}

size_t
ev_session_output_size(ev_session_t* ev_session) {
	return ev_session->output.total;
}

static inline int
check_eol(data_buffer_t* db,int from,const char* sep,size_t sep_len) {
	while (db) {
		int sz = db->wpos - from;
		if (sz >= sep_len) {
			return memcmp(db->data+from,sep,sep_len) == 0;
		}

		if (sz > 0) {
			if (memcmp(db->data+from,sep,sz)) {
				return 0;
			}
		}
		db = db->next;
		sep += sz;
		sep_len -= sz;
		from = 0;
	}
	return 0;
}

static inline int
search_eol(ev_session_t* ev_session,const char* sep,size_t sep_len) {
	data_buffer_t* current = ev_session->input.head;
	int offset = 0;
	while(current) {
		int i = current->rpos;
		for(; i < current->wpos;i++) {
			int ret = check_eol(current,i,sep,sep_len);
			if (ret == 1) {
				return offset + sep_len;
			}
			++offset;
		}
		current = current->next;
	}

	return -1;
}

size_t 
ev_session_read(struct ev_session* ev_session,char* result,size_t size) {
	if (size > ev_session->input.total)
		size = ev_session->input.total;

	int offset = 0;
	int need = size;
	while (need > 0) {
		data_buffer_t* rdb = ev_session->input.head;
		if (rdb->rpos + need < rdb->wpos) {
			memcpy(result + offset,rdb->data + rdb->rpos,need);
			rdb->rpos += need;

			offset += need;
			assert(offset == size);
			need = 0;
		} else {
			int left = rdb->wpos - rdb->rpos;
			memcpy(result + offset,rdb->data + rdb->rpos,left);
			offset += left;
			need -= left;
			free(rdb->data);
			
			data_buffer_t* tmp = ev_session->input.head;

			ev_session->input.head = ev_session->input.head->next;
			if (ev_session->input.head == NULL) {
				ev_session->input.head = ev_session->input.tail = NULL;
				assert(need == 0);
			}

			buffer_reclaim(ev_session,tmp);
		}
	}
	ev_session->input.total -= size;

	return size;
}

char* ev_session_read_util(ev_session_t* ev_session,const char* sep,size_t size,size_t* length) {
	int offset = search_eol(ev_session,sep,size);
	if (offset < 0) {
		return NULL;
	}
	*length = offset;
	char* result = malloc(offset);
	ev_session_read(ev_session,result,offset);
	return result;
}

int
ev_session_write(ev_session_t* ev_session,char* data,size_t size) {
	if (ev_session->alive == 0)
		return -1;

	if (!ev_is_active(&ev_session->wio)) {
		int total = socket_write(ev_session->fd,data,size);
		if (total < 0) {
			ev_session_disable(ev_session,EV_READ | EV_WRITE);
			ev_session->alive = 0;
			if (ev_session->event_cb)
				ev_session->event_cb(ev_session,ev_session->userdata);
			return -1;
		} else {
			if (total == size) {
				free(data);
				if (ev_session->write_cb) {
					ev_session->write_cb(ev_session,ev_session->userdata);
				}
			} else {
				struct data_buffer* wdb = buffer_next(ev_session);
				wdb->data = data;
				wdb->rpos = total;
				wdb->wpos = size;
				wdb->size = size;
				buffer_append(&ev_session->output,wdb);
				ev_io_start(ev_session->loop,&ev_session->wio);
			}
			return total;
		}
	} else {
		struct data_buffer* wdb = buffer_next(ev_session);
		wdb->data = data;
		wdb->rpos = 0;
		wdb->wpos = size;
		wdb->size = size;
		buffer_append(&ev_session->output,wdb);
		return 0;
	}
}
