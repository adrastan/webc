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

int accept_connection(int);
int server_running = 0;

void start_server(struct config settings)
{
  struct addrinfo hints;
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  
  // get socket for address and port 
  int sock = get_socket(settings.address, settings.port, &hints, settings.max_connections);
  free(settings.address);
  settings.address = NULL;
  free(settings.port);
  settings.port = NULL;

  if (sock == -1) {
    perror("error opening socket");
    exit(1);
  }

  server_running = 1;

  // this is the main loop of the server. each iteration will
  // attempt to accept an incoming connection and send data through it
  while (server_running) {
    if (!accept_connection(sock)) {
      return;
    }
  }
}

int accept_connection(int sock)
{
  struct sockaddr_storage their_addr;
  socklen_t addr_size = sizeof their_addr;
  int new_fd = accept(sock, (struct sockaddr *)&their_addr, &addr_size);

  if (new_fd == -1) {
    fprintf(stderr, "accept error: %s\n", gai_strerror(errno));
    return -1;
  }

  printf("Accepting connection on socket %d\n", new_fd);

  if (create_thread(handle_connection, &new_fd) != 0) {
    perror("unable to create a new thread");
    close(new_fd);
    return -1;
  }
  return 1;
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

// function to handle incoming connections from the socket.
void *handle_connection(void *args)
{
  int new_fd = *(int *)args;
  Response res;

  // http/1.1 spec states that a client can
  // send a request through the same connection
  // as long as they have connection: keep-alive header
  // hence we need to keep checking the socket for incomming connections
  while (1) {
    res = get_response(new_fd);

    if (res.message == NULL) {
      close(new_fd);
      break;
    } else {
      send_response(new_fd, res);
    }

    if (res.close) {
      close(new_fd);
      break;
    }
  }

  return NULL;
}
