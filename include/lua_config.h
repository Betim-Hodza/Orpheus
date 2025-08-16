#ifndef LUA_CONFIG_H
#define LUA_CONFIG_H

#include <lualib.h>
#include <lua.h>
#include <stdlib.h>
#include <string.h>
#include <lauxlib.h>

// setup and configure lua script (required for tui to work)
void config_lua(lua_State *L, char *starting_directory, char *connection_type, char *socket_path, char *host, int *port);

#endif