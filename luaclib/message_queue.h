#ifndef QUEUE_H
#define QUEUE_H

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <sys/types.h>


struct queue_message {
	int source;
	int session;
	void* data;
	size_t size;
};

struct message_queue;

struct message_queue* queue_create();
void queue_free(struct message_queue* queue_ctx);

inline void queue_push(struct message_queue* queue_ctx,int source,int session,void* data,size_t size);
inline struct queue_message* queue_pop(struct message_queue* queue_ctx,int ud);

#endif