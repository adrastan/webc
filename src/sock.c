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

Request get_request(int);
int get_start_of_body(char *, size_t);

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

void send_response(int new_fd, Response res)
{
  if (!send_all(new_fd, res.message, res.length, 0)) {
    res.close = 1;
  }
}

Response get_response(int new_fd)
{
  Request req = get_request(new_fd);
  Response res;

  if (!req.ok) {
    res.message = "HTTP/1.1 400 Bad Request\r\nContent-length:0\r\n\r\n";
    res.close = 1;
  } else {
    res = get_response_message(req);
  }
  return res;
}


// attempts to get message from the socket.
// this can fail if:
// 1: the server was unable to allocate memory for the message (close socket)
// 2: the connection timed out (close the socket)
// 3: the connection was closed (close the socket)
// 4: recv returned -1 (close the socket)
Request get_request(int new_fd)
{
  Request req;
  req.message = malloc(BUF_SIZE);

  if (req.message == NULL) {
    errno = 3;
    req.ok = 0;
    return req;
  }

  size_t bytes = recv_all(new_fd, req, MSG_DONTWAIT);
  if (bytes == -1) {
    free(req.message);
    req.message = NULL;
    req.ok = 0;
  } else {
    req.length = bytes;
    req.ok = 1;
  }
  return req;
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

// keep getting bytes from the socket until either
// 1: timeout
// 2: connection is closed
// 3: there was an error
// 4: \r\n\r\n is received
int recv_all(int new_fd, Request req, int flags)
{
  int position = 0, bytes, total_bytes = 0;
  char buf[BUF_SIZE];
  time_t start = time(NULL);
  int buf_factor = 1, start_of_body = 0, processed_headers = 0;

  while (1) {
    // request is taking too long; abort
    if ((time(NULL) - start) > TIMEOUT) {
      printf("Socket %d timed out\n", new_fd);
      errno = ETIMEOUT;
      return -1;
    }

    bytes = recv(new_fd, buf, BUF_SIZE, flags);

    // increase req message size if bytes received doesn't fit
    if (position >= (BUF_SIZE) * (buf_factor)) {
      char *new_message = (char *)realloc(req.message, (BUF_SIZE * (buf_factor + 1)));
      if (new_message == NULL) {
        return -1;
      }
      ++buf_factor;
      req.message = new_message;
    }

    // got bytes from socket
    if (bytes > 0) {
      total_bytes += bytes;
      start = time(NULL);
      position = read_bytes_into_message(position, req.message, buf, bytes);
    }
    // socket connection was closed 
    else if (bytes == 0) {
      printf("Closing socket %d\n", new_fd);
      errno = ECLOSED;
      return -1;
    } 
    // there was an error
    else if (bytes < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
      return -1;
    }

    if (!start_of_body) {
      start_of_body = get_start_of_body(req.message, total_bytes);
    }

    if (start_of_body && !processed_headers) {
      process_headers(req, start_of_body);
      processed_headers = 1;
    }

    if (start_of_body && start_of_body <= total_bytes - 1) {
      req.body = &req.message[start_of_body];
      break;
    }
    else if (start_of_body) {
      req.body = NULL;
      break;
    }
  }

  return total_bytes;
}

int get_start_of_body(char *message, size_t len)
{
  for (int i = 0; i < len; ++i) {
    if (message[i - 3] == '\r' && message[i - 2] == '\n' && message[i - 1] == '\r' && message[i] == '\n') {
      return i + 1;
    }
  }
  return 0;
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
