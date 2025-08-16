// This purpose is to create a music player client in C
//
// The goal of this is for it to be a TUI (Terminal user interface)
//
// * note: want to create config file in lua, will learn how to do so
// * support both unix socket and loopback network connections

#include <stdlib.h>
#include <string.h>
#include <mpd/client.h>
#include <ncurses.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <stdbool.h>
#include "../include/mpd_connections.h"
#include "../include/ui.h"
#include "../include/lua_config.h"

// globals
struct mpd_connection *conn;
UI ui;


int main()
{
    // local to main
    char *starting_directory = strdup("");
    char *connection_type = NULL;
    char *socket_path = NULL;
    char *host = NULL;
    int port = 0;

    // setup lua
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);
    config_lua(L, starting_directory, connection_type, socket_path, host, &port);

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

    // make our mpd connection
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
		
    // clean up temp vars
    free(connection_type);
    free(socket_path);
    free(host);

    printf("Before init ncurses\n");

    // init ncurses
    initscr();
    raw();
    keypad(stdscr, TRUE);
    noecho();
    curs_set(0);
    
    printf("After init ncurses \n");

    printf("Before init ui \n");

    // initialize our ui
    init_ui(starting_directory, &ui);
    free(starting_directory);

    printf("After init ui \n");

    printf("Before run tui \n");

    // run tui
    run_tui(conn, &ui);
    // clean up when user exits
    clean_tui(&ui);
    mpd_connection_free(conn);

    return 0;
}
