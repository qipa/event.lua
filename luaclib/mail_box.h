#ifndef MAIL_BOX_H
#define MAIL_BOX_H



struct mail_message {
	struct mail_message* next;
	int source;
	int session;
	void* data;
	size_t size;
};


#endif