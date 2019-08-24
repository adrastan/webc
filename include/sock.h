#ifndef SOCK_H
#define SOCK_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

int get_socket(const char *, const char *, const struct addrinfo *, int);

#endif /* SOCK_H */
