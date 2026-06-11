#include "ui.hpp"
#include "player.hpp"
#include "util.hpp"
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

  state.music_root = music_root.string();
  state.current_directory = music_root.string();
  state.current_tab = Tab::home;

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
    auto* song = state.player.getQueueSong(i);
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

	// only update when we change directories
	if (state.current_directory != state.last_listed_directory || state.current_tab == Tab::directory)
	{
		// err msg inside listDir func
		if (!Util::listDir(state.current_directory, state.item_uris, state.item_types))
		{
			Util::errorPrint("Can't get directory listing");
			return;
		}
		state.last_listed_directory = state.current_directory;

		// print out all the items
		for (size_t i = 0; i < state.item_uris.size(); i++)
		{
			// don't print too many items
			if (i + 3 >= getmaxy(state.main_area))
				break;

			// print out the selected one with highlighting
			if (state.selected_index == i)
			{
				wattron(state.main_area, A_REVERSE | A_BOLD);
				mvwprintw(state.main_area, i + 3, 2, "%s", state.item_uris[i].c_str());
				wattroff(state.main_area, A_REVERSE | A_BOLD);
			}
			else
			{
				wattroff(state.main_area, A_REVERSE);
				mvwprintw(state.main_area, i + 3, 2, "%s", state.item_uris[i].c_str());
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

  auto* current = state.player.getCurrentSong();
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
    mvwprintw(state.main_area, 1, 2, "Orpheus - C++ Music Player");
    if (state.player.getCurrentSong() != nullptr)
    {
      auto* current = state.player.getCurrentSong();
      mvwprintw(state.main_area, 2, 2, "Now Playing: %s", current->song_name.c_str());
      mvwprintw(state.main_area, 3, 2, "Artist: %s", current->artist_name.c_str());
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
        break;
			case 'j':
			case KEY_DOWN:
        if (state.selected_index < static_cast<int>(state.item_uris.size()) - 1)
				{
          state.selected_index++;
				}
        break;
      case 27: // ESC
        state.current_directory = getParentDirectory(state.current_directory);
        state.selected_index = 0;
        break;
      case '\n':

        // select subdir or song
        if (state.selected_index >= 0 && (size_t)state.selected_index < state.item_uris.size())
        {
          if (state.item_types[state.selected_index] == ItemType::Directory)
          {
            // append dir to curr dir, we go down by 1
            state.current_directory.append("/" + state.item_uris[state.selected_index]);
            state.current_directory.pop_back(); // remove trailing /
            state.selected_index = 0;
          }
          else
          {
						// check if it ends with any applicable formats 
						std::string song_file = state.item_uris[state.selected_index];
						if (song_file.ends_with(".mp3") || song_file.ends_with(".flac") || song_file.ends_with(".wav"))
						{
							// add this song to the queue
							SongMetadata song;
							song.song_name = state.item_uris[state.selected_index];
							song.artist_name = "Artist name";
							song.song_path = state.current_directory + "/" + state.item_uris[state.selected_index];
							song.album_image_path = "album image path";

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
    if (state.player.isCurrentEnded())
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
