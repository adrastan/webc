#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "message.h"
#include "uri.h"


// returns a pointer to the next header in the request
// sets the carriage return character to null
char * get_next_line(char *message, size_t length)
{
  static int pos = 0;
  static char *save_ptr;
  char *p = NULL;

  if (pos >= length) {
    pos = 0;
    return NULL;
  }

  if (message != NULL) {
    pos = 0;
    save_ptr = message;
  }

  for (int i = pos; i < length; ++i) {
    if ((i + 1) < length && save_ptr[i] == '\r' && save_ptr[i + 1] == '\n') {
      save_ptr[i] = '\0';
      p = &save_ptr[pos];
      pos = i + 2;
      break;
    }
  }

  return p;
}

void process_headers(Request req, size_t start_of_body)
{
  req.header_count = 0;
  char *status_line = get_next_line(req.message, start_of_body);
  if (status_line == NULL) {
    req.ok = 0;
    req.message = NULL;
    return;
  }
  req.method = strtok(status_line, " ");
  req.uri = strtok(NULL, " ");
  req.version = strtok(NULL, "\n");
  
  char *header = get_next_line(NULL, start_of_body);
  while (header != NULL) {
    req.headers[req.header_count] = header;
    ++req.header_count;
    header = get_next_line(NULL, start_of_body);
  } 
  return;
}

Response get_response_message(Request req)
{
  Response res;
  res.message = "HTTP/1.1 200 OK \r\nContent-length:0\r\n\r\n";
  res.length = strlen(res.message);
  res.close = 0;

  return res; 
}
