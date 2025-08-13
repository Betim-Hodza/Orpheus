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
#include "../include/mpd_connections.h"
#include "../include/ui.h"

// globals
struct mpd_connection *conn;
UI ui;


int main()
{
    char *starting_directory = strdup("");
    char *connection_type = NULL;
    char *socket_path = NULL;
    char *host = NULL;
    int port = 0;

    lua_State *L = luaL_newstate();
    luaL_openlibs(L);

    // as of now assume config is in current dir
    if (luaL_dofile(L, "config.lua") == LUA_OK)
    {
        lua_getglobal(L, "music_directory");
        if (lua_isstring(L, -1))
        {
            const char *lua_path = lua_tostring(L, -1);
            if (lua_path[0] == '~' && (lua_path[1] == '/' || lua_path[1] == '\0'))
            {
                const char *home = getenv("HOME");
                if (home)
                {
										// get home path
                    size_t home_len = strlen(home);
                    size_t path_len = strlen(lua_path + 1);
                    char *expanded = malloc(home_len + path_len + 2);
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
            const char *mpd_music_dir = "/home/bay/Music";
            size_t mpd_len = strlen(mpd_music_dir);

						// Check if starting_directory begins with mpd_music_dir (length mpd_len) and is either
						// exactly mpd_music_dir or a subdirectory (ends with '/' or '\0')	
            if (strncmp(starting_directory, mpd_music_dir, mpd_len) == 0 && 
                (starting_directory[mpd_len] == '/' || starting_directory[mpd_len] == '\0'))
            {
                char *relative = strdup(starting_directory + mpd_len + (starting_directory[mpd_len] == '/' ? 1 : 0));
                free(starting_directory);
                starting_directory = relative;
            }
        }
        lua_pop(L, 1);

        // get lua configuration
        lua_getglobal(L, "connection_type");
        if (lua_isstring(L, -1))
        {
            connection_type = strdup(lua_tostring(L, -1));
        }
        lua_pop(L, 1);

        lua_getglobal(L, "socket_path");
        if (lua_isstring(L, -1))
        {
            socket_path = strdup(lua_tostring(L, -1));
        }
        lua_pop(L, 1);

        lua_getglobal(L, "host");
        if (lua_isstring(L, -1))
        {
            host = strdup(lua_tostring(L, -1));
        }
        lua_pop(L, 1);

        lua_getglobal(L, "port");
        if (lua_isnumber(L, -1))
        {
            port = (int)lua_tointeger(L, -1);
        }
        lua_pop(L, 1);
    }
    lua_close(L);

    // how we will define connection type
    if (!connection_type)
    {
        connection_type = strdup("socket");
    }
    else if (!socket_path && strcmp(connection_type, "socket") == 0)
    {
        socket_path = strdup("/home/bay/.config/mpd/socket");
    }
    else if (!host && strcmp(connection_type, "network") == 0)
    {
        host = strdup("localhost");
    }
    else if (port == 0 && strcmp(connection_type, "network") == 0)
    {
        port = 6600;
    }

    // make mpd connection
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
		
    free(connection_type);
    free(socket_path);
    free(host);

		// init ncurses
    initscr();
    raw();
    keypad(stdscr, TRUE);
    noecho();
    curs_set(0);
    
    // initialize our ui and run the ui
    init_ui(starting_directory, &ui);
    free(starting_directory);
    run_tui(conn, &ui);
    clean_tui(&ui);
    mpd_connection_free(conn);

    return 0;
}
