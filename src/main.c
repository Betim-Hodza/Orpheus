// This purpose is to create a music player client in C
//
// The goal of this is for it to be a TUI (Terminal user interface)
//
// * note: want to create config file in lua, will learn how to do so
// * support both unix socket and loopback network connections

#include "../include/lua_config.h"
#include "../include/mpd_connections.h"
#include "../include/ui.h"
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <mpd/client.h>
#include <ncurses.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

// globals
struct mpd_connection *conn;
UI ui;

int main() {

  // init
  OrpheusConfig cfg;
  config_init(&cfg); // defaults

  if (!config_load(&cfg, "config.lua"))
    config_load(&cfg, "~/.config/orpheus/config.lua");

  // make our mpd connection
  if (strcmp(cfg.connection_type, "socket") == 0) {
    conn = mpd_connection_new(cfg.socket_path, 0, 0);
  } else if (strcmp(cfg.connection_type, "network") == 0) {
    conn = mpd_connection_new(cfg.host, cfg.port, 0);
  } else {
    printf("bad connection type? shrug\n");
    config_free(&cfg);
    if (isendwin() == FALSE)
      endwin();
    exit(1);
  }
  validate_connection(conn);

  printf("Before init ncurses\n");

  // init ncurses
  initscr();
  raw();
  keypad(stdscr, TRUE);
  noecho();
  curs_set(0);

  printf("After init ncurses \n");

  printf("Before init ui \n");

  // initialize our ui
  init_ui(cfg.starting_directory, &ui);
  free(cfg.starting_directory);

  printf("After init ui \n");

  printf("Before run tui \n");

  // run tui
  run_tui(conn, &ui);

  // cleanup
  clean_tui(&ui);
  config_free(&cfg);
  mpd_connection_free(conn);

  return 0;
}
