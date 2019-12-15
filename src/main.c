#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>
#include "sock.h"
#include "message.h"

#define BUF_SIZE 8192
#define TIMEOUT  10
#define E_TIMEOUT 0
#define E_CLOSED 1
#define E_RECV 2
#define E_MALLOC 3

int create_thread(void *, int *);
void *handle_connection(void *);
int recv_all(int, char *, int); 
int send_all(int, void *, size_t, int);
struct request get_request(int);

int main(int argc, char *argv[])
{
  struct addrinfo hints;
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  
  // get socket listening on port 80 
  int sock = get_socket(argv[1], "http", &hints, 5);
  if (sock == -1) {
    perror("error opening socket");
    exit(1);
  }

  socklen_t addr_size;
  struct sockaddr_storage their_addr;
  int new_fd;

  // this is the main loop of the server. each iteration will
  // attempt to accept an incoming connection and send data through it
  for (;;) {
    addr_size = sizeof their_addr;
    new_fd = accept(sock, (struct sockaddr *)&their_addr, &addr_size);

    if (new_fd == -1) {
      fprintf(stderr, "accept error: %s\n", gai_strerror(errno));
      continue;
    }
    printf("Accepting connection on socket %d\n", new_fd);

    if (create_thread(handle_connection, &new_fd) != 0) {
      perror("unable to create a new thread");
      close(new_fd);
    }
  }

  return 0;
}

// function to create a new thread for the new socket connection
int create_thread(void *func, int *new_fd)
{
  pthread_t thread_id;
  return pthread_create(&thread_id, NULL, handle_connection, new_fd); 
}

// function to handle incoming connections from client.
// 1: attempt to get request message from client
// 2: parse the request message
// 3: send the response to the client
void *handle_connection(void *args)
{
  // get socket related to the connection;
  int new_fd = *(int *)args;

  printf("Started connection on socket %d on new thread\n", new_fd);

  while (1) {
    printf("Getting request message from socket %d\n", new_fd);
    // get the next request message from the socket
    Request req = get_request(new_fd);

    // there was a problem getting the request message
    if (!req.ok) {
      printf("Closing socket %d\n", new_fd);
      close(new_fd);
      return NULL; 
    }
    printf("Socket %d: %s", new_fd, req.message);

    printf("Getting response message for socket %d\n", new_fd);
    // parse the request message into the appropriate response
    Response res = get_response(req.message, req.length);
    free(req.message);
    req.message = NULL;

    if (!res.ok) {
      // TODO check errno and send error response
      close(new_fd);
      return NULL;
    }

    printf("Sending response message to socket %d\n", new_fd);
    // send all response bytes to the socket
    if (send_all(new_fd, res.message, res.length, 0) == -1) {
      close(new_fd);
      return NULL;
    }

    // printf("Closing socket %d\n", new_fd);
    // close(new_fd);
    // return NULL;
  }
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

  size_t bytes = recv_all(new_fd, req.message, MSG_DONTWAIT);
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

// receive all bytes from socket descriptor into message. this function
// will return the number of bytes if it was successful otherwise it returns
// -1
int recv_all(int new_fd, char *message, int flags)
{
  int position = 0, bytes;
  char buf[BUF_SIZE];
  time_t start;
  start = time(NULL);

  while (1) {
    // server timeout
    if ((time(NULL) - start) > TIMEOUT) {
      printf("Socket %d timed out\n", new_fd);
      errno = 0;
      return -1;
    }

    bytes = recv(new_fd, buf, BUF_SIZE - 1, flags);

    // message was received
    if (bytes > 0) {
      for (int i = 0; i < bytes; ++i) {
        if (position == 0 && (buf[i] == '\r' || buf[i] == '\n')) {
          continue; 
        }
        message[position] = buf[i];
        ++position;
      } 
    }

    // connection was closed
    if (bytes == 0) {
      errno = 1;
      return -1;
    }

    // error occured, check if it's not a blocking error
    if (bytes < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
      errno = 2;
      return -1;
    } else if (message[position - 4] == '\r' && message[position - 3] == '\n' && message[position - 2] == '\r' && message[position - 1] == '\n') {
      break;
    }
  }
  message[position] = '\0';
  return position;
}
