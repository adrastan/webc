#ifndef SERVER_H
#define SERVER_H

struct config {
  char *address;
  char *port;
  size_t max_post;
};

void clean_up(void);
void start_server(struct config);
void stop_server(void);
int create_thread(void *, int *);
void *handle_connection(void *);
struct request get_request(int);

#endif /* SERVER_H */
