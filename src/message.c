#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "message.h"
#include "uri.h"

Response parse_get_request(Request);
Response parse_head_request(char *);
char * get_next_line(char *, size_t);

Response parse_request(Request req)
{
  // the first line of the request message
  char *status_line = get_next_line(req.message, req.length);

  // invalid status line
  if (status_line == NULL) {
    return generate_response("400", "Bad Request");
  }

  // get request method token (get, post etc)
  req.method = strtok(status_line, " ");
  if (req.method == NULL) {
    return generate_response("400", "Bad Request"); 
  }

  // get the request url token
  req.uri = strtok(NULL, " ");
  if (req.uri == NULL) {
    return generate_response("400", "Bad Request");
  }

  // get http version token
  req.version = strtok(NULL, " ");
  if (req.version == NULL) {
    return generate_response("400", "Bad Request");
  }

  // check method, url and version are correct format
  // then get all available headers
  req.headers = malloc(sizeof (char *) * 10); 
  char *header = get_next_line(NULL, req.length);
  while (header != NULL) {
    req.headers[0] = header;
    header = get_next_line(NULL, req.length);
  }

  return generate_response("200", "OK");
  /*
  if (strcmp(method, "GET") == 0) {
    return parse_get_request(req);
  } else if (strcmp(method, "HEAD") == 0) {
    return parse_head_request(req.message);
  } else {
    return generate_response("501", "Not Implemented");
  }
  */
}


Response parse_get_request(Request req)
{
  char *uri = strtok(NULL, " ");
  if (!is_valid_uri(uri)) {
    return generate_response("400", "Bad Request");
  }

  req.uri = uri;
  char *version = strtok(NULL, " ");
  char *save_ptr = version; 
  char *http_tok = strtok_r(save_ptr, "/", &save_ptr);
  char *version_tok = strtok_r(save_ptr, "\r\n", &save_ptr);

  if (strcmp(http_tok, "HTTP") != 0) {
    return generate_response("400", "Bad Request");
  }

  if (strchr(version_tok, '.') == NULL) {
    return generate_response("400", "Bad Request");
  }
  
  if (strtof(version_tok, NULL) == 0) {
    return generate_response("400", "Bad Request");
  }
  char *header_tok;
  while ((header_tok = strtok_r(save_ptr, "\r\n", &save_ptr)) != NULL) {
    printf("%s\n", header_tok);
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
    return res;
  }
  strcpy(res.message, "HTTP/1.1 ");
  strcat(res.message, status_code);
  strcat(res.message, reason_text);
  strcat(res.message, "\r\nContent-length:0\r\n\r\n");
  res.length = strlen(res.message);
  return res;
}

char * get_next_line(char *message, size_t length)
{
  static int pos = 0;
  static char *save_ptr;
  char *p = NULL;

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

void process_headers(Request req)
{
  return;
}
