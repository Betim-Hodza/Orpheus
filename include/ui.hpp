#pragma once

// direct includes
#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <filesystem>
#include <string>
#include <vector>

#include "art.hpp"
#include "player.hpp"

// macros
#define MAX_ITEMS 1000
#define MAX_PATH 256
#define TAB_COUNT 4

enum class Tab
{
  home,
  directory,
  queue,
  help
};

enum class ItemType
{
  Directory,
  Song
};

struct UI
{
  // main windows
  WINDOW *main_area = nullptr;
  WINDOW *header = nullptr;
  WINDOW *footer = nullptr;
  WINDOW *directory_selection = nullptr;
  WINDOW *queue_area = nullptr;

  // quick maths
  unsigned int max_rows = 0;
  unsigned int max_cols = 0;

  // directory semi-globals
  time_t last_header_clock_update = 0;
  std::string current_directory;
  std::string last_listed_directory;
  std::string music_root;
  std::vector<std::string> item_uris;
  std::vector<ItemType> item_types;
  int selected_index = 0;
  int page = 1; // page number for scrolling
  int items_per_page = 0;

  // turn on and off screens
  Tab current_tab = Tab::home;
  Tab last_tab = Tab::home;

  // ascii album art
  Art::ColorMode art_color_mode = Art::ColorMode::ANSI_256;
  Art::RenderMode art_render_mode = Art::RenderMode::BLOCK;
  Art::ImageData cached_image;
  Art::AsciiCanvas current_art;
  std::string cached_song_path;

  // queue scrolling
  unsigned int queue_ctr = 0;

  // audio player
  MiniAudioPlayer player;
};

class UIManager
{
public:
  UIManager();
  ~UIManager();

  // external methods
  void init(const std::filesystem::path &music_root);
  void cleanup();
  void run();

  // internal methods
  UI state;
  void updateHeader();
  void updateFooter();
  void updateMainArea();
  void updateDirectoryBrowser();
  void helpScreen();
  void queueScreen();

  std::string getParentDirectory(const std::string &path);
};
