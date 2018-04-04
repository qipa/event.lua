

#define CACHED_SIZE 1024 * 1024

typedef void (*accept_callback)(void* ud,int id);
typedef void (*close_callback)(void* ud,int id);
typedef void (*data_callback)(void* ud,int client_id,int message_id,void* data,size_t size);



__thread char CACHED_BUFFER[CACHED_SIZE];

struct client_manager {
	struct ev_loop* loop;
	struct ev_listener* listener;
	struct object_container* container;
	accept_callback accept_func;
	close_callback close_func;
	data_callback data_func;
	void* ud;
};

struct ev_client {
	struct client_manager* manager;
	struct ev_session* session;
	int id;
	int need;
	uint8_t seed;
};

static void
read_complete(struct ev_session* ev_session, void* ud) {
	struct ev_client* client = ud;

	while (true) {
		size_t total = ev_session_input_size(ev_session);
		if (client->need == 0) {
			if (total >= 2) {
				uint8_t header[2];
				ev_session_read(ev_session,(char*)header,2);
			} else {
				return;
			}
		}

		total = ev_session_input_size(ev_session);
		if (client->need > 0) {
			if (total >= client->need) {
				char* data = CACHED_BUFFER;
				if (client->need > CACHED_SIZE) {
					data = malloc(client->need);
				}
				ev_session_read(ev_session,data,client->need);
				
				int i;
			    for (i = 0; i < client->need; ++i) {
			        data[i] = data[i] ^ client->seed;
			        client->seed += data[i];
			    }
			    ushort id = data[0] | data[1] << 8;

			    client->manager->data_func(client->manager->ud,client->id,id,&data[2],client->need - 2);

			    if (data != CACHED_BUFFER)
			    	free(data);

			    client->need = 0;
			} else {
				return;
			}
		}
	}
}	

static void
error_happen(struct ev_session* session,void* ud) {
	struct ev_client* client = ud;
	int id = client->id;
	ev_session_free(client->session);
	container_remove(client->manager,client->id);
	free(client);
	manager->close_func(manager->ud,id);
}

static void 
accept_client(struct ev_listener *listener, int fd, char* info, void *ud) {
	struct client_manager* manager = ud;
	if (fd < 0) {
		fprintf(stderr,"accept fd error:%s\n",info);
		return;
	}

	socket_nonblock(fd);
	socket_keep_alive(fd);
	socket_closeonexec(fd);

	struct ev_client* client = malloc(sizeof(*client));
	client->manager = manager;
	client->session = ev_session_bind(manager->loop,fd);
	client->id = container_add(manager->container,session);
	ev_session_setcb(client->session,read_complete,NULL,error_happen,client);
	ev_session_enable(client->session,EV_READ);
	manager->accept_func(manager->ud,client->id);
}

struct client_manager*
client_manager_create(struct ev_loop* loop,size_t max) {
	struct client_manager* manager = malloc(sizeof(*manager));
	memset(manager,0,sizeof(*manager));
	
	manager->container = container_create(max);
	manager->loop = loop;
	return manager;
}

int
client_manager_listen(struct client_manager* manager,const char* ip,int port) {
	struct sockaddr_in si;
	si.sin_family = AF_INET;
	si.sin_addr.s_addr = inet_addr(ip);
	si.sin_port = htons(port);

	int flag = SOCKET_OPT_NOBLOCK | SOCKET_OPT_CLOSE_ON_EXEC | SOCKET_OPT_REUSEABLE_ADDR;
	manager->listener = ev_listener_bind(manager->loop,(struct sockaddr*)&si,sizeof(si),16,flag,accept_client,manager);
	if (!manager->listener) {
		return -1;
	}
	return 0;
} 

void
client_manager_callback(struct client_manager* manager,accept_callback accept,close_callback close,data_callback data,void* ud) {
	manager->accept_func = accept;
	manager->close_func = close;
	manager->data_func = data;
	manager->ud = ud;
}

int
client_manager_close_listener(struct client_manager* manager) {
	if (manager->listener) {
		ev_listener_free(manager->listener);
		manager->listener = NULL;
		return 0;
	}
	return -1;
}

int
client_manager_close_client(struct client_manager* manager,int client_id) {
	struct ev_client* client = container_get(manager->container,client_id);
	if (!client) {
		return -1;
	}
	ev_session_free(client->session);
	container_remove(manager->container,client_id);
	free(client);
	return 0;
}

void
client_manager_send(struct client_manager* manager,int client_id,int message_id,void* data,size_t size) {
	struct ev_client* client = container_get(manager->container,client_id);

	size_t total = size + sizeof(short) * 2;

    uint8_t* mb = malloc(total);
    memset(mb,0,total);
    memcpy(mb,&total,2);
    memcpy(mb+2,&message_id,2);
    memcpy(mb+4,data,size);

	ev_session_write(client->session,mb,total);

	free(data);
}

static void 
close_client(int id,void* data) {
	struct ev_client* client = data;
	ev_session_free(client->session);
	free(client);
}

void
client_manager_release(struct client_manager* manager) {
	client_manager_close_listener(manager);
	container_foreach(manager->container,close_client);
	container_release(manager->container);
	free(manager);
}