#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
  int status;
  struct addrinfo *servinfo;
  struct addrinfo hints;
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  
  // get address info for host using port 80, if param is not provided use
  // loopback address
  if ((status = getaddrinfo(argv[1], "http", &hints, &servinfo) != 0)) {
    fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
    exit(1);
  }

  char ipstr[INET6_ADDRSTRLEN];
  struct addrinfo *p;
  int sock;

  // loop through list of structs and bind to the first one we can
  for (p = servinfo; p != NULL; p = p->ai_next) {
    sock = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if (sock == -1) {
      continue;
    }
    if (bind(sock, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
      fprintf(stderr, "bind error: %s\n", gai_strerror(errno));
      exit(1);  
    }
    break;
  }
  
  freeaddrinfo(servinfo);

  // check if we were unable to bind any sockets and exit program
  if (p == NULL) {
    fprintf(stderr, "failed to bind\n");
    exit(1);
  }

  // setup the socket to start listening for incoming connections
  // this will mark the socket as a listening socket which accept()
  // can be called on and sets the maximum number of connections in the queue
  if (listen(sock, 5) == -1) {
    fprintf(stderr, "listen error: %s\n", gai_strerror(errno));
    exit(1);  
  }

  socklen_t addr_size;
  struct sockaddr_storage their_addr;
  int new_fd;

  // this is the main loop of the server. each iteration will
  // attempt to accept an incoming connection and send data through it
  while(1) {
    addr_size = sizeof their_addr;
    new_fd = accept(sock, (struct sockaddr *)&their_addr, &addr_size);

    if (new_fd == -1) {
      fprintf(stderr, "accept error: %s\n", gai_strerror(errno));
      continue;
    }

    if (send(new_fd, "Hello", 5, 0) == -1) {
      perror("error sending data");
    }

    close(new_fd);
  }

  return 0;
}
