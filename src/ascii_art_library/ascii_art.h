#ifndef ASCII_ART_H
#define ASCII_ART_H

/* Structure to hold ASCII art output */
typedef struct {
    char **lines;    /* Array of strings (ASCII art lines) */
    int num_lines;   /* Number of lines */
    int max_width;   /* Width of longest line */
} AsciiArt;

/*
 * Convert a JPEG image to ASCII art.
 * @param image_path: Path to the JPEG file.
 * @param ascii_width: Desired width of ASCII art in characters.
 * @param max_height: Maximum height of ASCII art in lines.
 * @return: AsciiArt structure with the result, or NULL on error.
 * Caller must free the result using ascii_art_free().
 */
AsciiArt *jpeg_to_ascii(const char *image_path, int ascii_width, int max_height);

/*
 * Free an AsciiArt structure and its contents.
 * @param art: Pointer to the AsciiArt structure.
 */
void ascii_art_free(AsciiArt *art);

#endif /* ASCII_ART_H */
