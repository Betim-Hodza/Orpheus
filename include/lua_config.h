#ifndef LUA_CONFIG_H
#define LUA_CONFIG_H

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  char *starting_directory; // relative path under MPD music root
  char *connection_type;    // "socket" or "network"
  char *socket_path;
  char *host;
  int port;
} OrpheusConfig;

// Initialize config with safe defaults (caller must later call config_free)
void config_init(OrpheusConfig *cfg);

// Load config from Lua file into cfg. Returns 1 on success, 0 on failure.
int config_load(OrpheusConfig *cfg, const char *config_path);

// Free all heap strings inside the config struct
void config_free(OrpheusConfig *cfg);

#endif
