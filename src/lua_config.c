#include "../include/lua_config.h"
#include <stdio.h>
#include <unistd.h>

// expand ~ to $HOME
static char *expand_tilde(const char *path) {
  if (!path || path[0] != '~')
    return path ? strdup(path) : NULL;

  const char *home = getenv("HOME");
  if (!home)
    home = "/";

  // +1 for NULL skip the ~
  size_t len = strlen(home) + strlen(path + 1) + 1;
  char *out = malloc(len);
  if (!out)
    return NULL;
  snprintf(out, len, "%s%s", home, path + 1);
  return out;
}

// strip the MPD Music root prefix from absolute path
// so MPD recieve a relative URI. If doesn't start with root
// value is unchanged (it's relative)
static char *make_mpd_relative(const char *abs_path) {
  const char *home = getenv("HOME");
  if (!home)
    return strdup(abs_path);

  // build root: $HOME/Music
  size_t root_len = strlen(home) + strlen("/Music");
  char *music_root = malloc(root_len + 1);
  if (!music_root)
    return strdup(abs_path);
  snprintf(music_root, root_len + 1, "%s/Music", home);

  char *result;
  if (strncmp(abs_path, music_root, root_len) == 0 &&
      (abs_path[root_len] == '/' || abs_path[root_len] == '\0')) {
    // skip past root + trailing slash
    const char *rel = abs_path + root_len + (abs_path[root_len] == '/' ? 1 : 0);
    result = strdup(rel);
  } else {
    result = strdup(abs_path);
  }

  free(music_root);
  return result;
}

// pull string global from lua stack, null if not present
static char *lua_get_string(lua_State *L, const char *name) {
  lua_getglobal(L, name);
  char *val = NULL;
  if (lua_isstring(L, -1))
    val = strdup(lua_tostring(L, -1));
  lua_pop(L, 1);
  return val;
}

// pull int global, returns default_val if not present
static int lua_get_int(lua_State *L, const char *name, int default_val) {
  lua_getglobal(L, name);
  int val = default_val;
  if (lua_isnumber(L, -1))
    val = (int)lua_tointeger(L, -1);
  lua_pop(L, 1);
  return val;
}

void config_init(OrpheusConfig *cfg) {
  cfg->starting_directory = strdup("");
  cfg->connection_type = strdup("socket");
  cfg->socket_path = NULL;
  cfg->host = NULL;
  cfg->port = 6600;
}

void config_free(OrpheusConfig *cfg) {
  free(cfg->starting_directory);
  cfg->starting_directory = NULL;
  free(cfg->connection_type);
  cfg->connection_type = NULL;
  free(cfg->socket_path);
  cfg->socket_path = NULL;
  free(cfg->host);
  cfg->host = NULL;
}

int config_load(OrpheusConfig *cfg, const char *config_path) {
  lua_State *L = luaL_newstate();
  if (!L)
    return 0;
  luaL_openlibs(L);

  // Expand tilde in the config file path itself
  char *expanded_cfg = expand_tilde(config_path);
  int ok = (luaL_dofile(L, expanded_cfg) == LUA_OK);
  free(expanded_cfg);

  if (!ok) {
    fprintf(stderr, "lua config error: %s\n", lua_tostring(L, -1));
    lua_close(L);
    return 0;
  }

  // music_directory
  char *raw_dir = lua_get_string(L, "music_directory");
  if (raw_dir) {
    char *expanded = expand_tilde(raw_dir);
    free(raw_dir);

    char *relative = make_mpd_relative(expanded);
    free(expanded);

    free(cfg->starting_directory);
    cfg->starting_directory = relative;
  }

  // connection_type
  char *conn_type = lua_get_string(L, "connection_type");
  if (conn_type) {
    free(cfg->connection_type);
    cfg->connection_type = conn_type;
  }

  // socket_path
  char *sock = lua_get_string(L, "socket_path");
  if (sock) {
    free(cfg->socket_path);
    cfg->socket_path = sock;
  }

  // host/port
  char *host = lua_get_string(L, "host");
  if (host) {
    free(cfg->host);
    cfg->host = host;
  }

  int port = lua_get_int(L, "port", 0);
  if (port > 0)
    cfg->port = port;

  lua_close(L);
  return 1;
}
