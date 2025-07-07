// This purpose is to create a music player client in C
//
// The goal of this is for it to be a TUI (Terminal user interface)
//
// * note: want to create config file in lua, will learn how to do so
// * support both unix socket and loopback network connections

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <mpd/client.h>
#include <ncurses.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <stdbool.h>

#define MAX_ITEMS 1000
#define MAX_PATH 256
#define TAB_COUNT 4

typedef enum
{
	home,
	directory,
	queue,
	help
} Tab;

typedef struct 
{
    WINDOW *header;
    WINDOW *main_area;
    WINDOW *footer;
    WINDOW *directory_selection;
    int max_rows;
    int max_cols;
    char *current_directory;
    char **item_uris;
    int *item_types; // 0 for dir, 1 for song
    int item_count;
    int selected_index;
    bool show_directory_browser;
    bool show_directory_selection;
    char input_buffer[MAX_PATH];
    int input_pos;
		Tab current_tab;
} UI;

// globals
struct mpd_connection *conn;
UI ui;

void validate_connection(struct mpd_connection *conn)
{
    if (conn == NULL) 
    {
        fprintf(stderr, "Out of memory\n");
        if (isendwin() == FALSE) endwin();
        exit(1);
    }
    if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) 
    {
        fprintf(stderr, "MPD error: %s\n", mpd_connection_get_error_message(conn));
        mpd_connection_free(conn);
        if (isendwin() == FALSE) endwin();
        exit(1);
    }
}

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

void init_ui(const char *starting_directory)
{
    getmaxyx(stdscr, ui.max_rows, ui.max_cols);

		// create new windows
    ui.header = newwin(3, ui.max_cols, 0, 0);
    ui.main_area = newwin(ui.max_rows - 5, ui.max_cols, 3, 0);
    ui.directory_selection = newwin(2, ui.max_cols, ui.max_rows - 4, 0);
    ui.footer = newwin(2, ui.max_cols, ui.max_rows - 2, 0);

		// directory setup
    ui.current_directory = strdup(starting_directory ? starting_directory : "");
    ui.item_uris = malloc(MAX_ITEMS * sizeof(char*));
    ui.item_types = malloc(MAX_ITEMS * sizeof(int));
    ui.item_count = 0;
    ui.selected_index = 0;
    
    ui.show_directory_browser = false;
    ui.show_directory_selection = false;
    ui.current_tab = home;

    strncpy(ui.input_buffer, starting_directory ? starting_directory : "", MAX_PATH - 1);
    ui.input_buffer[MAX_PATH - 1] = '\0';
    ui.input_pos = strlen(ui.input_buffer);
}

void update_header()
{
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    char time_str[20];
    strftime(time_str, sizeof(time_str), "%I:%M:%S", tm);
		werase(ui.header);
    
		// display time
		box(ui.header, 0, 0);
    mvwprintw(ui.header, 1, 2, "Time: %s", time_str);

		const char *tab_names[] = {"Home", "Directory", "Queue", "Help"};
    int x_pos = 2;
    for (int i = 0; i < TAB_COUNT; i++) 
		{
        if (i == ui.current_tab) 
				{
            wattron(ui.header, A_REVERSE);
        }
        mvwprintw(ui.header, 2, x_pos, " %s ", tab_names[i]);
        if (i == ui.current_tab) 
				{
            wattroff(ui.header, A_REVERSE);
        }
        x_pos += strlen(tab_names[i]) + 3;
    }
    wrefresh(ui.header);
}

void update_directory_browser()
{
    for (int i = 0; i < ui.item_count; i++)
    {
        if (ui.item_uris[i]) free(ui.item_uris[i]);
    }
    ui.item_count = 0;

    if (!mpd_send_list_meta(conn, ui.current_directory[0] ? ui.current_directory : NULL))
    {
        mvwprintw(ui.main_area, 2, 2, "Error sending MPD command");
        wrefresh(ui.main_area);
        return;
    }

    struct mpd_entity *entity;
    while ((entity = mpd_recv_entity(conn)) != NULL && ui.item_count < MAX_ITEMS)
    {
        if (mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_DIRECTORY)
        {
            const struct mpd_directory *dir = mpd_entity_get_directory(entity);
            ui.item_uris[ui.item_count] = strdup(mpd_directory_get_path(dir));
            ui.item_types[ui.item_count] = 0;
        }
        else if (mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_SONG)
        {
            const struct mpd_song *song = mpd_entity_get_song(entity);
            ui.item_uris[ui.item_count] = strdup(mpd_song_get_uri(song));
            ui.item_types[ui.item_count] = 1;
        }
        mpd_entity_free(entity);
        ui.item_count++;
    }

    if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS)
    {
        mvwprintw(ui.main_area, 2, 2, "MPD error: %s", mpd_connection_get_error_message(conn));
        mpd_response_finish(conn);
        wrefresh(ui.main_area);
        return;
    }

    mpd_response_finish(conn);

    werase(ui.main_area);
    box(ui.main_area, 0, 0);
    mvwprintw(ui.main_area, 1, 2, "Directory: %s", ui.current_directory);
    if (ui.item_count == 0)
    {
        mvwprintw(ui.main_area, 2, 2, "No items found");
    }
    else
    {
        for (int j = 0; j < ui.item_count; j++)
        {
            if (j == ui.selected_index)
            {
                wattron(ui.main_area, A_REVERSE);
            }
            if (ui.item_types[j] == 0)
            {
                mvwprintw(ui.main_area, j + 2, 2, "%s/", ui.item_uris[j]);
            }
            else
            {
                mvwprintw(ui.main_area, j + 2, 2, "%s", ui.item_uris[j]);
            }
            if (j == ui.selected_index)
            {
                wattroff(ui.main_area, A_REVERSE);
            }
        }
    }
    wrefresh(ui.main_area);
}

void update_directory_selection() 
{
    werase(ui.directory_selection);
    box(ui.directory_selection, 0, 0);
    mvwprintw(ui.directory_selection, 1, 2, "Music Directory: %s", ui.input_buffer);
    mvwprintw(ui.directory_selection, 1, ui.max_cols - 30, "[Enter to save, Esc to cancel]");
    wrefresh(ui.directory_selection);
}

void update_main_area() 
{
    werase(ui.main_area);
    box(ui.main_area, 0, 0);
    if (ui.show_directory_browser) 
    {
        update_directory_browser();
    } 
    else 
    {
        mvwprintw(ui.main_area, 1, 2, "cbmp - C-based Music Player");
    }
    wrefresh(ui.main_area);
}

void update_footer()
{
    werase(ui.footer);
    box(ui.footer, 0, 0);

    if (!mpd_send_status(conn))
    {
        mvwprintw(ui.footer, 1, 2, "MPD command error");
        wrefresh(ui.footer);
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
                pos = (pos + 1) % (ui.max_cols - 4);                        
                mvwprintw(ui.footer, 1, 2 + pos, "|");
                break;
            case MPD_STATE_PAUSE:
                mvwprintw(ui.footer, 1, 2, "Paused");
                break;
            case MPD_STATE_STOP:
                mvwprintw(ui.footer, 1, 2, "Stopped");
                break;
            default:
                mvwprintw(ui.footer, 1, 2, "Unknown (default switch)");
                break;
        }
        mpd_status_free(status);
    }
    else
    {
        mvwprintw(ui.footer, 1, 2, "No status");
    }

    if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS)
    {
        mvwprintw(ui.footer, 1, 2, "MPD error: %s", mpd_connection_get_error_message(conn));
    }

    mpd_response_finish(conn);
    wrefresh(ui.footer);
}

void run_tui() 
{
    int current_win = 0; 
    int ch;
    timeout(500);
    while ((ch = getch()) != 'q') 
    {
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

        // update current window
        switch (current_win) 
        {
            // home 
            case 0:
                ui.show_directory_browser = false;
                // ui.show_playlists = false
                // ui.show_help = false
                break;
            // directory
            case 1:
                ui.show_directory_browser = true;
                // ui.show_help = false
                // ui.show_playlists = false 
                break;
            // playlist
            case 2:
                ui.show_directory_browser = false;
                // ui.show_playlists = true
                // ui.show_help = false
                break;
            // help
            case 3:
                ui.show_directory_browser = false;
                // ui.show_help = true
                // ui.show_playlists = false
                break;
            default:
                break;
        }

				ui.current_tab = current_win;

        if (ch == 'p')
        {
            if (!mpd_send_status(conn))
            {
                mvwprintw(ui.main_area, ui.item_count + 2, 2, "Failed to get status: %s", 
                          mpd_connection_get_error_message(conn));
                wrefresh(ui.main_area);
                mpd_response_finish(conn);
                break;
            }
            struct mpd_status *status = mpd_recv_status(conn);
            if (status)
            {
                switch (mpd_status_get_state(status))
                {
                    case MPD_STATE_PLAY:
                        if (mpd_run_pause(conn, true))
                        {
                            mvwprintw(ui.main_area, ui.item_count + 2, 2, "Paused");
                        }
                        else
                        {
                            mvwprintw(ui.main_area, ui.item_count + 2, 2, "Failed to pause: %s", 
                                      mpd_connection_get_error_message(conn));
                        }
                        break;
                    case MPD_STATE_PAUSE:
                    case MPD_STATE_STOP:
                        if (mpd_run_play(conn))
                        {
                            mvwprintw(ui.main_area, ui.item_count + 2, 2, "Playing");
                        }
                        else
                        {
                            mvwprintw(ui.main_area, ui.item_count + 2, 2, "Failed to play: %s", 
                                      mpd_connection_get_error_message(conn));
                        }
                        break;
                    default:
                        mvwprintw(ui.main_area, ui.item_count + 2, 2, "Unknown playback state");
                        break;
                }
                mpd_status_free(status);
            }
            else
            {
                mvwprintw(ui.main_area, ui.item_count + 2, 2, "No status available");
            }
            mpd_response_finish(conn);
            wrefresh(ui.main_area);            
        }
        else if (ui.show_directory_browser) 
        {
            // explore directory
            switch (ch) 
            {
                case KEY_UP:
                    if (ui.selected_index > 0) ui.selected_index--;
                    break;
                case KEY_DOWN:
                    if (ui.selected_index < ui.item_count - 1) ui.selected_index++;
                    break;
                case '\n':
                    if (ui.item_count == 0 || ui.selected_index < 0 || ui.selected_index >= ui.item_count)
                    {
                        break; // No valid selection
                    }
                    if (ui.item_types[ui.selected_index] == 0) 
                    {
                        char *new_dir = ui.item_uris[ui.selected_index];
                        free(ui.current_directory);
                        ui.current_directory = strdup(new_dir);
                        ui.selected_index = 0;
                    }
                    else 
                    {
                        if (mpd_run_add(conn, ui.item_uris[ui.selected_index]))
                        {
                            mvwprintw(ui.main_area, ui.item_count + 2, 2, "Song added");
                            if (!mpd_run_play(conn))
                            {
                                mvwprintw(ui.main_area, ui.item_count + 3, 2, "Failed to play: %s", 
                                          mpd_connection_get_error_message(conn));
                            }
                        }
                        else
                        {
                            mvwprintw(ui.main_area, ui.item_count + 2, 2, "Failed to add song: %s", 
                                      mpd_connection_get_error_message(conn));
                        }
                        wrefresh(ui.main_area);
                        mpd_response_finish(conn);    
                    }
                    break;
                case 'u':
                    char *parent = get_parent_directory(ui.current_directory);
                    if (parent) 
                    {
                        free(ui.current_directory);
                        ui.current_directory = parent;
                        ui.selected_index = 0;
                    }
                    break;
            }
        }
        else if (ui.show_directory_selection) 
        {
            if (ch == '\n') 
            {
                free(ui.current_directory);
                ui.current_directory = strdup(ui.input_buffer);
                ui.show_directory_selection = false;
            }
            else if (ch == 27) 
            { // Esc
                strncpy(ui.input_buffer, ui.current_directory, MAX_PATH - 1);
                ui.input_buffer[MAX_PATH - 1] = '\0';
                ui.input_pos = strlen(ui.input_buffer);
                ui.show_directory_selection = false;
            }
            else if (ch == KEY_BACKSPACE || ch == 127) 
            {
                if (ui.input_pos > 0) 
                {
                    ui.input_buffer[--ui.input_pos] = '\0';
                }
            } 
            else if (ch >= 32 && ch <= 126 && ui.input_pos < MAX_PATH - 1) 
            {
                ui.input_buffer[ui.input_pos++] = ch;
                ui.input_buffer[ui.input_pos] = '\0';
            }
        }

        update_header();
        update_main_area();
        if (ui.show_directory_selection) update_directory_selection();
        update_footer();
    }
}

int main()
{
    // init lua config
    char *starting_directory = strdup("");
    char *connection_type = NULL;
    char *socket_path = NULL;
    char *host = NULL;
    int port = 0;

    lua_State *L = luaL_newstate();
    luaL_openlibs(L);
    if (luaL_dofile(L, "config.lua") == LUA_OK)
    {
        // Read music_directory
        lua_getglobal(L, "music_directory");
        if (lua_isstring(L, -1))
        {
            const char *lua_path = lua_tostring(L, -1);
            // Expand ~ if present
            if (lua_path[0] == '~' && (lua_path[1] == '/' || lua_path[1] == '\0'))
            {
                const char *home = getenv("HOME");
                if (home)
                {
                    size_t home_len = strlen(home);
                    size_t path_len = strlen(lua_path + 1); // Skip ~
                    char *expanded = malloc(home_len + path_len + 2); // +2 for '/' and '\0'
                    strcpy(expanded, home);
                    if (path_len > 0 && lua_path[1] == '/')
                        strcat(expanded, lua_path + 1);
                    else if (path_len > 0)
                    {
                        strcat(expanded, "/");
                        strcat(expanded, lua_path + 1);
                    }
                    free(starting_directory);
                    starting_directory = expanded;
                }
            }
            else
            {
                free(starting_directory);
                starting_directory = strdup(lua_path);
            }
            // Convert absolute path to relative if it starts with MPD's music_directory
            const char *mpd_music_dir = "/home/bay/Music"; // Adjust based on mpd.conf
            size_t mpd_len = strlen(mpd_music_dir);
            if (strncmp(starting_directory, mpd_music_dir, mpd_len) == 0)
            {
                char *relative = strdup(starting_directory + mpd_len);
                if (relative[0] == '/') memmove(relative, relative + 1, strlen(relative));
                free(starting_directory);
                starting_directory = relative;
            }
        }
        lua_pop(L, 1);

        // Read connection_type
        lua_getglobal(L, "connection_type");
        if (lua_isstring(L, -1))
        {
            connection_type = strdup(lua_tostring(L, -1));
        }
        lua_pop(L, 1);

        // Read socket_path
        lua_getglobal(L, "socket_path");
        if (lua_isstring(L, -1))
        {
            socket_path = strdup(lua_tostring(L, -1));
        }
        lua_pop(L, 1);

        // Read host
        lua_getglobal(L, "host");
        if (lua_isstring(L, -1))
        {
            host = strdup(lua_tostring(L, -1));
        }
        lua_pop(L, 1);

        // Read port
        lua_getglobal(L, "port");
        if (lua_isnumber(L, -1))
        {
            port = (int)lua_tointeger(L, -1);
        }
        lua_pop(L, 1);
    }
    lua_close(L);

    // Set default connection parameters if not specified
    if (!connection_type)
    {
        connection_type = strdup("socket");
    }
    if (!socket_path && strcmp(connection_type, "socket") == 0)
    {
        socket_path = strdup("/home/bay/.config/mpd/socket");
    }
    if (!host && strcmp(connection_type, "network") == 0)
    {
        host = strdup("localhost");
    }
    if (port == 0 && strcmp(connection_type, "network") == 0)
    {
        port = 6600; // Default MPD port
    }

    // init mpd
    if (strcmp(connection_type, "socket") == 0)
    {
        conn = mpd_connection_new(socket_path, 0, 0);
    }
    else if (strcmp(connection_type, "network") == 0)
    {
        conn = mpd_connection_new(host, port, 0);
    }
    else
    {
        fprintf(stderr, "Invalid connection_type: %s\n", connection_type);
        free(starting_directory);
        free(connection_type);
        free(socket_path);
        free(host);
        if (isendwin() == FALSE) endwin();
        exit(1);
    }
    validate_connection(conn);

    // Clean up connection parameters
    free(connection_type);
    free(socket_path);
    free(host);

    // init ncurses
    initscr();                                    /* Start curses mode          */
    raw();                                                
    keypad(stdscr, TRUE);             
    noecho();
    curs_set(0);
    
    // init ui and run it
    init_ui(starting_directory);
    free(starting_directory);
    run_tui();

    // cleanup
    free(ui.current_directory);
    for (int i = 0; i < ui.item_count; i++)
    {
        if (ui.item_uris[i]) free(ui.item_uris[i]);
    }
    free(ui.item_uris);
    free(ui.item_types);
    delwin(ui.header);
    delwin(ui.main_area);
    delwin(ui.directory_selection);
    delwin(ui.footer);
    endwin();
    mpd_connection_free(conn);

    return 0;
}
