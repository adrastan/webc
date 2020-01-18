#ifndef SOCK_H
#define SOCK_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "message.h"

#define BUF_SIZE 1024

int get_socket(const char *, const char *, const struct addrinfo *, int);
int recv_all(int, Request, int); 
int send_all(int, void *, size_t, int);
int read_bytes_into_message(int, char *, char *, int);

#endif /* SOCK_H */
