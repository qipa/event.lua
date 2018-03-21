#include "message_queue.h"

#define THRESHOLD 1024

struct queue_context {
	int head;
	int tail;
	int cap;
	struct queue_message* message;
	int threshold;
	int overload;
};

struct message_queue {
	struct queue_context queue[2];
	pthread_mutex_t lock;
	int reader;
	int writer;
};

struct message_queue* 
queue_create() {
	struct message_queue* mq_ctx = malloc(sizeof(*mq_ctx));
	int i;
	for(i = 0;i < 2;i++) {
		struct queue_context* q = &mq_ctx->queue[i];
		q->tail = q->head = 0;
		q->cap = 8;
		q->threshold = THRESHOLD;
		q->overload = 0;
		q->message = malloc(sizeof(*q->message) * q->cap);
		memset(q->message,0,sizeof(*q->message) * q->cap);
	}
	mq_ctx->reader = 0;
	mq_ctx->writer = 1;
	pthread_mutex_init(&mq_ctx->lock, NULL);
	return mq_ctx;
}

void
queue_free(struct message_queue* mq) {
	free(mq->queue[0].message);
	free(mq->queue[1].message);
	pthread_mutex_destroy(&mq->lock);
	free(mq);
}

static inline int
queue_length(struct queue_context *q) {
	int head, tail,cap;
	head = q->head;
	tail = q->tail;
	cap = q->cap;

	if (head <= tail) {
		return tail - head;
	}
	return tail + cap - head;
}

void
queue_push(struct message_queue* mq,int source,int session,void* data,size_t size) {
	pthread_mutex_lock(&mq->lock);
	assert(mq->writer != mq->reader);
	struct queue_context* q = &mq->queue[mq->writer];

	struct queue_message* message = &q->message[q->tail];
	message->source = source;
	message->session = session;
	message->data = data;
	message->size = size;

	if (++q->tail >= q->cap)
		q->tail = 0;
	
	if (q->head == q->tail) {
		struct queue_message* nmesasge = malloc(sizeof(*nmesasge) * q->cap * 2);
		int i;
		for(i = 0;i < q->cap;i++) {
			nmesasge[i] = q->message[(q->head + i)%q->cap];
		}
		q->head = 0;
		q->tail = q->cap;
		q->cap *= 2;
		free(q->message);
		q->message = nmesasge;
	}

	if (queue_length(q) >= q->threshold) {
		q->threshold = q->threshold * 2;
		q->overload = 1;
	}
	pthread_mutex_unlock(&mq->lock);
}

struct queue_message*
queue_pop(struct message_queue* mq,int ud) {
	struct queue_context* queue = &mq->queue[mq->reader];
	
	if (queue->overload == 1) {
		queue->overload = 0;
		fprintf(stderr,"ctx:[%d] reader queue overload:%d\n",ud,queue_length(queue));
	}

	if (queue->head != queue->tail) {
		struct queue_message* message = &queue->message[queue->head++];
		if (queue->head >= queue->cap) {
			queue->head = 0;
		}
		return message;
	}

	pthread_mutex_lock(&mq->lock);

	++mq->writer;
	++mq->reader;
	mq->writer = mq->writer % 2;
	mq->reader = mq->reader % 2;

	pthread_mutex_unlock(&mq->lock);

	queue = &mq->queue[mq->reader];

	if (queue->head != queue->tail) {
		struct queue_message* message = &queue->message[queue->head++];
		if (queue->head >= queue->cap) {
			queue->head = 0;
		}
		return message;
	}
	return NULL;
}