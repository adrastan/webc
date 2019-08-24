#ifndef MESSAGE_H
#define MESSAGE_H

#include <stddef.h>

struct response
{
  size_t length;
  char *message;
  char status[3];
  int ok;
};

struct request
{
  size_t length;
  char *message;
  int ok;
};

struct response get_response(char *req, size_t len);

#endif /* MESSAGE_H */
