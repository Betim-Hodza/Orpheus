#include "../include/ui.h"
#include <mpd/connection.h>
#include <mpd/player.h>
#include <mpd/response.h>
#include <mpd/status.h>
#include <ncurses.h>
#include <unistd.h>


#define STB_IMAGE_IMPLEMENTATION
#include "../include/stb_image.h"

/**
 * @brief Initializes all of necessary UI variables for TUI
 * 
 * @param starting_directory 
 * @param ui 
 */
void init_ui(const char *starting_directory, UI *ui)
{
  getmaxyx(stdscr, ui->max_rows, ui->max_cols);

  // create new windows
  ui->header = newwin(3, ui->max_cols, 0, 0);
  ui->main_area = newwin(ui->max_rows - 5, ui->max_cols, 3, 0);
  ui->directory_selection = newwin(2, ui->max_cols, ui->max_rows - 4, 0);
  ui->footer = newwin(2, ui->max_cols, ui->max_rows - 2, 0);

  // directory setup
  ui->current_directory = strdup(starting_directory ? starting_directory : "");
  ui->item_uris = malloc(MAX_ITEMS * sizeof(char*));
  ui->item_types = malloc(MAX_ITEMS * sizeof(int));
  // check for malloc failures
  if (!ui->item_uris || !ui->item_types) 
  {
    fprintf(stderr, "Memory allocation failed\n");
    endwin();
    exit(1);
  }
  
  ui->item_count = 0;
  ui->selected_index = 0;
  
  // bools and where we are
  ui->show_directory_browser = false;
  ui->show_help = false;
  ui->show_directory_selection = false;
  ui->current_tab = home;

  // current path and input buffer
  strncpy(ui->input_buffer, starting_directory ? starting_directory : "", MAX_PATH - 1);
  ui->input_buffer[MAX_PATH - 1] = '\0';
  ui->input_pos = strlen(ui->input_buffer);
}

/**
 * @brief Cleans up TUI heap memory 
 * 
 * @param ui 
 */
void clean_tui(UI* ui)
{
  // after done looping clean up
  free(ui->current_directory);
  for (int i = 0; i < ui->item_count; i++)
  {
    if (ui->item_uris[i]) free(ui->item_uris[i]);
  }

	// free up ascii art 
	if (ui->ascii_art) 
	{
		for (int i = 0; i < ui->ascii_height; i++) 
		{
			free(ui->ascii_art[i]);
		}
		free(ui->ascii_art);
	}
	if (ui->cached_image_path) {
			free(ui->cached_image_path);
	}


  free(ui->item_uris);
  free(ui->item_types);
  delwin(ui->header);
  delwin(ui->main_area);
  delwin(ui->directory_selection);
  delwin(ui->footer);
  endwin();
}


/**
 * @brief changes the time in UI headeralong with displaying which tab you're in
 * 
 * @param ui 
 */
void update_header(UI* ui)
{
  time_t now = time(NULL);
  struct tm *tm = localtime(&now);
  char time_str[20];
  strftime(time_str, sizeof(time_str), "%I:%M:%S", tm);
  werase(ui->header);
  
  // display time
  box(ui->header, 0, 0);
  mvwprintw(ui->header, 1, 2, "Time: %s", time_str);

  const char *tab_names[] = {"Home", "Directory", "Queue", "Help"};
  int x_pos = 2;
  for (int i = 0; i < TAB_COUNT; i++) 
  {
    if ((Tab)i == ui->current_tab) 
    {
      // pretty sure this is like bolding
      wattron(ui->header, A_REVERSE);
    }
    mvwprintw(ui->header, 2, x_pos, " %s ", tab_names[i]);
    if ((Tab)i == ui->current_tab) 
    {
      wattroff(ui->header, A_REVERSE);
    }
    x_pos += strlen(tab_names[i]) + 3;
  }
  wrefresh(ui->header);
}

/**
 * @brief for each item we'll update the displayed directory from mpd
 * 
 * @param conn 
 * @param ui 
 */
void update_directory_browser(struct mpd_connection *conn, UI* ui)
{
    // reset everything (assume values are there)
    for (int i = 0; i < MAX_ITEMS; i++) 
    {
      ui->item_uris[i] = NULL;
      ui->item_types[i] = 0;
    }

    // free each item if theres a value
    for (int i = 0; i < ui->item_count; i++)
    {
      if (ui->item_uris[i]) free(ui->item_uris[i]);
    }
    ui->item_count = 0;

    // if we cant receive the list of meta data something went wrong
    if (!mpd_send_list_meta(conn, ui->current_directory[0] ? ui->current_directory : NULL))
    {
      mvwprintw(ui->main_area, 2, 2, "Error sending MPD command");
      wrefresh(ui->main_area);
      return;
    }

    // detect directory and songs, map these strings to item_uris and associate its type
    struct mpd_entity *entity;
    while ((entity = mpd_recv_entity(conn)) != NULL && ui->item_count < MAX_ITEMS)
    {
      if (mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_DIRECTORY)
      {
        const struct mpd_directory *dir = mpd_entity_get_directory(entity);
        ui->item_uris[ui->item_count] = strdup(mpd_directory_get_path(dir));
        ui->item_types[ui->item_count] = 0;
      }
      else if (mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_SONG)
      {
        const struct mpd_song *song = mpd_entity_get_song(entity);
        ui->item_uris[ui->item_count] = strdup(mpd_song_get_uri(song));
        ui->item_types[ui->item_count] = 1;
      }
      mpd_entity_free(entity);
      ui->item_count++;
    }

    // err handling 
    if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS)
    {
      mvwprintw(ui->main_area, 2, 2, "MPD error: %s", mpd_connection_get_error_message(conn));
      mpd_response_finish(conn);
      wrefresh(ui->main_area);
      return;
    }

    mpd_response_finish(conn);

    // draw subdirectories and items
    werase(ui->main_area);
    box(ui->main_area, 0, 0);
    mvwprintw(ui->main_area, 1, 2, "Directory: %s", ui->current_directory);
    
    if (ui->item_count == 0)
    {
      mvwprintw(ui->main_area, 2, 2, "No items found");
    }
    else
    {
      for (int j = 0; j < ui->item_count; j++)
      {
        if (j == ui->selected_index)
        {
          wattron(ui->main_area, A_REVERSE);
        }
        if (ui->item_types[j] == 0)
        {
          mvwprintw(ui->main_area, j + 2, 2, "%s/", ui->item_uris[j]);
        }
        else
        {
          mvwprintw(ui->main_area, j + 2, 2, "%s", ui->item_uris[j]);
        }
        if (j == ui->selected_index)
        {
          wattroff(ui->main_area, A_REVERSE);
        }
      }
    }
    wrefresh(ui->main_area);
}


/**
 * @brief simple wrapper func to update dir selection
 * 
 * @param ui 
 */
void update_directory_selection(UI* ui) 
{
    werase(ui->directory_selection);
    box(ui->directory_selection, 0, 0);
    mvwprintw(ui->directory_selection, 1, 2, "Music Directory: %s", ui->input_buffer);
    mvwprintw(ui->directory_selection, 1, ui->max_cols - 30, "[Enter to save, Esc to cancel]");
    wrefresh(ui->directory_selection);
}

/**
 * @brief Simple wrapper to display help when needed
 * 
 * @param ui 
 */
void help_screen(UI *ui)
{
    werase(ui->main_area);
    box(ui->main_area, 0, 0);

    mvwprintw(ui->main_area, 1, 2, "Main Help:");
    mvwprintw(ui->main_area, 2, 2, "P              | Play/Pause");
    mvwprintw(ui->main_area, 3, 2, "'[' ']'        | Prev song, Next song");
    mvwprintw(ui->main_area, 4, 2, "<LEFT> <RIGHT> | Move to tabs left or right (cycles)");
    mvwprintw(ui->main_area, 6, 2, "Directory Help:");
    mvwprintw(ui->main_area, 7, 2, "<UP> <DOWN>    | Scrolls up and down a list");
    mvwprintw(ui->main_area, 8, 2, "U              | Goes up a directory]");
    mvwprintw(ui->main_area, 9, 2, "<ENTER>        | Goes down a directory and adds song to que");
    wrefresh(ui->main_area);
}

/**
 * @brief Updates the main area based on the current tab
 * 
 * @param conn 
 * @param ui 
 */
void update_main_area(struct mpd_connection *conn, UI* ui) 
{
  werase(ui->main_area);
  box(ui->main_area, 0, 0);
  if (ui->show_directory_browser) 
  {
    update_directory_browser(conn, ui);
  }
  else if (ui->show_help)
  {
    help_screen(ui);
  }
  else 
  {
    mvwprintw(ui->main_area, 1, 2, "Orpheus - C-based Music Player");


    // Display current song info
    struct mpd_song *song = mpd_run_current_song(conn);
    if (song == NULL) 
    {
      mvwprintw(ui->main_area, 15, 2, "No Song currently playing");
    } 
    else
    {
      const char *artist = mpd_song_get_tag(song, MPD_TAG_ARTIST, 0);
      const char *title = mpd_song_get_tag(song, MPD_TAG_TITLE, 0);
      const char *album = mpd_song_get_tag(song, MPD_TAG_ALBUM, 0);
      mvwprintw(ui->main_area, 15, 2, "Artist: %s", artist ? artist : "Unknown");
      mvwprintw(ui->main_area, 16, 2, "Title: %s", title ? title : "Unknown");
      mvwprintw(ui->main_area, 17, 2, "Album: %s", album ? album : "Unknown");

			if (!mpd_send_status(conn))
			{
				mvwprintw(ui->footer, 18, 2, "MPD command error");
				wrefresh(ui->footer);
				return;
			}

			struct mpd_status *status = mpd_recv_status(conn);
			if (status)
			{
				if (mpd_status_get_state(status) == MPD_STATE_PLAY || mpd_status_get_state(status)==MPD_STATE_PAUSE)
				{
					// get song uri 
					const char *song_uri = mpd_song_get_uri(song);

					if (song_uri == NULL)
					{
						mvwprintw(ui->main_area, 18, 2, "No song uri");
					}
					else 
					{
						// for now we just want to rec songs
						image_to_ascii(ui, "/home/bay/Pictures/wallpapers/gojo_blue.png");
						// get album art 

						//size_t size;
						//void *buffer = malloc(2024* 2024);
						//if (buffer == NULL)
						//{
						//	mvwprintw(ui->main_area, 18, 2, "buffer alloc failed");
						//}
						//else 
						//{
						//	ssize_t read_size = mpd_run_albumart(conn, song_uri, 0, buffer, size);
						//	
						//	if (read_size >= 0 && size > 0) 
						//	{
						//		// sav album art temp
						//		FILE *fp = fopen("/tmp/orpheus_cover.jpg", "wb");
						//		if(fp) 
						//		{
						//			fwrite(buffer, 1, size, fp);
						//			fclose(fp);

						//			// display ascii 
						//			image_to_ascii(ui, "/tmp/orpheus_cover.jpg");
						//		}
						//		else 
						//		{
						//			mvwprintw(ui->main_area, 18, 2, "Failed to save album art");
						//		}
						//	}
						//	else 
						//	{
						//		mvwprintw(ui->main_area, 18, 2, "No album art available");
						//	}

						//	free(buffer);
						//}
						

						// pull current album art
						//if ( mpd_send_albumart(conn, song_uri,0))
						//{
						//	//mpd_recv_albumart(struct mpd_connection *connection, void *buffer, size_t buffer_size)

						//	// Display ASCII art
						//	image_to_ascii(ui, "/home/bay/Pictures/wallpapers/kurukuru.gif");

						//}
					}
				}

				mpd_status_free(status);
			}
			else
			{
				mvwprintw(ui->footer, 1, 2, "No status");
			}


      mpd_song_free(song);
    }
  }
  wrefresh(ui->main_area);
}

/**
 * @brief Update footer with state of mpd
 * 
 * @param conn 
 * @param ui 
 * @return update 
 */
void update_footer(struct mpd_connection *conn, UI* ui)
{
  werase(ui->footer);
  box(ui->footer, 0, 0);

  if (!mpd_send_status(conn))
  {
    mvwprintw(ui->footer, 1, 2, "MPD command error");
    wrefresh(ui->footer);
    return;
  }

  struct mpd_status *status = mpd_recv_status(conn);
  if (status)
  {
    switch (mpd_status_get_state(status))
    {
      case MPD_STATE_PLAY:
        // visualizer movement
        static int pos = 0;
        pos = (pos + 1) % (ui->max_cols - 4);                        
        mvwprintw(ui->footer, 1, 2 + pos, "|");
        break;
      case MPD_STATE_PAUSE:
        mvwprintw(ui->footer, 1, 2, "Paused");
        break;
      case MPD_STATE_STOP:
        mvwprintw(ui->footer, 1, 2, "Stopped");
        break;
      default:
        mvwprintw(ui->footer, 1, 2, "Unknown (default switch)");
        break;
    }
    mpd_status_free(status);
  }
  else
  {
    mvwprintw(ui->footer, 1, 2, "No status");
  }

  if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS)
  {
    mvwprintw(ui->footer, 1, 2, "MPD error: %s", mpd_connection_get_error_message(conn));
  }

  mpd_response_finish(conn);
  wrefresh(ui->footer);
}

/**
 * @brief Main tui loop that gets user input, default is main screen but user can switch screens
 *        This is what is called by main 
 * @param conn 
 * @param ui 
 */
void run_tui(struct mpd_connection *conn, UI* ui) 
{
  int current_win = 0; 
  int ch;
  timeout(500);
  
  // while we haven't quit
  while ((ch = getch()) != 'q') 
  {
    // looping window tabs
    if (ch == KEY_LEFT)
    {
      if (current_win == 0)
      {
        current_win = 3;
      }
      else 
      {
        current_win -= 1;
      }
    }
    if (ch == KEY_RIGHT)
    {
      if (current_win == 3)
      {
        current_win = 0;
      }
      else
      {
        current_win += 1;
      }
    }

    // play / pause status
    if (ch == 'p')
    {
      if (!mpd_send_status(conn))
      {
        mvwprintw(ui->main_area, ui->item_count + 2, 2, "Failed to get status: %s", 
        mpd_connection_get_error_message(conn));
        wrefresh(ui->main_area);
        mpd_response_finish(conn);
        break;
      }

        // get status to switch play / pause
        struct mpd_status *status = mpd_recv_status(conn);
        if (status)
        {
          switch (mpd_status_get_state(status))
          {
            // switch play -> pause or fail
            case MPD_STATE_PLAY:
              if (mpd_run_pause(conn, true))
              {
                mvwprintw(ui->main_area, ui->item_count + 2, 2, "Paused");
              }
              else
              {
                mvwprintw(ui->main_area, ui->item_count + 2, 2, "Failed to pause: %s", 
                mpd_connection_get_error_message(conn));
              }
              break;
            case MPD_STATE_PAUSE:
              // switch stop -> play and pause -> play (same action)
              case MPD_STATE_STOP:
              if (mpd_run_play(conn))
              {
                mvwprintw(ui->main_area, ui->item_count + 2, 2, "Playing");
              }
              else
              {
                mvwprintw(ui->main_area, ui->item_count + 2, 2, "Failed to play: %s", 
                mpd_connection_get_error_message(conn));
              }
              break;
                // unknown state
            default:
              mvwprintw(ui->main_area, ui->item_count + 2, 2, "Unknown playback state");
              break;
          }
          mpd_status_free(status);
        }
        else
        {
          mvwprintw(ui->main_area, ui->item_count + 2, 2, "No status available");
        }
        mpd_response_finish(conn);
        wrefresh(ui->main_area);            
    }

    // skip to next song
    else if (ch == ']') 
    {
      if (!mpd_run_next(conn))
      {
        mvwprintw(ui->main_area, 10, 10, "Error skipping song: %s", mpd_connection_get_error_message(conn));
      }
    }
    else if (ch == '[')
    {
      if (!mpd_run_previous(conn))
      {
        mvwprintw(ui->main_area, 10, 10, "Error running prev song: %s", mpd_connection_get_error_message(conn));
      }

    }

    // once we enter in the directory browser we can search for music in the user defined dir
    else if (ui->show_directory_browser) 
    {
      // explore directory
      switch (ch) 
      {
        // scroll up and down 
        case KEY_UP:
          if (ui->selected_index > 0) ui->selected_index--;
          break;
        case KEY_DOWN:
          if (ui->selected_index < ui->item_count - 1) ui->selected_index++;
          break;

        // select a directory or add song to queue
        case '\n':
          if (ui->item_count == 0 || ui->selected_index < 0 || ui->selected_index >= ui->item_count)
          {
            break; // No valid selection
          }
          // go down directory
          if (ui->item_types[ui->selected_index] == 0) 
          {
            char *new_dir = ui->item_uris[ui->selected_index];
            free(ui->current_directory);
            ui->current_directory = strdup(new_dir);
            ui->selected_index = 0;
          }
          // add song to queue
          else 
          {
            if (mpd_run_add(conn, ui->item_uris[ui->selected_index]))
            {
              mvwprintw(ui->main_area, ui->item_count + 2, 2, "Song added");
              if (!mpd_run_play(conn))
              {
                mvwprintw(ui->main_area, ui->item_count + 3, 2, "Failed to play: %s", 
                mpd_connection_get_error_message(conn));
              }
            }
            else
            {
              mvwprintw(ui->main_area, ui->item_count + 2, 2, "Failed to add song: %s", 
              mpd_connection_get_error_message(conn));
            }
            wrefresh(ui->main_area);
            mpd_response_finish(conn);    
          }
          break;
            // go up a dir
        case 'u':
          char *parent = get_parent_directory(ui->current_directory);
          if (parent) 
          {
            free(ui->current_directory);
            ui->current_directory = parent;
            ui->selected_index = 0;
          }
          break;
      }
    }
    else if (ui->show_directory_selection) 
    {
      if (ch == '\n') 
      {
        free(ui->current_directory);
        ui->current_directory = strdup(ui->input_buffer);
        ui->show_directory_selection = false;
      }
      else if (ch == 27) 
      {   
        // Esc
        strncpy(ui->input_buffer, ui->current_directory, MAX_PATH - 1);
        ui->input_buffer[MAX_PATH - 1] = '\0';
        ui->input_pos = strlen(ui->input_buffer);
        ui->show_directory_selection = false;
      }
      else if (ch == KEY_BACKSPACE || ch == 127) 
      {
        if (ui->input_pos > 0) 
        {
          ui->input_buffer[--ui->input_pos] = '\0';
        }
      } 
      else if (ch >= 32 && ch <= 126 && ui->input_pos < MAX_PATH - 1) 
      {
        ui->input_buffer[ui->input_pos++] = ch;
        ui->input_buffer[ui->input_pos] = '\0';
      }
    }

    // update current window
    switch (current_win) 
    {
      // home 
      case 0:
        ui->show_directory_browser = false;
        // ui->show_playlists = false
        ui->show_help = false;
        break;
      // directory
      case 1:
        ui->show_directory_browser = true;
        ui->show_help = false;
        // ui->show_playlists = false 
        break;
      // playlist
      case 2:
        ui->show_directory_browser = false;
        // ui->show_playlists = true
        ui->show_help = false;
        break;
      // help
      case 3:
        ui->show_directory_browser = false;
        ui->show_help = true;
        // ui->show_playlists = false
        break;
      default:
        break;
    }
    ui->current_tab = current_win;

    update_header(ui);
    update_main_area(conn, ui);
    update_footer(conn, ui);
  }
}

/**
 * @brief Get the parent directory object
 * 
 * @param path 
 * @return char* 
 */
char *get_parent_directory(const char *path)
{
  if (strlen(path) == 0)
  {
    return NULL;
  }

  char *last_slash = strrchr(path, '/');

  if (last_slash == NULL)
  {
    return strdup("");
  }

  size_t len = last_slash - path;
  char *parent = malloc(len + 1);
  strncpy(parent, path, len);
  parent[len] = '\0';

  return parent;
}


/**
 * @brief Display ASCII art in the ncurses window
 *
 * @param win The ncurses window to render the ASCII art
 * @param image_path Path to the image file
 * @return none
 */
void image_to_ascii(UI *ui, const char *image_path) 
{
	const char *chars = "`^\",:;Il!i~+_-?][}(1)(|\\/tfjrxnuvczXYUJCLQ0OZmwqpdbkhao#MW&8%B@S";
	const int charsLen = strlen(chars);
	
	// Check if we need to reload the image
	bool need_reload = false;
	if (ui->cached_image_path == NULL || strcmp(ui->cached_image_path, image_path) != 0) 
	{
		need_reload = true;
	}
	
	// Get current window dimensions
	int win_height, win_width;
	getmaxyx(ui->main_area, win_height, win_width);
	int ascii_width = win_width / 2;
	int ascii_height = (win_height - 4) > 0 ? (win_height - 4) : 1;
	
	// Check if window size changed
	if (ui->ascii_art != NULL && (ascii_width != ui->ascii_width || ascii_height != ui->ascii_height)) 
	{
		need_reload = true;
	}
	
	// Load and convert image if needed
	if (need_reload) 
	{
		// Free old ASCII art if it exists
		if (ui->ascii_art) 
		{
			for (int i = 0; i < ui->ascii_height; i++) 
			{
				free(ui->ascii_art[i]);
			}
			free(ui->ascii_art);
			ui->ascii_art = NULL;
		}
		
		// Load image
		int width, height, channels;
		unsigned char *image_data = stbi_load(image_path, &width, &height, &channels, 0);
		if (!image_data) 
		{
			mvwprintw(ui->main_area, 2, 2, "Failed to load image: %s", image_path);
			return;
		}
		
		// Allocate memory for ASCII art
		ui->ascii_art = malloc(ascii_height * sizeof(char*));
		for (int i = 0; i < ascii_height; i++) 
		{
			ui->ascii_art[i] = malloc((ascii_width + 1) * sizeof(char));
		}
		ui->ascii_width = ascii_width;
		ui->ascii_height = ascii_height;
		
		// Update cached path
		if (ui->cached_image_path) 
		{
			free(ui->cached_image_path);
		}
		ui->cached_image_path = strdup(image_path);
		
		// Convert image to ASCII and store in array
		for (int y = 0; y < ascii_height && y * height / ascii_height < height; y++) 
		{
			for (int x = 0; x < ascii_width && x * width / ascii_width < width; x++) 
			{
				int img_x = (x * width) / ascii_width;
				int img_y = (y * height) / ascii_height;
				int idx = (img_y * width + img_x) * channels;
				
				unsigned char r = image_data[idx];
				unsigned char g = image_data[idx + 1];
				unsigned char b = image_data[idx + 2];
				
				float avg = (r + g + b) / 3.0f;
				int char_idx = (int)(charsLen * (avg / 255.0f));
				char_idx = char_idx < 0 ? 0 : (char_idx >= charsLen ? charsLen - 1 : char_idx);
				
				ui->ascii_art[y][x] = chars[char_idx];
			}
			ui->ascii_art[y][ascii_width] = '\0';
		}
			
		stbi_image_free(image_data);
	}
	
	// Always render the cached ASCII art
	if (ui->ascii_art) 
	{
		// Clear the area first
		for (int y = 2; y < ui->ascii_height + 2; y++) 
		{
			mvwprintw(ui->main_area, y, 2, "%*s", win_width - 4, "");
		}
		
		// Render from cache
		for (int y = 0; y < ui->ascii_height; y++) 
		{
			for (int x = 0; x < ui->ascii_width; x++) 
			{
				mvwprintw(ui->main_area, y + 2, x * 2 + 2, "%c", ui->ascii_art[y][x]);
			}
		}
	}
}
