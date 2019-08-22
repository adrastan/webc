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

#define BUF_SIZE 5
#define TIMEOUT  60

void print_error(int, char *);
int create_thread(void *, int *);
void *handle_connection(void *);

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

    if (create_thread(handle_connection, &new_fd) != 0) {
      perror("unable to create a new thread");
      close(new_fd);
    }
  }

  return 0;
}

int create_thread(void *func, int *new_fd)
{
  pthread_t thread_id;
  return pthread_create(&thread_id, NULL, handle_connection, new_fd); 
}

void *handle_connection(void *args)
{
  int new_fd = *(int *)args;
  int bytes, position = 0;
  char buf[BUF_SIZE];
  time_t start;
  start = time(NULL);

  char *message = (char *)malloc(8096 * sizeof(char));
  if (message == NULL) {
    printf("unable to allocate memory for request\n");
    return NULL;
  }

  // keep getting bytes until error, closed or end of data detected
  // otherwise keep waiting
  while (1) {
    // server timeout
    if ((time(NULL) - start) > TIMEOUT) {
      print_error(new_fd, "server timeout");
      return NULL;
    }
   
    bytes = recv(new_fd, buf, BUF_SIZE - 1, MSG_DONTWAIT);

    // message was received
    if (bytes > 0) {
      for (int i = 0; i < bytes; ++i) {
        message[position] = buf[i];
        ++position;
      } 
    }

    // connection was closed
    if (bytes == 0) {
      print_error(new_fd, "connection closed");
      return NULL;
    }
    
    // error occured, check if it's not a blocking error
    if (bytes < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
      print_error(new_fd, "recv error");
      return NULL;
    } else if (message[position - 4] == '\r' && message[position - 3] == '\n' && message[position - 2] == '\r' && message[position - 1] == '\n') {
      break;
    }
  }

  int sent = send(new_fd, "HTTP/1.1 200 OK\r\nContent-Length: 5\r\n\r\nHello", 43, 0);

  if (sent == -1) {
    print_error(new_fd, "error sending bytes");
    return NULL;
  }

  printf("sent %d bytes\n", sent);
  close(new_fd);
  return NULL;
}

void print_error(int sock, char *mes)
{
  perror(mes);
  close(sock);
}
