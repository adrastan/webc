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
  char **headers;
  char *body;
};

typedef struct response Response;
typedef struct request Request;

Response parse_request(Request req);
Response generate_response(char *, char *);
void process_headers(Request);

#endif /* MESSAGE_H */
