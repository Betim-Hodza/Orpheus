// This purpose is to create a music player client in C
//
// The goal of this is for it to be a TUI (Terminal user interface)
//
// * note: want to create config file in lua, will learn how to do so
// * support both unix socket and loopback network connections

#include "../include/log.h"
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

  // setup logging
  FILE *logfile = fopen("/tmp/orpheus.log", "w");
  if (logfile) {
    // going to make this into a flag we can use later or maybe an env var ?
    log_add_fp(logfile, LOG_DEBUG);
    log_set_quiet(true);
  } else {
    fclose(logfile);
    log_warn("no logfile for this session");
  }

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
    log_fatal("bad connection type? shrug");
    config_free(&cfg);
    if (isendwin() == FALSE)
      endwin();
    exit(1);
  }
  validate_connection(conn);

  log_debug("Before init ncurses");

  // init ncurses
  initscr();
  raw();
  keypad(stdscr, TRUE);
  noecho();
  curs_set(0);

  log_debug("After init ncurses");

  log_debug("Before init ui");

  // initialize our ui
  init_ui(cfg.starting_directory, &ui);
  free(cfg.starting_directory);

  log_debug("After init ui");

  log_debug("Before run tui");

  // run tui
  run_tui(conn, &ui);

  // cleanup
  fclose(logfile);
  clean_tui(&ui);
  config_free(&cfg);
  mpd_connection_free(conn);

  return 0;
}
