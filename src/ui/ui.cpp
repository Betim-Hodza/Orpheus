#include "../../include/ui.hpp"
#include "../../include/log.hpp"
#include "../../include/util.hpp"
#include <ctime>
#include <iomanip>
#include <ncurses.h>
#include <sstream>

UIManager::UIManager()
{
}

UIManager::~UIManager()
{
  cleanup();
}

/* *
 * @brief cleans up class variables
 * */
void UIManager::cleanup()
{
  delwin(state.header);
  delwin(state.main_area);
  delwin(state.directory_selection);
  delwin(state.queue_area);
  delwin(state.footer);
  endwin();
}

// helpers

/* *
 * @brief gets parent directory from a path string
 * @param the string path of the current directory
 * */
std::string UIManager::getParentDirectory(const std::string &path)
{
  if (path.empty())
    return "";

  size_t last_slash = path.find_last_of('/');
  if (last_slash == std::string::npos)
    return "";

  return path.substr(0, last_slash);
}

/* *
 * @brief initialize UI
 *
 * @param path to music directory
 * */
void UIManager::init(const std::filesystem::path &music_root)
{
  Util::debugPrint("initializing UI with music root: " + music_root.string());

  getmaxyx(stdscr, state.max_rows, state.max_cols);
  Util::debugPrint("Terminal dimensions: " + std::to_string(state.max_rows) + " x " + std::to_string(state.max_cols));

  state.header = newwin(3, state.max_cols, 0, 0);
  state.main_area = newwin(state.max_rows - 5, state.max_cols, 3, 0);
  state.directory_selection = newwin(2, state.max_cols, state.max_rows - 4, 0);
  state.queue_area = newwin(state.max_rows - 5, state.max_cols, 3, 0);
  state.footer = newwin(2, state.max_cols, state.max_rows - 2, 0);

  if (!state.header || !state.main_area || !state.directory_selection || !state.queue_area || !state.footer)
  {
    Util::debugPrint("Failed to create one or more ncurses windows");
    endwin();
    exit(1);
  }

  state.music_root = music_root.string();
  state.current_directory = music_root.string();
  state.current_tab = Tab::home;

  Util::debugPrint("UI initialized successfully");
}

/* *
 * @brief Shows tabs and current time
 * */
void UIManager::updateHeader()
{
  auto now = std::time(nullptr);
  auto *tm = std::localtime(&now);
  std::ostringstream oss;
  oss << std::put_time(tm, "%I:%M:%S");
  std::string time_str = oss.str();

  werase(state.header);
  box(state.header, 0, 0);
  mvwprintw(state.header, 1, 2, "Time: %s", time_str.c_str());

  std::vector<std::string> tab_names = {"Home", "Directory", "Queue", "Help"};
  int x_pos = 2;
  for (int i = 0; i < TAB_COUNT; i++)
  {
    if (static_cast<Tab>(i) == state.current_tab)
      wattron(state.header, A_REVERSE);
    mvwprintw(state.header, 2, x_pos, " %s ", tab_names[i].c_str());
    if (static_cast<Tab>(i) == state.current_tab)
      wattroff(state.header, A_REVERSE);
    x_pos += tab_names[i].length() + 3;
  }
  wrefresh(state.header);
}

/* *
 * @brief updates to directory selection screen
 * */
void UIManager::updateDirectorySelection()
{
  werase(state.directory_selection);
  box(state.directory_selection, 0, 0);
  mvwprintw(state.directory_selection, 1, 2, "Music directory: %s", state.input_buffer.c_str());
  mvwprintw(state.directory_selection, 1, state.max_cols - 30, "[Enter to save, Esc to cancel]");
  wrefresh(state.directory_selection);
}

/* *
 * @brief updates to help screen
 * */
void UIManager::helpScreen()
{
  werase(state.main_area);
  box(state.main_area, 0, 0);

  mvwprintw(state.main_area, 1, 2, "Main Help:");
  mvwprintw(state.main_area, 2, 2, "P              | Play/Pause");
  mvwprintw(state.main_area, 3, 2, "'[' ']'        | Prev song, Next song");
  mvwprintw(state.main_area, 4, 2, "<LEFT> <RIGHT> | Move to tabs left or right (cycles)");
  mvwprintw(state.main_area, 5, 2, "<BACKSPACE>    | Clear song queue");

  mvwprintw(state.main_area, 7, 2, "Directory Help:");
  mvwprintw(state.main_area, 8, 2, "<UP> <DOWN>    | Scrolls up and down a list");
  mvwprintw(state.main_area, 9, 2, "<ESC>          | Goes up a directory");
  mvwprintw(state.main_area, 10, 2, "<ENTER>        | Goes down a directory and adds song to queue");
  wrefresh(state.main_area);
}

/* *
 * @brief updates to song queue screen
 *
 * @note for now we're going to leave out the mpd connection stuff
 * impl is largely incomplete
 * */
void UIManager::queueScreen()
{
  werase(state.main_area);
  box(state.main_area, 0, 0);
  mvwprintw(state.main_area, 1, 2, "Queue / Playlist:");

  Util::debugPrint("Impl largely incomplete");

  // get status from our future playlist feature in our program
  // if (!status)
  //{
  //	Util::printError("Failed to get status in queue_screen");
  //	mvwprintw(state.main_area, 2, 2, "Failed to get status");
  //	wrefresh(state.main_area);
  //	return;
  //}

  // state.total_qsongs = getQueueLength(status);
  // for now we'll use 0 as a placeholder
  state.total_qsongs = 0;
  if (state.total_qsongs == 0)
  {
    mvwprintw(state.main_area, 2, 2, "Queue empty");
    wrefresh(state.main_area);
    return;
  }

  unsigned start, end;
  start = state.queue_ctr % state.total_qsongs;
  end = start + (unsigned)(state.max_rows - 3);
  if (end > state.total_qsongs)
    end = state.total_qsongs;

  // get q list range or smthing from our own code later

  // print out each song given
  // TITLE, ARTIST,

  wrefresh(state.main_area);
}

// Update Functions

/* *
 * @brief basic tree directory browser to load up queue and play songs
 * pressing enter adds it to the playlist queue (end of queue).
 * pressing b puts the song at the beginning of the queue (play next)
 * pressing del clears the queue (shows confirmation)
 * */
void UIManager::updateDirectoryBrowser()
{
  werase(state.main_area);
  box(state.main_area, 0, 0);
  mvwprintw(state.main_area, 1, 2, "Directory: %s", state.current_directory.c_str());

  // read the state.current dir / states music path / root
  std::vector<std::string> content_list;

  // err msg inside listDir func
  if (!Util::listDir(state.current_directory, content_list))
  {
    return;
  }

  // get all the items in the directory (directories and files)
  for (size_t i = 0; i < content_list.size(); i++)
  {
    mvwprintw(state.main_area, i + 3, 2, content_list[i].c_str());
  }
  // selection of current file / directory ?
  // go up and down a directory with enter (down), esq (up)

  wrefresh(state.main_area);
}

/* *
 * @brief footer shows state of play paused or stopped. also shows animation
 * for music playing. current time in song and total song length.
 * * */
void UIManager::updateFooter()
{
  werase(state.footer);
  box(state.footer, 0, 0);
  mvwprintw(state.footer, 1, 2, "Ready");

  // draw some animation inside this box that goes in a circle
  // on play / pause, show current time playing in the song and total song len

  wrefresh(state.footer);
}

/* *
 * @brief routes different tabs.
 * Shows ASCII Album art and current song playing
 * */
void UIManager::updateMainArea()
{

  switch (state.current_tab)
  {
  case Tab::directory:
    updateDirectoryBrowser();
    break;
  case Tab::queue:
    queueScreen();
    break;
  case Tab::help:
    helpScreen();
    break;
  case Tab::home:
  default:
    werase(state.main_area);
    box(state.main_area, 0, 0);
    mvwprintw(state.main_area, 1, 2, "Orpheus - C++ Music Player");
    mvwprintw(state.main_area, 2, 2, "No Track Playing");
    wrefresh(state.main_area);
    break;
  }
}

/* *
 * @brief event loop with switching between states and tabs
 * also kinda the entry point of this file
 * */
void UIManager::run()
{
  Util::debugPrint("Starting TUI event loop");
  int ch;
  timeout(500);

  while ((ch = getch()) != 'q')
  {
    // tab switching
    if (ch == KEY_LEFT)
    {
      int current = static_cast<int>(state.current_tab);
      current = (current == 0) ? 3 : current - 1;
      state.current_tab = static_cast<Tab>(current);
    }
    if (ch == KEY_RIGHT)
    {
      int current = static_cast<int>(state.current_tab);
      current = (current == 3) ? 0 : current + 1;
      state.current_tab = static_cast<Tab>(current);
    }

    // Music controls to be implemented later
    // if (ch == 'p') // play / pause
    // if (ch == ']') // skip forwards
    // if (ch == '[') // skip back
    // if (ch == KEY_BACKSPACE) // clear queue

    // directory browser input
    if (state.current_tab == Tab::directory)
    {
      switch (ch)
      {
      case KEY_UP:
        if (state.selected_index > 0)
          state.selected_index--;
        break;
      case KEY_DOWN:
        if (state.selected_index < static_cast<int>(state.item_uris.size()) - 1)
          state.selected_index++;
        break;
      case 27: // ESC
        state.current_directory = getParentDirectory(state.current_directory);
        state.selected_index = 0;
        break;
      }
    }

    // directory selection inout
    if (state.show_directory_selection)
    {
      if (ch == '\n')
      {
        state.current_directory = state.input_buffer;
        state.show_directory_selection = false;
      }
      else if (ch == 27) // ESC
      {
        state.input_buffer = state.current_directory;
        state.show_directory_selection = false;
      }
      else if (ch == KEY_BACKSPACE && !state.input_buffer.empty())
      {
        state.input_buffer.pop_back();
      }
      else if (ch >= 32 && ch <= 126)
      {
        state.input_buffer += static_cast<char>(ch);
      }
    }

    // queue scroll
    if (state.current_tab == Tab::queue)
    {
      if (ch == KEY_UP && state.total_qsongs > 0)
      {
        state.queue_ctr = (state.queue_ctr + state.total_qsongs - 1) % state.total_qsongs;
      }
      else if (ch == KEY_DOWN && state.total_qsongs > 0)
      {
        state.queue_ctr = (state.queue_ctr + 1) % state.total_qsongs;
      }
    }

    updateHeader();
    updateMainArea();
    updateFooter();
  }

  Util::debugPrint("User has quit TUI");
}
