#ifndef UI_H
#define UI_H

// direct includes
#include <ncurses.h>
#include <mpd/client.h>
// indirect includes
#include <stdlib.h>
#include <string.h>
#include <time.h>


// macros
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
		bool show_help;
    char input_buffer[MAX_PATH];
    int input_pos;
		Tab current_tab;
} UI;

// initialize the ui after setting up ncurses
void init_ui(const char *starting_directory, UI *ui);
// clean up tui
void clean_tui(UI* ui);
// updates time and current tab
void update_header(UI* ui);
// browse music dir
void update_directory_browser(struct mpd_connection *conn, UI* ui);
// updates directory screen
void update_directory_selection(UI* ui);
//help
void help_screen(UI *ui);
// update main tab with basic info (expand with album art)
void update_main_area(struct mpd_connection *conn, UI* ui);
// update footer with fun animation when music plays
void update_footer(struct mpd_connection *conn, UI* ui);
// runs whole tui process (Gets called from main)
void run_tui(struct mpd_connection *conn, UI* ui);
// get dir up
char *get_parent_directory(const char *path);


#endif