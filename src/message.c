#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "message.h"

Response generate_response(char *, char *);
Response parse_get_request(char *);
Response parse_head_request(char *);

// takes a request message as argument and returns
// the correct response.
Response get_response(char *message, size_t len)
{
  Response res;
  res = parse_request(message, len);
  return res;
}

Response parse_request(char *message, size_t len)
{
  char copy[len];
  strcpy(copy, message);
  char *token = strtok(message, " ");

  if (token == NULL) {
    return generate_response("400", "Bad Request"); 
  }

  if (strcmp(token, "GET") == 0) {
    return parse_get_request(message);
  } else if (strcmp(token, "HEAD") == 0) {
    return parse_head_request(message);
  } else {
    return generate_response("501", "Not Implemented");
  }
}


Response parse_get_request(char *message)
{
  char *token = strtok(NULL, " ");

  if (token == NULL) {
    return generate_response("400", "Bad Request");
  }

  return generate_response("200", "OK");
}

Response parse_head_request(char *message)
{
  Response res;
  return res;
}

Response generate_response(char *status_code, char *reason_text)
{

  Response res;
  res.message = malloc(100);
  if (res.message == NULL) {
    res.message = "HTTP/1.1 500 Internal Server Error\r\nContent-length:0\r\n\r\n";
    res.ok = 1;
    return res;
  }
  strcpy(res.message, "HTTP/1.1 ");
  strcat(res.message, status_code);
  strcat(res.message, reason_text);
  strcat(res.message, "\r\nContent-length:0\r\n\r\n");
  res.length = strlen(res.message);
  res.ok = 1;
  return res;
}

