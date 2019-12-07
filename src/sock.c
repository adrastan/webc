#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include "sock.h"

int get_socket(const char *address, const char *service, const struct addrinfo *hints, int max_connections)
{
  int status, sock;
  struct addrinfo *servinfo;

  // get address info for host using port 80, if param is not provided use
  // loopback address
  if ((status = getaddrinfo(address, service, hints, &servinfo) != 0)) {
    fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
    return -1; 
  }

  struct addrinfo *p;

  // loop through list of structs and bind to the first one we can
  for (p = servinfo; p != NULL; p = p->ai_next) {
    sock = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if (sock == -1) {
      continue;
    }
    if (bind(sock, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
      fprintf(stderr, "bind error: %s\n", gai_strerror(errno));
      return -1;  
    }
    break;
  }
  
  freeaddrinfo(servinfo);

  // check if we were unable to bind any sockets and exit program
  if (p == NULL) {
    fprintf(stderr, "failed to bind\n");
    return -1;
  }

  // setup the socket to start listening for incoming connections
  // this will mark the socket as a listening socket which accept()
  // can be called on and sets the maximum number of connections in the queue
  if (listen(sock, max_connections) == -1) {
    fprintf(stderr, "listen error: %s\n", gai_strerror(errno));
    return -1;
  }

  return sock;
}
