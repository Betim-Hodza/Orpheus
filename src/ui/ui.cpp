#include "ui.hpp"
#include "art.hpp"
#include "player.hpp"
#include "util.hpp"
#include <ctime>
#include <iomanip>
#include <map>
#include <ncurses.h>
#include <sstream>
#include <utility>

// cache for ANSI-256 / grayscale color pairs
static std::map<std::pair<int, int>, int> g_color_pair_cache;
static int g_next_pair_id = 1;

// grayscale density 0-9 mapped to ANSI-256 gray pallete indices
static const int GRAYSCALE_ANSI_MAP[10] = {236, 238, 240, 242, 244, 246, 248, 250, 253, 255};

/* *
 * @brief get or create a COLOR_PAIR id for (fg, bg)
 * fg and bg are ncurses color numbers
 *
 * @param fg
 * @param bg
 * */
static int getColorPair(int fg, int bg)
{
  auto key = std::make_pair(fg, bg);
  auto it = g_color_pair_cache.find(key);
  if (it != g_color_pair_cache.end())
  {
    return it->second;
  }

  int pair_id = g_next_pair_id++;
  init_pair(pair_id, fg, bg);
  g_color_pair_cache[key] = pair_id;
  return pair_id;
}

/* *
 * @brief Render one AsciiCell to ncurses window
 * */
static void drawCell(WINDOW *win, int y, int x, const Art::AsciiCell &cell, Art::ColorMode mode)
{
  int fg = cell.fg;
  int bg = cell.bg;
  int pair_id = 0;

  switch (mode)
  {
  case Art::ColorMode::GRAYSCALE: {
    // map density indices to actual ANSI gray color
    int ansi_fg = GRAYSCALE_ANSI_MAP[fg];
    int ansi_bg = GRAYSCALE_ANSI_MAP[bg];
    pair_id = getColorPair(ansi_fg, ansi_bg);
    break;
  }
  case Art::ColorMode::ANSI_256: {
    pair_id = getColorPair(fg, bg);
    break;
  }
  }

  // draw the half-block char with the color pair
  cchar_t cc;
  setcchar(&cc, &cell.ch, 0, pair_id, nullptr);
  mvwadd_wch(win, y, x, &cc);
}

/* *
 * @brief Draw an AsciiCanvas at the specified window coordinates.
 * */
static void drawAsciiArt(WINDOW *win, int start_y, int start_x, const Art::AsciiCanvas &art)
{
  if (art.cells.empty() || art.width == 0 || art.height == 0)
  {
    return;
  }

  for (int y = 0; y < art.height; ++y)
  {
    for (int x = 0; x < art.width; ++x)
    {
      const auto &cell = art.cells[y * art.width + x];
      drawCell(win, start_y + y, start_x + x, cell, art.mode);
    }
  }
}

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
    return "/home/bay/Music";

  size_t last_slash = path.find_last_of('/');
  if (last_slash == std::string::npos)
    return "/home/bay/Music";

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

  state.items_per_page = getmaxy(state.main_area) - 4;

  state.music_root = music_root.string();
  state.current_directory = music_root.string();
  state.current_tab = Tab::home;

  start_color();
  use_default_colors();

  state.player.init();

  Util::debugPrint("UI initialized successfully");
}

/* *
 * @brief Shows tabs and current time
 * */
void UIManager::updateHeader()
{
  auto now = std::time(nullptr);
  if (now != state.last_header_clock_update || state.current_tab != state.last_tab)
  {
    state.last_header_clock_update = now;
    state.last_tab = state.current_tab;
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
}

/* *
 * @brief updates to help screen
 * */
void UIManager::helpScreen()
{
  werase(state.main_area);
  box(state.main_area, 0, 0);

  mvwprintw(state.main_area, 1, 2, "Main Help:");
  mvwprintw(state.main_area, 2, 2, "P                     | Play/Pause");
  mvwprintw(state.main_area, 4, 2, "<LEFT> <RIGHT> 'H' 'L'| Move to tabs left or right (cycles)");
  mvwprintw(state.main_area, 5, 2, "<BACKSPACE>           | Clear song queue");
  mvwprintw(state.main_area, 6, 2, "'A'                   | Cycle GRAYSCALE / ANSI for Album art");
  mvwprintw(state.main_area, 7, 2, "'Z'                   | Cycle BLOCK / DETAILED mode for Album art");

  mvwprintw(state.main_area, 9, 2, "Directory Help:");
  mvwprintw(state.main_area, 10, 2, "<UP> <DOWN> 'K' 'J'   | Scrolls up and down a list");
  mvwprintw(state.main_area, 11, 2, "<ESC> '-'             | Goes up a directory");
  mvwprintw(state.main_area, 12, 2, "<ENTER>               | Goes down a directory and adds song to queue");
  wrefresh(state.main_area);
}

/* *
 * @brief updates to song queue screen
 * */
void UIManager::queueScreen()
{
  werase(state.main_area);
  box(state.main_area, 0, 0);
  mvwprintw(state.main_area, 1, 2, "Queue / Playlist:");

  size_t qsize = state.player.getQueueSize();
  if (qsize == 0)
  {
    mvwprintw(state.main_area, 2, 2, "Queue empty");
    wrefresh(state.main_area);
    return;
  }

  unsigned start, end;
  start = state.queue_ctr % qsize;
  end = start + (unsigned)(state.max_rows - 3);
  if (end > qsize)
  {
    end = qsize;
  }

  int current_idx = state.player.getCurrentIndex();
  for (unsigned i = start; i < end; i++)
  {
    auto *song = state.player.getQueueSong(i);
    std::string label = song ? song->song_name : "[unknown]";
    if ((int)i == current_idx)
    {
      wattron(state.main_area, A_REVERSE);
      mvwprintw(state.main_area, i - start + 3, 2, "> %s", label.c_str());
      wattroff(state.main_area, A_REVERSE);
    }
    else
    {
      mvwprintw(state.main_area, i - start + 3, 2, "  %s", label.c_str());
    }
  }

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

  // only update when we change directories or scroll past max items shown on screen
  if (state.current_directory != state.last_listed_directory || state.current_tab == Tab::directory)
  {

    // err msg inside listDir func
    if (!Util::listDir(state.current_directory, state.item_uris, state.item_types))
    {
      Util::errorPrint("Can't get directory listing");
      return;
    }
    state.last_listed_directory = state.current_directory;

    // filter out non music files (keep the /)
    std::vector<std::string> music_fext = {"mp3", "m4a", "flac", "wav"};

    for (size_t i = 0; i < state.item_uris.size();)
    {
      std::string uri = state.item_uris[i];
      size_t dot = uri.find_last_of('.');

      bool is_music = false;

      if (dot != std::string::npos)
      {
        std::string ext = uri.substr(dot + 1);

        for (const auto &mtype : music_fext)
        {
          if (ext == mtype)
          {
            is_music = true;
            break;
          }
        }
      }

      if (!is_music)
      {
        // if not a directory
        if (!state.item_uris[i].ends_with('/'))
        {
          // need to filter the itemtype too
          state.item_types.erase(state.item_types.begin() + i);
          state.item_uris.erase(state.item_uris.begin() + i);
        }
        else
        {
          ++i;
        }
      }
      else
      {
        ++i;
      }
    }

    // we want to scroll based on page count
    int offset = (state.page - 1) * state.items_per_page;

    for (size_t i = offset; i < state.item_uris.size() && static_cast<int>(i) < offset + state.items_per_page; i++)
    {
      int row = i - offset;

      // if state.selected_index is past 'screen 1' of items, go to screen 2,
      // but we want screen n -> n+1 and if we go back n+1 -> n
      // we know we can only display a max char of getmaxy(state.main_area)

      // print out the selected one with highlighting
      if (state.selected_index == static_cast<int>(i))
      {
        wattron(state.main_area, A_REVERSE | A_BOLD);
        mvwprintw(state.main_area, row + 3, 2, "%s", state.item_uris[i].c_str());
        wattroff(state.main_area, A_REVERSE | A_BOLD);
      }
      else
      {
        wattroff(state.main_area, A_REVERSE);
        mvwprintw(state.main_area, row + 3, 2, "%s", state.item_uris[i].c_str());
      }
    }

    mvwprintw(state.main_area, 1, state.max_cols - 30, "[Enter to save, Esc to cancel]");
    wrefresh(state.main_area);
  }
}

/* *
 * @brief footer shows state of play paused or stopped. also shows animation
 * for music playing. current time in song and total song length.
 * * */
void UIManager::updateFooter()
{
  werase(state.footer);
  box(state.footer, 0, 0);

  auto *current = state.player.getCurrentSong();
  if (current != nullptr)
  {
    mvwprintw(state.footer, 1, 2, "Now Playing: %s - %s", current->artist_name.c_str(), current->song_name.c_str());
  }
  else
  {
    mvwprintw(state.footer, 1, 2, "Ready");
  }

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

    // left side album art
    int album_width = (state.max_cols / 2) - 2;
    int album_height = state.max_rows - 7;

    auto *current = state.player.getCurrentSong();
    if (current != nullptr)
    {
      // update album art ?
      if (state.cached_song_path != current->song_path)
      {
        state.cached_song_path = current->song_path;

        if (!current->cached_image.pixels.empty())
        {
          state.current_art = Art::Generate(current->cached_image, state.art_color_mode, state.art_render_mode,
                                            album_width, album_height);
        }
        else
        {
          // no image
          state.current_art = Art::AsciiCanvas();
        }
      }

      // Draw the art
      if (!state.current_art.cells.empty())
      {
        int album_y_offset = 1 + (album_height - state.current_art.height) / 2;
        if (album_y_offset < 1)
        {
          album_y_offset = 1;
        }
        drawAsciiArt(state.main_area, album_y_offset, 2, state.current_art);
      }
      else
      {
        mvwprintw(state.main_area, 2, 2, "[no cover art]");
      }

      // right side, song info
      int info_x = album_width + 4;
      int info_y = 2;

      mvwprintw(state.main_area, info_y, info_x, "Now Playing:");
      mvwprintw(state.main_area, info_y + 1, info_x, "%s", current->song_name.c_str());
      mvwprintw(state.main_area, info_y + 3, info_x, "Artist");
      mvwprintw(state.main_area, info_y + 4, info_x, "%s", current->artist_name.c_str());

      // progress bar
      int pos = state.player.getCurrentPositionSeconds();
      int total = state.player.getSongLengthSeconds();
      int percent = state.player.getProgressPercent();

      int bar_width = state.max_cols - info_x - 4;
      int filled = (bar_width * percent) / 100;

      std::string bar;
      for (int i = 0; i < bar_width; ++i)
      {
        bar += (i < filled) ? "█" : "░";
      }

      mvwprintw(state.main_area, info_y + 7, info_x, "%s", bar.c_str());
      mvwprintw(state.main_area, info_y + 8, info_x, "%s / %s", Util::formatDuration(pos).c_str(),
                Util::formatDuration(total).c_str());
    }
    else
    {
      mvwprintw(state.main_area, 2, 2, "No Track Playing");
    }

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
  nodelay(stdscr, true);

  while ((ch = getch()) != 'q')
  {
    napms(5); // sleep 16ms
    // tab switching
    if (ch == KEY_LEFT || ch == 'h')
    {
      int current = static_cast<int>(state.current_tab);
      current = (current == 0) ? 3 : current - 1;
      state.current_tab = static_cast<Tab>(current);
    }
    if (ch == KEY_RIGHT || ch == 'l')
    {
      int current = static_cast<int>(state.current_tab);
      current = (current == 3) ? 0 : current + 1;
      state.current_tab = static_cast<Tab>(current);
    }

    // Music controls
    if (ch == 'p' || ch == 'P')
    {
      state.player.pauseSong();
    }
    if (ch == ']')
    {
      state.player.nextSong();
    }
    if (ch == '[')
    {
      state.player.prevSong();
    }
    if (ch == KEY_BACKSPACE)
    {
      state.player.clearQueue();
    }

    if (ch == 'a' || ch == 'A')
    {
      switch (state.art_color_mode)
      {
      case Art::ColorMode::ANSI_256:
        state.art_color_mode = Art::ColorMode::GRAYSCALE;
        break;
      case Art::ColorMode::GRAYSCALE:
        state.art_color_mode = Art::ColorMode::ANSI_256;
        break;
      }
      // force art regen on next frame
      state.cached_song_path.clear();
    }

    if (ch == 'z' || ch == 'Z')
    {
      switch (state.art_render_mode)
      {
      case Art::RenderMode::BLOCK:
        state.art_render_mode = Art::RenderMode::DETAILED;
        break;
      case Art::RenderMode::DETAILED:
        state.art_render_mode = Art::RenderMode::BLOCK;
        break;
      }
      // force art regen on next frame
      state.cached_song_path.clear();
    }

    // directory browser input
    if (state.current_tab == Tab::directory)
    {
      switch (ch)
      {
      case 'k':
      case KEY_UP:
        if (state.selected_index > 0)
        {
          state.selected_index--;
        }
        state.page = (state.selected_index / state.items_per_page) + 1;
        break;
      case 'j':
      case KEY_DOWN:
        if (state.selected_index < static_cast<int>(state.item_uris.size()) - 1)
        {
          state.selected_index++;
        }
        state.page = (state.selected_index / state.items_per_page) + 1;
        break;
      case 27: // ESC
      case '-':
        state.current_directory = getParentDirectory(state.current_directory);

        if (state.current_directory == "")
        {
          state.current_directory = "/";
        }

        state.selected_index = 0;
        state.page = 1;
        break;
      case '\n':

        // select subdir or song
        if (state.selected_index >= 0 && (size_t)state.selected_index < state.item_uris.size())
        {
          if (state.item_types[state.selected_index] == ItemType::Directory)
          {
            // append dir to curr dir, we go down by 1
            if (state.current_directory == "/")
            {
              state.current_directory.append(state.item_uris[state.selected_index]);
              state.current_directory.pop_back(); // remove trailing /
              state.page = 1;
              state.selected_index = 0;
              break;
            }

            state.current_directory.append("/" + state.item_uris[state.selected_index]);
            state.current_directory.pop_back(); // remove trailing /
            state.selected_index = 0;
            state.page = 1;

            Util::debugPrint("current_dir: " + state.current_directory);
          }
          else
          {
            // check if it ends with any applicable formats
            std::string song_file = state.item_uris[state.selected_index];
            if (song_file.ends_with(".mp3") || song_file.ends_with(".flac") || song_file.ends_with(".wav"))
            {
              // add this song to the queue
              SongMetadata song;
              song.song_path = state.current_directory + "/" + state.item_uris[state.selected_index];
              Util::debugPrint("song_path: " + song.song_path);

              // ask taglib to process artist name and image
              TagLib::FileRef f(song.song_path.c_str());

              song.song_name = std::string(f.tag()->title().toCString());
              song.artist_name = std::string(f.tag()->artist().toCString());

              // try to get image embedded in TagLib
              auto pictures = f.tag()->complexProperties("PICTURE");
              if (!pictures.isEmpty())
              {
                // extract the first picture
                auto &pic = pictures.front();
                auto it = pic.find("data");
                if (it != pic.end() && it->second.type() == TagLib::Variant::ByteVector)
                {
                  TagLib::ByteVector bv = it->second.value<TagLib::ByteVector>();
                  Art::LoadImageMemory(song.cached_image, reinterpret_cast<const uint8_t *>(bv.data()), bv.size());
                }
              }
              else
              {
                // resolve with external cover image in path
                song.album_image_path = Art::ResolveImage(song.song_path);
                if (song.album_image_path != "no image")
                {
                  Art::LoadImageFile(song.cached_image, song.album_image_path);
                }
              }

              song.album_image_path = "unknown";

              bool was_empty = state.player.isEmpty();
              state.player.queueSong(song);

              // If this is the first song, start playing it immediately
              if (was_empty)
              {
                state.player.nextSong();
              }
            }
            else
            {
              // not a valid file
              mvwprintw(state.main_area, 10, 2, "Not a supported music file");
            }
          }
          Util::debugPrint("current_dir: " + state.current_directory);
        }
        break;
      }
    }

    // queue scroll
    if (state.current_tab == Tab::queue)
    {
      size_t qsize = state.player.getQueueSize();
      if (ch == KEY_UP && qsize > 0)
      {
        state.queue_ctr = (state.queue_ctr + qsize - 1) % qsize;
      }
      else if (ch == KEY_DOWN && qsize > 0)
      {
        state.queue_ctr = (state.queue_ctr + 1) % qsize;
      }
    }

    // Auto-advance: if current song ended, play the next one
    if (state.player.isCurrentEnded() || state.player.isAtEnd())
    {
      Util::debugPrint("Song ended, advancing to next");
      state.player.nextSong();
    }

    updateHeader();
    updateMainArea();
    updateFooter();
  }

  Util::debugPrint("User has quit TUI");
  return;
}
