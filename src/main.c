#include <glib.h>
#include "server.h"

void load_config(char **, struct config *);
void load_config_defaults(void);

int main(int argc, char *argv[])
{
  struct config settings; 
  load_config(argv, &settings);

  start_server(settings);
  return 0;
}

void load_config(char *argv[], struct config *settings)
{
  g_autoptr(GError) error = NULL;
  g_autoptr(GKeyFile) key_file = g_key_file_new();

  if (!g_key_file_load_from_file(key_file, "/etc/webc/config.ini", G_KEY_FILE_NONE, &error)) {
    load_config_defaults();
    return;
  }

  settings->address = g_key_file_get_string(key_file, "Settings", "address", &error);
  settings->port = g_key_file_get_string(key_file, "Settings", "port", &error);
  settings->max_connections = g_key_file_get_integer(key_file, "Settings", "max_connections", &error);
}

void load_config_defaults()
{
  return;
}
