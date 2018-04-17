#ifndef PIPE_MESSAGE_H
#define PIPE_MESSAGE_H



struct pipe_message {
	struct pipe_message* next;
	int source;
	int session;
	void* data;
	size_t size;
};


#endif