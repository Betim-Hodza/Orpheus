#include "../include/mpd_connections.h"

// validates a mpd connection given the mpd struct
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