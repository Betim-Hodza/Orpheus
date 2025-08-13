#ifndef MPD_CONNECTIONS_H
#define MPD_CONNECTIONS_H

// directly used
#include <mpd/client.h>
#include <ncurses.h>
// indirectly used
#include <stdlib.h>
#include <stdio.h>

void validate_connection(struct mpd_connection *conn);


#endif