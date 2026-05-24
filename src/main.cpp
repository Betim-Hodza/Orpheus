// This purpose is to create a music player client in CPP
//
// The goal of this is for it to be a TUI (Terminal user interface)
//
// * note: want to create config file in lua, will learn how to do so
// * support both unix socket and loopback network connections

#include "log.hpp"
#include "miniaudio.h"
#include "ui.hpp"
#include "util.hpp"
#include <iostream>
#include <lauxlib.h>
#include <lualib.h>
#include <ncurses.h>
#include <stdbool.h>
#include <stdlib.h>

int main()
{
  // file logging
  // writing both to the console and stdout
  FileLogger logger("orpheus_debug.log");
  std::streambuf *old_cout = std::cout.rdbuf(logger.rdbuf());

  // find current music directory
  Util::debugPrint("Looking for music dir: ~/Music");
  std::filesystem::path music_path = Util::expandHome("~/Music");

  Util::debugPrint("Initializing ncurses");

  // init ncurses
  initscr();
  set_escdelay(0); // we don't want to delay pressing esc for user
  raw();
  keypad(stdscr, TRUE);
  noecho();
  curs_set(0);

  Util::debugPrint("init miniaudio");

  Util::debugPrint("Starting TUI up");

  UIManager ui_manager;
  ui_manager.init(music_path.string());
  ui_manager.run();

  Util::debugPrint("Cleaning TUI up");
  // restore this so we dont segfault
  std::cout.rdbuf(old_cout);

  return 0;
}
