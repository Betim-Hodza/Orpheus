#include "../include/lua_config.h"
#include <stdio.h>

void config_lua(lua_State *L, char *starting_directory, char *connection_type, char *socket_path, char *host, int *port)
{
    // Places to check for config should be in current dir, and ~/.config/orpheus/config.lua
    if ( (luaL_dofile(L, "config.lua") == LUA_OK) || (luaL_dofile(L, "~/.config/orpheus/config.lua") == LUA_OK) )
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
            char mpd_music_dir[256];  // This shouldn't be that big, but if it becomes a problem oh well
            const char *user = getenv("USER");
            if (user != NULL) 
            {
                snprintf(mpd_music_dir, sizeof(mpd_music_dir), "/home/%s/Music", user);
            }
            else 
            {
                perror("No user detected through getenv('USER')\nExiting...");
                exit(1);
            }
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
            *port = (int)lua_tointeger(L, -1);
        }
        lua_pop(L, 1);
    }
    lua_close(L);
}