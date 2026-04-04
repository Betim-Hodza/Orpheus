#include "../include/ui.h"
#include "../include/img_to_ascii.h"
#include "../include/log.h"
#include <mpd/albumart.h>
#include <mpd/connection.h>
#include <mpd/entity.h>
#include <mpd/error.h>
#include <mpd/player.h>
#include <mpd/recv.h>
#include <mpd/response.h>
#include <mpd/status.h>
#include <ncurses.h>
#include <unistd.h>

/**
 * @brief Initializes all of necessary UI variables for TUI
 *
 * @param starting_directory
 * @param ui
 */
void init_ui(const char *starting_directory, UI *ui) {
  log_debug("Initializing UI with starting directory: '%s'",
            starting_directory ? starting_directory : "(null)");

  getmaxyx(stdscr, ui->max_rows, ui->max_cols);
  log_debug("Terminal dimensions: %d rows x %d cols", ui->max_rows,
            ui->max_cols);

  // create new windows
  ui->header = newwin(3, ui->max_cols, 0, 0);
  ui->main_area = newwin(ui->max_rows - 5, ui->max_cols, 3, 0);
  ui->directory_selection = newwin(2, ui->max_cols, ui->max_rows - 4, 0);
  ui->queue_area = newwin(ui->max_rows - 5, ui->max_cols, 3, 0);
  ui->footer = newwin(2, ui->max_cols, ui->max_rows - 2, 0);

  if (!ui->header || !ui->main_area || !ui->directory_selection ||
      !ui->queue_area || !ui->footer) {
    log_fatal("Failed to create one or more ncurses windows");
    endwin();
    exit(1);
  }

  // directory setup
  ui->current_directory = strdup(starting_directory ? starting_directory : "");
  ui->item_uris = malloc(MAX_ITEMS * sizeof(char *));
  ui->item_types = malloc(MAX_ITEMS * sizeof(int));

  if (!ui->item_uris || !ui->item_types) {
    log_fatal("Memory allocation failed for item_uris / item_types");
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
  strncpy(ui->input_buffer, starting_directory ? starting_directory : "",
          MAX_PATH - 1);
  ui->input_buffer[MAX_PATH - 1] = '\0';
  ui->input_pos = strlen(ui->input_buffer);

  log_debug("UI initialized successfully");
}

/**
 * @brief Cleans up TUI heap memory
 *
 * @param ui
 */
void clean_tui(UI *ui) {
  log_debug("Cleaning up UI resources");

  free(ui->current_directory);
  for (int i = 0; i < ui->item_count; i++) {
    if (ui->item_uris[i])
      free(ui->item_uris[i]);
  }

  // free up ascii art
  if (ui->ascii_art) {
    for (int i = 0; i < ui->ascii_height; i++) {
      free(ui->ascii_art[i]);
    }
    free(ui->ascii_art);
  }
  if (ui->cached_image_path) {
    free(ui->cached_image_path);
  }

  unlink("/tmp/orpheus_cover.jpg");
  free(ui->item_uris);
  free(ui->item_types);
  delwin(ui->header);
  delwin(ui->main_area);
  delwin(ui->directory_selection);
  delwin(ui->queue_area);
  delwin(ui->footer);
  endwin();

  log_debug("UI cleanup complete");
}

/**
 * @brief changes the time in UI header along with displaying which tab you're
 * in
 *
 * @param ui
 */
void update_header(UI *ui) {
  time_t now = time(NULL);
  struct tm *tm = localtime(&now);
  char time_str[20];
  strftime(time_str, sizeof(time_str), "%I:%M:%S", tm);

  werase(ui->header);
  box(ui->header, 0, 0);
  mvwprintw(ui->header, 1, 2, "Time: %s", time_str);

  const char *tab_names[] = {"Home", "Directory", "Queue", "Help"};
  int x_pos = 2;
  for (int i = 0; i < TAB_COUNT; i++) {
    if ((Tab)i == ui->current_tab)
      wattron(ui->header, A_REVERSE);
    mvwprintw(ui->header, 2, x_pos, " %s ", tab_names[i]);
    if ((Tab)i == ui->current_tab)
      wattroff(ui->header, A_REVERSE);
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
void update_directory_browser(struct mpd_connection *conn, UI *ui) {
  log_debug("Updating directory browser for path: '%s'", ui->current_directory);

  // reset everything
  for (int i = 0; i < MAX_ITEMS; i++) {
    ui->item_uris[i] = NULL;
    ui->item_types[i] = 0;
  }

  for (int i = 0; i < ui->item_count; i++) {
    if (ui->item_uris[i])
      free(ui->item_uris[i]);
  }
  ui->item_count = 0;

  if (!mpd_send_list_meta(conn, ui->current_directory[0] ? ui->current_directory
                                                         : NULL)) {
    log_error("Failed to send list_meta MPD command for directory '%s': %s",
              ui->current_directory, mpd_connection_get_error_message(conn));
    mvwprintw(ui->main_area, 2, 2, "Error sending MPD command");
    wrefresh(ui->main_area);
    return;
  }

  struct mpd_entity *entity;
  while ((entity = mpd_recv_entity(conn)) != NULL &&
         ui->item_count < MAX_ITEMS) {
    if (mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_DIRECTORY) {
      const struct mpd_directory *dir = mpd_entity_get_directory(entity);
      ui->item_uris[ui->item_count] = strdup(mpd_directory_get_path(dir));
      ui->item_types[ui->item_count] = 0;
    } else if (mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_SONG) {
      const struct mpd_song *song = mpd_entity_get_song(entity);
      ui->item_uris[ui->item_count] = strdup(mpd_song_get_uri(song));
      ui->item_types[ui->item_count] = 1;
    }
    mpd_entity_free(entity);
    ui->item_count++;
  }

  if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
    log_error("MPD error after receiving entities: %s",
              mpd_connection_get_error_message(conn));
    mvwprintw(ui->main_area, 2, 2, "MPD error: %s",
              mpd_connection_get_error_message(conn));
    mpd_response_finish(conn);
    wrefresh(ui->main_area);
    return;
  }

  mpd_response_finish(conn);
  log_debug("Directory browser loaded %d items", ui->item_count);

  werase(ui->main_area);
  box(ui->main_area, 0, 0);
  mvwprintw(ui->main_area, 1, 2, "Directory: %s", ui->current_directory);

  if (ui->item_count == 0) {
    mvwprintw(ui->main_area, 2, 2, "No items found");
  } else {
    for (int j = 0; j < ui->item_count; j++) {
      if (j == ui->selected_index)
        wattron(ui->main_area, A_REVERSE);
      if (ui->item_types[j] == 0)
        mvwprintw(ui->main_area, j + 2, 2, "%s/", ui->item_uris[j]);
      else
        mvwprintw(ui->main_area, j + 2, 2, "%s", ui->item_uris[j]);
      if (j == ui->selected_index)
        wattroff(ui->main_area, A_REVERSE);
    }
  }
  wrefresh(ui->main_area);
}

/**
 * @brief simple wrapper func to update dir selection
 *
 * @param ui
 */
void update_directory_selection(UI *ui) {
  werase(ui->directory_selection);
  box(ui->directory_selection, 0, 0);
  mvwprintw(ui->directory_selection, 1, 2, "Music Directory: %s",
            ui->input_buffer);
  mvwprintw(ui->directory_selection, 1, ui->max_cols - 30,
            "[Enter to save, Esc to cancel]");
  wrefresh(ui->directory_selection);
}

/**
 * @brief Simple wrapper to display help when needed
 *
 * @param ui
 */
void help_screen(UI *ui) {
  werase(ui->main_area);
  box(ui->main_area, 0, 0);

  mvwprintw(ui->main_area, 1, 2, "Main Help:");
  mvwprintw(ui->main_area, 2, 2, "P              | Play/Pause");
  mvwprintw(ui->main_area, 3, 2, "'[' ']'        | Prev song, Next song");
  mvwprintw(ui->main_area, 4, 2,
            "<LEFT> <RIGHT> | Move to tabs left or right (cycles)");
  mvwprintw(ui->main_area, 5, 2, "<BACKSPACE>    | Clear song queue");

  mvwprintw(ui->main_area, 7, 2, "Directory Help:");
  mvwprintw(ui->main_area, 8, 2, "<UP> <DOWN>    | Scrolls up and down a list");
  mvwprintw(ui->main_area, 9, 2, "<ESC>          | Goes up a directory");
  mvwprintw(ui->main_area, 10, 2,
            "<ENTER>        | Goes down a directory and adds song to queue");
  wrefresh(ui->main_area);
}

void queue_screen(struct mpd_connection *conn, UI *ui) {
  int i = 3;
  werase(ui->main_area);
  box(ui->main_area, 0, 0);
  mvwprintw(ui->main_area, 1, 2, "Queue / Playlist:");

  struct mpd_status *status = mpd_run_status(conn);
  if (!status) {
    log_error("Failed to get MPD status in queue_screen: %s",
              mpd_connection_get_error_message(conn));
    mpd_connection_clear_error(conn);
    mvwprintw(ui->main_area, 2, 2, "Failed to get MPD status");
    wrefresh(ui->main_area);
    return;
  }

  ui->total_qsongs = mpd_status_get_queue_length(status);
  mpd_status_free(status);
  log_debug("Queue length: %u", ui->total_qsongs);

  unsigned start, end;
  if (ui->total_qsongs == 0) {
    mvwprintw(ui->main_area, 2, 2, "Queue empty");
    wrefresh(ui->main_area);
    return;
  }

  start = ui->queue_ctr % ui->total_qsongs;
  end = start + (unsigned)(ui->max_rows - 3);
  if (end > ui->total_qsongs)
    end = ui->total_qsongs;

  if (!mpd_send_list_queue_range_meta(conn, start, end)) {
    if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
      log_error("Failed to send list_queue_range_meta (start=%u end=%u): %s",
                start, end, mpd_connection_get_error_message(conn));
      mpd_connection_clear_error(conn);
    }
    mvwprintw(ui->main_area, 2, 2, "Queue empty");
    wrefresh(ui->main_area);
    return;
  }

  struct mpd_entity *entity;
  while ((entity = mpd_recv_entity(conn)) != NULL) {
    if (mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_SONG) {
      struct mpd_song *song = (struct mpd_song *)mpd_entity_get_song(entity);
      unsigned pos = mpd_song_get_pos(song);
      const char *title = mpd_song_get_tag(song, MPD_TAG_TITLE, 0);
      const char *artist = mpd_song_get_tag(song, MPD_TAG_ARTIST, 0);
      mvwprintw(ui->main_area, i, 2, "%u : %s -- %s", pos,
                title ? title : "Unknown", artist ? artist : "Unknown");
      i++;
    }
    mpd_entity_free(entity);
  }

  mpd_response_finish(conn);
  wrefresh(ui->main_area);
}

/**
 * @brief Updates the main area based on the current tab
 *
 * @param conn
 * @param ui
 */
void update_main_area(struct mpd_connection *conn, UI *ui) {
  werase(ui->main_area);
  box(ui->main_area, 0, 0);

  if (ui->show_directory_browser) {
    update_directory_browser(conn, ui);
    return;
  }
  if (ui->show_queue) {
    queue_screen(conn, ui);
    return;
  }
  if (ui->show_help) {
    help_screen(ui);
    return;
  }

  // --- Home screen ---
  mvwprintw(ui->main_area, 1, 2, "Orpheus - C-based Music Player");

  struct mpd_song *song = mpd_run_current_song(conn);
  if (song == NULL) {
    if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
      log_error("Failed to get current song: %s",
                mpd_connection_get_error_message(conn));
      mvwprintw(ui->main_area, 15, 2, "mpd song err: %s",
                mpd_connection_get_error_message(conn));
      mpd_connection_clear_error(conn);
    } else {
      log_debug("No current song playing");
    }
    wrefresh(ui->main_area);
    return;
  }

  const char *artist = mpd_song_get_tag(song, MPD_TAG_ARTIST, 0);
  const char *title = mpd_song_get_tag(song, MPD_TAG_TITLE, 0);
  const char *album = mpd_song_get_tag(song, MPD_TAG_ALBUM, 0);
  const char *song_uri = mpd_song_get_uri(song);

  log_debug("Current song: '%s' by '%s' (album: '%s', uri: '%s')",
            title ? title : "Unknown", artist ? artist : "Unknown",
            album ? album : "Unknown", song_uri ? song_uri : "Unknown");

  mvwprintw(ui->main_area, ui->max_rows - 10, 2, "Artist: %s",
            artist ? artist : "Unknown");
  mvwprintw(ui->main_area, ui->max_rows - 9, 2, "Title: %s",
            title ? title : "Unknown");
  mvwprintw(ui->main_area, ui->max_rows - 8, 2, "Album: %s",
            album ? album : "Unknown");

  struct mpd_status *status = mpd_run_status(conn);
  if (!status) {
    log_error("Failed to get MPD status in update_main_area: %s",
              mpd_connection_get_error_message(conn));
    mpd_connection_clear_error(conn);
    mvwprintw(ui->main_area, ui->max_rows - 7, 2, "No status");
    mpd_song_free(song);
    wrefresh(ui->main_area);
    return;
  }

  enum mpd_state state = mpd_status_get_state(status);
  mpd_status_free(status);

  if (state != MPD_STATE_PLAY && state != MPD_STATE_PAUSE) {
    mpd_song_free(song);
    wrefresh(ui->main_area);
    return;
  }

  if (song_uri == NULL) {
    log_warn("Current song has no URI, cannot fetch album art");
    mvwprintw(ui->main_area, 18, 2, "No song URI");
    mpd_song_free(song);
    wrefresh(ui->main_area);
    return;
  }

  // --- Album art fetch ---
  void *buff = malloc(8192);
  if (!buff) {
    log_error("Failed to allocate album art read buffer");
    mpd_song_free(song);
    wrefresh(ui->main_area);
    return;
  }

  FILE *fp = fopen("/tmp/orpheus_cover.jpg", "wb");
  if (fp == NULL) {
    log_error("Failed to open /tmp/orpheus_cover.jpg for writing");
    free(buff);
    mvwprintw(ui->main_area, 18, 2, "Cannot open temp file");
    mpd_song_free(song);
    wrefresh(ui->main_area);
    return;
  }

  int offset = 0;
  int read_size;
  do {
    read_size = mpd_run_albumart(conn, song_uri, offset, buff, 8192);
    if (read_size == -1) {
      log_warn("Album art not available for '%s': %s", song_uri,
               mpd_connection_get_error_message(conn));
      fclose(fp);
      free(buff);
      mpd_connection_clear_error(conn);
      unlink("/tmp/orpheus_cover.jpg");
      mvwprintw(ui->main_area, 18, 2, "No album art available");
      mpd_song_free(song);
      wrefresh(ui->main_area);
      return;
    }
    fwrite(buff, 1, read_size, fp);
    offset += read_size;
  } while (read_size > 0);

  fclose(fp);
  free(buff);
  log_debug("Album art written to /tmp/orpheus_cover.jpg (%d bytes)", offset);

  image_to_ascii(ui, conn, "/tmp/orpheus_cover.jpg");
  mpd_song_free(song);
  wrefresh(ui->main_area);
}

/**
 * @brief Update footer with state of mpd
 *
 * @param conn
 * @param ui
 */
void update_footer(struct mpd_connection *conn, UI *ui) {
  werase(ui->footer);
  box(ui->footer, 0, 0);

  struct mpd_status *status = mpd_run_status(conn);
  if (!status) {
    log_error("Failed to get MPD status in update_footer: %s",
              mpd_connection_get_error_message(conn));
    mpd_connection_clear_error(conn);
    mvwprintw(ui->footer, 1, 2, "MPD error: no status");
    wrefresh(ui->footer);
    return;
  }

  switch (mpd_status_get_state(status)) {
  case MPD_STATE_PLAY: {
    static int pos = 0;
    pos = (pos + 1) % (ui->max_cols - 4);
    mvwprintw(ui->footer, 1, 2 + pos, "|");
    break;
  }
  case MPD_STATE_PAUSE:
    mvwprintw(ui->footer, 1, 2, "Paused");
    break;
  case MPD_STATE_STOP:
    mvwprintw(ui->footer, 1, 2, "Stopped");
    break;
  default:
    mvwprintw(ui->footer, 1, 2, "Unknown state");
    break;
  }

  mpd_status_free(status);
  wrefresh(ui->footer);
}

/**
 * @brief Main tui loop
 *
 * @param conn
 * @param ui
 */
void run_tui(struct mpd_connection *conn, UI *ui) {
  log_info("Starting TUI event loop");

  int current_win = 0;
  int ch;
  timeout(500);

  while ((ch = getch()) != 'q') {

    // --- Tab switching ---
    if (ch == KEY_LEFT) {
      current_win = (current_win == 0) ? 3 : current_win - 1;
      log_debug("Switched to tab %d (left)", current_win);
    }
    if (ch == KEY_RIGHT) {
      current_win = (current_win == 3) ? 0 : current_win + 1;
      log_debug("Switched to tab %d (right)", current_win);
    }

    // --- Playback controls ---
    if (ch == 'p') {
      struct mpd_status *status = mpd_run_status(conn);
      if (!status) {
        log_error("Failed to get status for play/pause: %s",
                  mpd_connection_get_error_message(conn));
        mpd_connection_clear_error(conn);
      } else {
        enum mpd_state state = mpd_status_get_state(status);
        mpd_status_free(status);

        if (state == MPD_STATE_PLAY) {
          log_info("Pausing playback");
          if (!mpd_run_pause(conn, true)) {
            log_error("Failed to pause: %s",
                      mpd_connection_get_error_message(conn));
            mpd_connection_clear_error(conn);
          }
        } else {
          log_info("Starting playback");
          if (!mpd_run_play(conn)) {
            log_error("Failed to play: %s",
                      mpd_connection_get_error_message(conn));
            mpd_connection_clear_error(conn);
          }
        }
      }
    } else if (ch == ']') {
      log_info("Skipping to next song");
      if (!mpd_run_next(conn)) {
        log_error("Failed to skip to next song: %s",
                  mpd_connection_get_error_message(conn));
        mpd_connection_clear_error(conn);
      }
    } else if (ch == '[') {
      log_info("Going to previous song");
      if (!mpd_run_previous(conn)) {
        log_error("Failed to go to previous song: %s",
                  mpd_connection_get_error_message(conn));
        mpd_connection_clear_error(conn);
      }
    } else if (ch == KEY_BACKSPACE) {
      log_info("Clearing MPD queue");
      if (!mpd_run_clear(conn)) {
        log_error("Failed to clear queue: %s",
                  mpd_connection_get_error_message(conn));
        mpd_connection_clear_error(conn);
      }
    }

    // --- Directory browser input ---
    else if (ui->show_directory_browser) {
      switch (ch) {
      case KEY_UP:
        if (ui->selected_index > 0)
          ui->selected_index--;
        break;
      case KEY_DOWN:
        if (ui->selected_index < ui->item_count - 1)
          ui->selected_index++;
        break;
      case '\n':
        if (ui->item_count == 0 || ui->selected_index < 0 ||
            ui->selected_index >= ui->item_count)
          break;
        if (ui->item_types[ui->selected_index] == 0) {
          log_debug("Entering directory: %s",
                    ui->item_uris[ui->selected_index]);
          free(ui->current_directory);
          ui->current_directory = strdup(ui->item_uris[ui->selected_index]);
          ui->selected_index = 0;
        } else {
          log_info("Adding song to queue: %s",
                   ui->item_uris[ui->selected_index]);
          if (mpd_run_add(conn, ui->item_uris[ui->selected_index])) {
            mvwprintw(ui->main_area, ui->item_count + 2, 2, "Song added");
            if (!mpd_run_play(conn)) {
              log_error("Failed to start playback after add: %s",
                        mpd_connection_get_error_message(conn));
              mvwprintw(ui->main_area, ui->item_count + 3, 2,
                        "Failed to play: %s",
                        mpd_connection_get_error_message(conn));
            }
          } else {
            log_error("Failed to add song '%s': %s",
                      ui->item_uris[ui->selected_index],
                      mpd_connection_get_error_message(conn));
            mvwprintw(ui->main_area, ui->item_count + 2, 2,
                      "Failed to add song: %s",
                      mpd_connection_get_error_message(conn));
          }
          wrefresh(ui->main_area);
          mpd_response_finish(conn);
        }
        break;
      case 27: // ESC — go up a directory
      {
        char *parent = get_parent_directory(ui->current_directory);
        if (parent) {
          log_debug("Going up to parent directory: %s", parent);
          free(ui->current_directory);
          ui->current_directory = parent;
          ui->selected_index = 0;
        }
        break;
      }
      }
    }

    // --- Directory selection input ---
    else if (ui->show_directory_selection) {
      if (ch == '\n') {
        log_info("Directory selection confirmed: '%s'", ui->input_buffer);
        free(ui->current_directory);
        ui->current_directory = strdup(ui->input_buffer);
        ui->show_directory_selection = false;
      } else if (ch == 27) {
        log_debug("Directory selection cancelled");
        strncpy(ui->input_buffer, ui->current_directory, MAX_PATH - 1);
        ui->input_buffer[MAX_PATH - 1] = '\0';
        ui->input_pos = strlen(ui->input_buffer);
        ui->show_directory_selection = false;
      } else if (ch == KEY_BACKSPACE || ch == 127) {
        if (ui->input_pos > 0)
          ui->input_buffer[--ui->input_pos] = '\0';
      } else if (ch >= 32 && ch <= 126 && ui->input_pos < MAX_PATH - 1) {
        ui->input_buffer[ui->input_pos++] = ch;
        ui->input_buffer[ui->input_pos] = '\0';
      }
    }

    // --- Queue scroll input ---
    else if (ui->show_queue) {
      if (ch == KEY_UP) {
        if (ui->total_qsongs > 0)
          ui->queue_ctr =
              (ui->queue_ctr + ui->total_qsongs - 1) % ui->total_qsongs;
      } else if (ch == KEY_DOWN) {
        if (ui->total_qsongs > 0)
          ui->queue_ctr = (ui->queue_ctr + 1) % ui->total_qsongs;
      }
    }

    // --- Update active tab state ---
    switch (current_win) {
    case 0:
      ui->show_directory_browser = false;
      ui->show_queue = false;
      ui->show_help = false;
      break;
    case 1:
      ui->show_directory_browser = true;
      ui->show_help = false;
      ui->show_queue = false;
      break;
    case 2:
      ui->show_directory_browser = false;
      ui->show_queue = true;
      ui->show_help = false;
      break;
    case 3:
      ui->show_directory_browser = false;
      ui->show_help = true;
      ui->show_queue = false;
      break;
    default:
      break;
    }
    ui->current_tab = current_win;

    update_header(ui);
    update_main_area(conn, ui);
    update_footer(conn, ui);
  }

  log_info("User quit TUI (pressed 'q')");
}

/**
 * @brief Get the parent directory object
 *
 * @param path
 * @return char*
 */
char *get_parent_directory(const char *path) {
  if (strlen(path) == 0) {
    return NULL;
  }

  char *last_slash = strrchr(path, '/');
  if (last_slash == NULL) {
    return strdup("");
  }

  size_t len = last_slash - path;
  char *parent = malloc(len + 1);
  strncpy(parent, path, len);
  parent[len] = '\0';

  return parent;
}
