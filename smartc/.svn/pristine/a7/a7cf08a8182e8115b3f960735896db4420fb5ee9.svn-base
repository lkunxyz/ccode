#ifndef SERVER_H
#define SERVER_H

#include "http.h"

typedef struct {
	int cfd; /* client fd */
	int sfd; /* server fd */
    struct sockaddr_in client_addr;
    struct sockaddr_in server_addr;

	request *request;
	reply *reply;

	struct event event;
}connection;	

#endif
