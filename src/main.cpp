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
#include <lualib.h>
#include <ncurses.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

// globals
UI ui;

int main()
{

  // file logging
  // writing both to the console and stdout
  FileLogger logger("orpheus_debug.log");
  std::streambuf *old_cout = std::cout.rdbuf(logger.rdbuf());

  // find current music directory
  Util::debugPrint("Looking for music dir: ~/Music");
  std::filesystem::path music_path = Util::expand_home("~/Music");

  Util::debugPrint("Initializing Orpheus");

  // init ncurses
  initscr();
  raw();
  keypad(stdscr, TRUE);
  noecho();
  curs_set(0);

  Util::debugPrint("Starting TUI up");

  UIManager ui_manager;
  ui_manager.init(music_path.string());
  ui_manager.run();

  Util::debugPrint("Cleaning TUI up");

  return 0;
}
