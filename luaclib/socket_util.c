#include "socket_util.h"

int socket_nonblock(int fd) {
    int flags;
    if ((flags = fcntl(fd, F_GETFL, NULL)) < 0) {
        fprintf(stderr,"fcntl(%d, F_GETFD):O_NONBLOCK\n",fd);
        return -1;
    }
    if (!(flags & O_NONBLOCK)) {
        if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
            fprintf(stderr,"fcntl(%d, F_SETFD):O_NONBLOCK\n",fd);
            return -1;
        }
    }
    return 0;
}

int socket_closeonexec(int fd) {
    int flags;
    if ((flags = fcntl(fd, F_GETFL, NULL)) < 0) {
        fprintf(stderr,"fcntl(%d, F_GETFD):FD_CLOEXEC\n",fd);
        return -1;
    }
    if (!(flags & FD_CLOEXEC)) {
        if (fcntl(fd, F_SETFL, flags | FD_CLOEXEC) == -1) {
            fprintf(stderr,"fcntl(%d, F_SETFD):FD_CLOEXEC\n",fd);
            return -1;
        }
    }
    return 0;
}

int socket_keep_alive(int fd) {
    int keepalive = 1;
    return setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepalive , sizeof(keepalive));  
}

int socket_reuse_addr(int fd) {
    int one = 1;
    return setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void*) &one, sizeof(one));
}

int socket_reuse_port(int fd) {
    int one = 1;
    return setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, (void*) &one, sizeof(one));
}

int socket_recv_buffer(int fd,int size) {
    return setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));
}

int socket_send_buffer(int fd,int size) {
    return setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &size, sizeof(size));
}


int
socket_bind(struct sockaddr *addr, int addrlen, int flag,int protocol) {
    int fd;

    if (protocol == IPPROTO_TCP) {
        fd = socket(addr->sa_family, SOCK_STREAM, 0);
    } else {
        assert(protocol == IPPROTO_UDP);
        fd = socket(addr->sa_family, SOCK_DGRAM, 0);
    }

    if (fd < 0)
        return -1;

    if (flag & SOCKET_OPT_NOBLOCK) {
        if (socket_nonblock(fd) == -1) {
            close(fd);
            return -1;
        }
    }

    if (flag & SOCKET_OPT_CLOSE_ON_EXEC) {
        if (socket_closeonexec(fd) == -1) {
            close(fd);
            return -1;
        }
    }

    if (flag & SOCKET_OPT_REUSEABLE_ADDR) {
        if (socket_reuse_addr(fd) == -1) {
            close(fd);
            return -1;
        }
    }

    if (flag & SOCKET_OPT_REUSEABLE_PORT) {
        if (socket_reuse_port(fd) == -1) {
            close(fd);
            return -1;
        }
    }

    int status = bind(fd, addr, addrlen);
    if (status != 0)
        return -1;
    return fd;
}

int
socket_listen(struct sockaddr* addr, int addrlen, int backlog,int flag) {
    int listen_fd = socket_bind(addr, addrlen, flag, IPPROTO_TCP);
    if (listen_fd < 0) {
        return -1;
    }
    if (listen(listen_fd, backlog) == -1) {
        close(listen_fd);
        return -1;
    }

    return listen_fd;
}

int
socket_connect(struct sockaddr* addr, int addrlen,int* connected) {
    int fd = socket(addr->sa_family, SOCK_STREAM, 0);
    if (fd < 0) {
        return -1;
    }

    socket_keep_alive(fd);
    socket_nonblock(fd);
    socket_closeonexec(fd);

    int status = connect(fd, addr, addrlen);
    if (status != 0 && errno != EINPROGRESS) {
        close(fd);
        return -1;
    }

    if(status == 0) {
        *connected = 1;
        return fd;
    }

    *connected = 0;
    return fd;
}

int
socket_read(int fd,char* data,size_t len) {
    size_t offset = 0;
    size_t left = len;
    while(left > 0) {
        int n = (int)read(fd, data + offset, left);
        if (n < 0) {
            if (errno) {
                if (errno == EINTR) {
                    continue;
                } else if (errno == EAGAIN) {
                    break;
                } else {
                    return -1;
                }
            } else {
                assert(0);
            }
        } else if (n == 0) {
            return -1;
        } else {
            offset += n;
            left -= n;
        }
    }
    return offset;
}

int
socket_write(int fd,char* data,size_t size) {
    int total = 0;
    for (;;) {
        int sz =(int)write(fd, data, size);
        if (sz < 0)  {
            switch(errno)
            {
            case EINTR:
                continue;
            case EAGAIN:
                return total;
            default:
                fprintf(stderr,"send fd :%d error:%s\n",fd,strerror(errno));
                return -1;
            }
        }
        else if (sz == 0) {
            return -1;
        }
        else {
            size -= sz;
            data += sz;
            total += sz;
            if (0 == size)
                break;
        }
    }

    return total;
}

int
socket_udp_write(int fd,char* data,size_t size,struct sockaddr* addr,size_t addrlen) {
    int total = 0;
    for(;;) {
        int sz = sendto(fd,data,size,0,(struct sockaddr *)addr,addrlen);
        if (sz < 0) {
            switch(errno) 
            {
            case EINTR:
                continue;
            case EAGAIN:
                return total;
            default:
                fprintf(stderr,"sendto fd :%d error:%s\n",fd,strerror(errno));
                return -1;
            }
        } else if (sz == 0) {
            return -1;
        } else {
            size -= sz;
            data += sz;
            total += sz;
            if (0 == size)
                break;
        }
    }
    return total;
}

int
socket_accept(int listen_fd,char* info) {
    union sockaddr_all u;
    socklen_t len = sizeof(u);
    int client_fd = accept(listen_fd, &u.s, &len);
    if (client_fd < 0) {
        snprintf(info, HOST_SIZE, "%s", strerror(errno));
        return -1;
    }

    socket_keep_alive(client_fd);
    socket_nonblock(client_fd);

    if (u.s.sa_family == AF_INET) {
        void * sin_addr = (u.s.sa_family == AF_INET) ? (void*)&u.v4.sin_addr : (void *)&u.v6.sin6_addr;
        int sin_port = ntohs((u.s.sa_family == AF_INET) ? u.v4.sin_port : u.v6.sin6_port);
        char tmp[INET6_ADDRSTRLEN];
        if (inet_ntop(u.s.sa_family, sin_addr, tmp, sizeof(tmp))) {
            snprintf(info, HOST_SIZE, "%s:%d", tmp, sin_port);
        }
    } else {
        snprintf(info, HOST_SIZE, "ipc:unknown");
    }
    
    return client_fd;
}

char* 
get_peer_info(int fd) {
    union sockaddr_all u;
    socklen_t slen = sizeof(u);
    if (getpeername(fd, &u.s, &slen) == 0) {
        void * sin_addr = (u.s.sa_family == AF_INET) ? (void*)&u.v4.sin_addr : (void *)&u.v6.sin6_addr;
        int sin_port = ntohs((u.s.sa_family == AF_INET) ? u.v4.sin_port : u.v6.sin6_port);
        char tmp[INET6_ADDRSTRLEN];
        char* result = malloc(INET6_ADDRSTRLEN);
        memset(result,0,INET6_ADDRSTRLEN);
        if (inet_ntop(u.s.sa_family, sin_addr, tmp, INET6_ADDRSTRLEN)) {
            snprintf(result, INET6_ADDRSTRLEN, "%s:%d", tmp, sin_port);
            return result;
        } else {
            free(result);
        }
    }
    return NULL;
}