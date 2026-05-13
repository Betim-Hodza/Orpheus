// This purpose is to create a music player client in CPP
//
// The goal of this is for it to be a TUI (Terminal user interface)
//
// * note: want to create config file in lua, will learn how to do so
// * support both unix socket and loopback network connections

#include "../include/log.hpp"
#include "../include/ui.hpp"
#include "../include/util.hpp"
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

  // find current music directory
  Util::debugPrint("Looking for default music dir ~/Music");

  Util::debugPrint("Initializing Orpheus");
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
