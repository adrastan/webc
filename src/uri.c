#include <stdio.h>
#include <string.h>
#include "uri.h"


int is_valid_uri(char *uri)
{
  if (uri == NULL) {
    return 0;
  }
  int len = strlen(uri);

  if (uri[0] != '/') {
    return 0;
  }

  return 1;
}
