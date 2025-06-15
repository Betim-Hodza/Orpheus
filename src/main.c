// This purpose is to create a music player client in C
//
// The goal of this is for it to be a TUI (Terminal user interface)
//
// * note: want to create config file in lua, will learn how to do so
// * as for now assuming unix socket

#include <stdio.h>
#include <stdlib.h>
#include <mpd/client.h>
#include <ncurses.h>

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

int main ()
{
	//vars
	// used to get characters
	int ch; 
	// when connecting to socket, we ignore 2nd field (port), 3rd field is timeout in ms
	struct mpd_connection *conn = mpd_connection_new("/home/bay/.config/mpd/socket", 0, 0);

	validate_connection(conn);
	initscr();							/* Start curses mode 		  */
	raw();									// line buffering disables
	keypad(stdscr, TRUE); 	// we get to use arrowkeys and f1, f2, etc..
	
	
	// we can now use the connection
	mpd_run_next(conn);
	printw("connection Opened\n");
	refresh();

	/*somewhere between here we need to do fancy stuff for music player (call funcs)*/


	// on quit
	// close the connection and free memory
	mpd_connection_free(conn);
	printw("Connection Closed\n");

	printw("Hello World !!!\n");	/* Print Hello World		  */
	refresh();			/* Print it on to the real screen */

	printw("Type any character to see it in bold\n");
	ch = getch();

	if(ch == KEY_F(1))
	{
		printw("F1 key pressed \n");
	}
	else
	{
		printw("The pressed key is ");
		attron(A_BOLD);
		printw("%c", ch);
		attroff(A_BOLD);
	}
	refresh();
	
	getch();			/* Wait for user input */
	endwin();			/* End curses mode		  */

	return 0;
}
