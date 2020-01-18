#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include "sock.h"
#include "message.h"
#include "error_codes.h"

#define MAX_GET_REQUEST_SIZE 8192
#define TIMEOUT  10

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


// send len bytes to socket descriptor. this function will
// return 0 if it was successful otherwise it returns -1
int send_all(int new_fd, void *message, size_t len, int flags)
{
  char *ptr = (char *) message;

  while (len > 0) {
    int i = send(new_fd, ptr, len, flags);
    if (i < 1) {
      return -1;
    }
    ptr += i;
    len -= i;
  }

  return 0;
}

// receive all bytes from socket descriptor into message. this function
// will return the number of bytes if it was successful otherwise it returns
// -1
int recv_all(int new_fd, Request req, int flags)
{
  int position = 0, bytes, total_bytes = 0;
  char buf[BUF_SIZE];
  time_t start;
  start = time(NULL);
  int buf_factor = 1;

  while (1) {
    // server timeout
    if ((time(NULL) - start) > TIMEOUT) {
      printf("Socket %d timed out\n", new_fd);
      errno = ETIMEOUT;
      return -1;
    }

    bytes = recv(new_fd, buf, BUF_SIZE - 1, flags);

    if (position >= (BUF_SIZE - 1) * (buf_factor)) {
      char *new_message = (char *)realloc(req.message, (BUF_SIZE * (buf_factor + 1)));
      if (new_message == NULL) {
        return -1;
      }
      ++buf_factor;
      req.message = new_message;
    }

    // message was received
    if (bytes > 0) {
      if ((total_bytes + bytes) > MAX_GET_REQUEST_SIZE) {
        errno = EREQUEST_SIZE_EXCEEDED;
        return -1;
      }
      total_bytes += bytes;
      // reset timer once we get some bytes
      start = time(NULL);
      position = read_bytes_into_message(position, req.message, buf, bytes);
    }
    // connection was closed
    else if (bytes == 0) {
      printf("Closing socket %d\n", new_fd);
      errno = ECLOSED;
      return -1;
    }
    // error occured, check if it's not a blocking error
    else if (bytes < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
      return -1;
    }

    if (req.message[position - 4] == '\r' && req.message[position - 3] == '\n' && req.message[position - 2] == '\r' && req.message[position - 1] == '\n') {
      break;
    }
  }

  req.message[position] = '\0';
  return position;
}

int read_bytes_into_message(int position, char *message, char *buf, int bytes)
{
  for (int i = 0; i < bytes; ++i) {
    if (position == 0 && (buf[i] == '\r' || buf[i] == '\n')) {
      continue; 
    }
    message[position] = buf[i];
    ++position;
  }
  return position; 
}
