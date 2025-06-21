// This purpose is to create a music player client in C
//
// The goal of this is for it to be a TUI (Terminal user interface)
//
// * note: want to create config file in lua, will learn how to do so
// * as for now assuming unix socket

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <mpd/client.h>
#include <ncurses.h>

typedef struct 
{
	WINDOW *header;
	WINDOW *main_area;
	WINDOW *footer;
	int max_rows;
	int max_cols;
} UI;

// globals
struct mpd_connection *conn;
UI ui;

void validate_connection(struct mpd_connection *conn)
{
	if (conn == NULL) 
	{
    fprintf(stderr, "Out of memory\n");
    exit(1);
	}
	if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) 
	{
    fprintf(stderr, "%s\n", mpd_connection_get_error_message(conn));
    mpd_connection_free(conn);
		exit(1);
	}
}

void init_ui()
{
	getmaxyx(stdscr, ui.max_rows, ui.max_cols);

	ui.header = newwin(2, ui.max_cols, 0, 0);
	ui.main_area = newwin(ui.max_rows - 4, ui.max_cols, 2, 0);
	ui.footer = newwin(2, ui.max_cols, ui.max_rows - 2, 0);
}

void update_header()
{
	time_t now = time(NULL);
	struct tm *tm = localtime(&now);
	char time_str[20];
	strftime(time_str, sizeof(time_str), "%H:%M:S", tm);

	werase(ui.header);
	box(ui.header, 0, 0);
	mvwprintw(ui.header, 1, 2, "Time: %s", time_str);
	wrefresh(ui.header);
}

void update_main_area()
{
	werase(ui.main_area);
	box(ui.main_area, 0, 0);
	mvwprintw(ui.main_area, 1, 2, "CMBP - C-based Music Player");
	mvwprintw(ui.main_area, 2, 2, "Press 'q' to quit");
	wrefresh(ui.main_area);
}

void update_footer()
{
	werase(ui.footer);
	box(ui.footer, 0, 0);

	// get mpd status
	mpd_send_status(conn);
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
				mvwprintw(ui.footer, 1, 2, "Unkown (default switch)");
				break;
		}
		mpd_status_free(status);
	}
	else
	{
		mvwprintw(ui.footer, 1, 2, "No Status");
	}
	wrefresh(ui.footer);
}

void run_tui()
{
	int ch;
	timeout(500);
	while ((ch = getch()) != 'q')
	{
		update_header();
		update_main_area();
		update_footer();
	}
}

int main ()
{
	// init mpd
	conn = mpd_connection_new("/home/bay/.config/mpd/socket", 0, 0);
	validate_connection(conn);	

	// init ncurses
	initscr();									/* Start curses mode 		  */
	raw();												
	keypad(stdscr, TRUE); 			
	noecho();
	curs_set(0);
	
	// init ui and run it
	init_ui();
	run_tui();

	// cleanup
	delwin(ui.header);
	delwin(ui.main_area);
	delwin(ui.footer);
	endwin();
	mpd_connection_free(conn);


	return 0;
}
