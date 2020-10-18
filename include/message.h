#ifndef MESSAGE_H
#define MESSAGE_H

#include <stddef.h>

struct response
{
  size_t length;
  char *message;
  char status[3];
  int close;
};

struct request
{
  size_t length;
  char *message;
  int ok;
  int close;
  char *method;
  char *uri;
  char *version;
  char *headers[100];
  int header_count;
  char *body;
};

typedef struct response Response;
typedef struct request Request;

char * get_next_line(char *, size_t);
void process_headers(Request, size_t);
Response get_response_message(Request);

#endif /* MESSAGE_H */
