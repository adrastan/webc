#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <errno.h>
#include "server.h"
#include "sock.h"
#include "message.h"
#include "error_codes.h"

int server_running = 0;

void start_server(struct config settings)
{
  int new_fd;
  struct addrinfo hints;
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  
  // get socket for address and port 
  int sock = get_socket(settings.address, settings.port, &hints, 5);
  free(settings.address);
  settings.address = NULL;
  free(settings.port);
  settings.port = NULL;

  if (sock == -1) {
    perror("error opening socket");
    exit(1);
  }

  socklen_t addr_size;
  struct sockaddr_storage their_addr;
  server_running = 1;

  // this is the main loop of the server. each iteration will
  // attempt to accept an incoming connection and send data through it
  while (server_running) {
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
}

void stop_server()
{
  server_running = 0;
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


  while (1) {
    // get the next request message from the socket
    Request req = get_request(new_fd);

    // there was a problem getting the request message
    if (!req.ok) {
      if (errno == EREQUEST_SIZE_EXCEEDED) {
        Response res = generate_response("413", "Payload Too Large");
        send_all(new_fd, res.message, res.length, 0);
      }
      close(new_fd);
      return NULL; 
    }

    // parse the request message into the appropriate response
    Response res = get_response(req);
    free(req.message);
    req.message = NULL;

    if (!res.ok) {
      // TODO check errno and send error response
      close(new_fd);
      return NULL;
    }

    // send all response bytes to the socket
    if (send_all(new_fd, res.message, res.length, 0) == -1) {
      close(new_fd);
      return NULL;
    }
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
