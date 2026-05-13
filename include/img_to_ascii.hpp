#ifndef IMG_TO_ASCII_H
#define IMG_TO_ASCII_H

#include "../include/ui.h"
#include <mpd/albumart.h>

/**
 * @brief Convert image to ASCII art and display it in the UI
 *
 * @param ui The UI structure containing the main area window
 * @param conn The mpd connection socket
 * @param image_path Path to the image file to convert
 */
void image_to_ascii(UI *ui, struct mpd_connection *conn, const char *image_path);

#endif
