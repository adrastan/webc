#ifndef MESSAGE_H
#define MESSAGE_H

#include <stddef.h>

struct response
{
  size_t length;
  char *message;
  char status[3];
  int ok;
  int close;
};

struct request
{
  size_t length;
  char *message;
  int ok;
  int close;
};

typedef struct response Response;
typedef struct request Request;

Response parse_request(char *, size_t);
Response get_response(char *req, size_t len);

#endif /* MESSAGE_H */
