#include "../include/ui.h"
#include <mpd/connection.h>
#include <mpd/player.h>
#include <mpd/response.h>
#include <ncurses.h>

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

    mvwprintw(ui->main_area, 1, 2, "Help:");
    mvwprintw(ui->main_area, 2, 2, "P              | Play/Pause");
    mvwprintw(ui->main_area, 3, 2, "<LEFT> <RIGHT> | Move to tabs left or right (cycles)");
    mvwprintw(ui->main_area, 5, 2, "Directory Help:");
    mvwprintw(ui->main_area, 6, 2, "<UP> <DOWN>    | Scrolls up and down a list");
    mvwprintw(ui->main_area, 7, 2, "U              | Goes up a directory]");
    mvwprintw(ui->main_area, 8, 2, "<ENTER>        | Goes down a directory and adds song to que");
    wrefresh(ui->main_area);
}

/**
 * @brief Main screen that calls other screen sections when user switches
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
    mvwprintw(ui->main_area, 1, 2, "Orpeus - C-based Music Player");
    
    // Fetch and display album art as ASCII
    const char *temp_file = "/tmp/orpeus_album_art.jpg";
    if (fetch_album_art(conn, temp_file) == 0) 
    {
      // Convert JPEG to ASCII art
      int ascii_width = ui->max_cols - 4; // Fit within window borders
      int max_height = ui->max_rows - 7; // Leave space for text and borders
      AsciiArt *art = jpeg_to_ascii(temp_file, ascii_width, max_height);
      
      if (art && art->num_lines > 0) 
      {
        // Display ASCII art centered in the main area
        int start_y = 3; // Start below the title
        int start_x = (ui->max_cols - art->max_width) / 2; // Center horizontally
        for (int i = 0; i < art->num_lines && start_y + i < ui->max_rows - 4; i++) 
        {
          mvwprintw(ui->main_area, start_y + i, start_x, "%s", art->lines[i]);
        }
        ascii_art_free(art);
      } 
      else 
      {
        mvwprintw(ui->main_area, 3, 2, "No album art available or conversion failed");
      }

      // Clean up temporary file
      unlink(temp_file);
    } 
    else
    {
      mvwprintw(ui->main_area, 3, 2, "Failed to fetch album art");
    }

    // Display current song info
    if (mpd_send_current_song(conn)) 
    {
      struct mpd_song *song = mpd_recv_song(conn);
      if (song) 
      {
        mvwprintw(ui->main_area, ui->max_rows - 5, 2, "Now Playing: %s - %s",
          mpd_song_get_tag(song, MPD_TAG_ARTIST, 0) ? 
          mpd_song_get_tag(song, MPD_TAG_ARTIST, 0) : "Unknown Artist",
          mpd_song_get_tag(song, MPD_TAG_TITLE, 0) ? 
          mpd_song_get_tag(song, MPD_TAG_TITLE, 0) : "Unknown Title");
        mpd_song_free(song);
      }
      mpd_response_finish(conn);
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
 * @brief Fetches album art for the current song and saves it to a temporary file
 * @param conn MPD connection
 * @param temp_file Path to save the temporary JPEG file
 * @return 0 on success, -1 on failure
 */
int fetch_album_art(struct mpd_connection *conn, const char *temp_file) 
{
  // Clear any prior errors
  mpd_connection_clear_error(conn);

  // Get current song
  if (!mpd_send_current_song(conn)) 
  {
    fprintf(stderr, "Failed to send current song command: %s\n", 
            mpd_connection_get_error_message(conn));
    mpd_response_finish(conn);
    return -1;
  }

  struct mpd_song *song = mpd_recv_song(conn);
  if (!song) 
  {
    fprintf(stderr, "No current song or error retrieving it: %s\n",
            mpd_connection_get_error_message(conn));
    mpd_response_finish(conn);
    return -1;
  }

  const char *song_uri = mpd_song_get_uri(song);
  if (!song_uri) 
  {
    fprintf(stderr, "No URI for current song\n");
    mpd_song_free(song);
    mpd_response_finish(conn);
    return -1;
  }

  char *song_uri_copy = strdup(song_uri);  // Copy to avoid const issues
  mpd_song_free(song);
  mpd_response_finish(conn);

  // Send readpicture command for the song
  if (!mpd_send_readpicture(conn, song_uri_copy, 0)) 
  {
    fprintf(stderr, "Failed to send readpicture command: %s\n", 
            mpd_connection_get_error_message(conn));
    free(song_uri_copy);
    mpd_response_finish(conn);
    return -1;
  }

  // Open temporary file for writing
  FILE *fp = fopen(temp_file, "wb");
  if (!fp) 
  {
    fprintf(stderr, "Failed to open temporary file %s: %s\n", temp_file, strerror(errno));
    free(song_uri_copy);
    mpd_response_finish(conn);
    return -1;
  }

  // Allocate buffer for reading picture data
  const size_t buffer_size = 8192;  // Default chunk size as per documentation
  char *buffer = malloc(buffer_size);
  if (!buffer) 
  {
    fprintf(stderr, "Failed to allocate buffer for binary data\n");
    fclose(fp);
    unlink(temp_file);
    free(song_uri_copy);
    mpd_response_finish(conn);
    return -1;
  }

  // Read picture data in chunks
  size_t total_bytes_written = 0;
  int bytes_read;
  while ((bytes_read = mpd_recv_readpicture(conn, buffer, buffer_size)) > 0) 
  {
    size_t written = fwrite(buffer, 1, bytes_read, fp);
    if (written != (size_t)bytes_read) 
    {
      fprintf(stderr, "Failed to write to temp file: %s\n", strerror(errno));
      free(buffer);
      fclose(fp);
      unlink(temp_file);
      free(song_uri_copy);
      mpd_response_finish(conn);
      return -1;
    }
    total_bytes_written += written;
  }

  free(buffer);
  fclose(fp);
  mpd_response_finish(conn);

  // Check if we read any data
  if (total_bytes_written == 0) 
  {
    fprintf(stderr, "No picture data available for %s\n", song_uri_copy);
    free(song_uri_copy);
    unlink(temp_file);
    return -1;
  }

  free(song_uri_copy);

  // Final error check
  if (bytes_read < 0 || mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) 
  {
    fprintf(stderr, "MPD error after fetching picture: %s\n", 
            mpd_connection_get_error_message(conn));
    unlink(temp_file);
    return -1;
  }

  return 0;
}