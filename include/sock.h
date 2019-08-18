#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

int get_socket(const char *, const char *, const struct addrinfo *, int);
