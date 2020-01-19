#ifndef SOCK_H
#define SOCK_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "message.h"

#define BUF_SIZE 64

Response get_response(int);
int get_socket(const char *, const char *, const struct addrinfo *, int);
int recv_all(int, Request, int); 
int send_all(int, void *, size_t, int);
int read_bytes_into_message(int, char *, char *, int);
void send_response(int, Response);

#endif /* SOCK_H */
