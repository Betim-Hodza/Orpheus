#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jpeglib.h>
#include "ascii_art.h"

AsciiArt *jpeg_to_ascii(const char *image_path, int ascii_width, int max_height) {
    // Validate inputs
    if (!image_path || ascii_width <= 0 || max_height <= 0) {
        return NULL;
    }

    // Initialize libjpeg structures
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);

    // Open JPEG file
    FILE *infile = fopen(image_path, "rb");
    if (!infile) {
        jpeg_destroy_decompress(&cinfo);
        return NULL;
    }

    // Set up JPEG source and read header
    jpeg_stdio_src(&cinfo, infile);
    jpeg_read_header(&cinfo, TRUE);

    // Convert to grayscale
    cinfo.out_color_space = JCS_GRAYSCALE;
    jpeg_start_decompress(&cinfo);

    int width = cinfo.output_width;
    int height = cinfo.output_height;
    int row_stride = cinfo.output_width * cinfo.output_components;

    // Allocate image data array
    unsigned char *image = malloc(width * height);
    if (!image) {
        fclose(infile);
        jpeg_destroy_decompress(&cinfo);
        return NULL;
    }

    // Read image data
    JSAMPARRAY buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo, JPOOL_IMAGE, row_stride, 1);
    int y = 0;
    while (cinfo.output_scanline < cinfo.output_height) {
        jpeg_read_scanlines(&cinfo, buffer, 1);
        memcpy(image + y * width, buffer[0], width);
        y++;
    }

    // Clean up JPEG
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    fclose(infile);

    // Calculate ASCII art height, adjusting for 2:1 character aspect ratio
    float aspect_ratio = (float)height / width;
    int ascii_height = (int)(aspect_ratio * ascii_width / 2.0 + 0.5);
    if (ascii_height > max_height) {
        ascii_height = max_height;
    }

    // Allocate AsciiArt structure
    AsciiArt *art = malloc(sizeof(AsciiArt));
    if (!art) {
        free(image);
        return NULL;
    }
    art->lines = malloc(ascii_height * sizeof(char *));
    if (!art->lines) {
        free(image);
        free(art);
        return NULL;
    }
    for (int i = 0; i < ascii_height; i++) {
        art->lines[i] = malloc(ascii_width + 1);
        if (!art->lines[i]) {
            for (int j = 0; j < i; j++) free(art->lines[j]);
            free(art->lines);
            free(art);
            free(image);
            return NULL;
        }
    }
    art->num_lines = ascii_height;
    art->max_width = ascii_width;

    // ASCII characters from dark to light
    const char *ascii_chars = "@%#*+=-:. ";
    int num_chars = strlen(ascii_chars);

    // Generate ASCII art
    for (int r = 0; r < ascii_height; r++) {
        for (int c = 0; c < ascii_width; c++) {
            // Sample the corresponding pixel
            int x = (c * width) / ascii_width;
            int y = (r * height) / ascii_height;
            unsigned char pixel = image[y * width + x];

            // Map pixel value (0-255) to an ASCII character index
            int idx = (pixel * (num_chars - 1)) / 255;
            art->lines[r][c] = ascii_chars[idx];
        }
        art->lines[r][ascii_width] = '\0'; // Null-terminate each line
    }

    // Clean up
    free(image);
    return art;
}

void ascii_art_free(AsciiArt *art) {
    if (!art) return;
    for (int i = 0; i < art->num_lines; i++) {
        if (art->lines[i]) free(art->lines[i]);
    }
    free(art->lines);
    free(art);
}
