#include <stdio.h>
#include "message.h"

// takes a request message as argument and returns
// the correct response.
struct response get_response(char *req, size_t len)
{
  struct response res;
  res.message = "HTTP/1.1 200 OK\r\nContent-Length: 5\r\n\r\nHello";
  res.length = 43;
  res.ok = 1;
  return res;
}
