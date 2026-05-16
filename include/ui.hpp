#ifndef UI_H
#define UI_H

// direct includes
#include <aalib.h>
#include <errno.h>
#include <jpeglib.h>
#include <mpd/albumart.h>
#include <mpd/client.h>
#include <mpd/readpicture.h>
#include <ncurses.h>
#include <unistd.h>
// indirect includes
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <filesystem>
#include <string>
#include <vector>

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

  // directy semi-globals
  std::string current_directory;
  std::string music_root;
  std::vector<std::string> item_uris;
  std::vector<ItemType> item_types;
  int selected_index = 0;

  // turn on and off screens
  bool show_directory_selection = false;
  std::string input_buffer;
  Tab current_tab = Tab::home;

  // ascii album art
  std::vector<std::string> ascii_art;
  int ascii_width = 0;
  int ascii_height = 0;
  std::string cached_image_path;

  // queue scrolling
  unsigned int total_qsongs = 0;
  unsigned int queue_ctr = 0;
};

class UIManager
{
public:
  UIManager();
  ~UIManager();

  void init(const std::filesystem::path &music_root);
  void cleanup();
  void run();

private:
  UI state;

  void updateHeader();
  void updateFooter();
  void updateMainArea();
  void updateDirectoryBrowser();
  void updateDirectorySelection();
  void helpScreen();
  void queueScreen();

  std::string getParentDirectory(const std::string &path);
};

#endif
