#include "../include/img_to_ascii.h"
#include "../include/stb_image.h"
#include <pwd.h>
#include <sys/types.h>

/**
 * @brief Expand tilde (~) in path to home directory
 * 
 * @param path Path that may contain tilde
 * @return char* Expanded path (caller must free)
 */
char* expand_tilde(const char* path) {
    if (path[0] != '~') {
        return strdup(path);
    }
    
    const char* home = getenv("HOME");
    if (!home) {
        struct passwd* pw = getpwuid(getuid());
        home = pw ? pw->pw_dir : "/";
    }
    
    size_t home_len = strlen(home);
    size_t path_len = strlen(path);
    char* expanded = malloc(home_len + path_len);
    
    if (expanded) {
        strcpy(expanded, home);
        strcat(expanded, path + 1); // Skip the ~
    }
    
    return expanded;
}

/**
* @brief display ascii art 
*
* @param ui The UI structure containing the main area window
* @param image_path Path to the image file to convert
* @return none
*/
void image_to_ascii(UI *ui, const char *image_path) 
{
	const char *chars = "`^\",:;Il!i~+_-?][}(1)(|\\/tfjrxnuvczXYUJCLQ0OZmwqpdbkhao#MW&8%B@S";
	const int charsLen = strlen(chars);
	
	// Check if we need to reload the image
	bool need_reload = false;
	if (ui->cached_image_path == NULL || strcmp(ui->cached_image_path, image_path) != 0) 
	{
		need_reload = true;
	}
	
	// Get current window dimensions
	int win_height, win_width;
	getmaxyx(ui->main_area, win_height, win_width);
	int ascii_width = (win_width - 4) / 2;
	int ascii_height = (win_height - 4) > 0 ? (win_height - 4) : 1;

  // min dimensions
	if (ascii_width < 1) ascii_width = 1;
	if (ascii_height < 1) ascii_height = 1;
	
	// Check if window size changed
	if (ui->ascii_art != NULL && (ascii_width != ui->ascii_width || ascii_height != ui->ascii_height)) 
	{
		need_reload = true;
	}
	
	// Load and convert image if needed
	if (need_reload) 
	{
		// Free old ASCII art if it exists
		if (ui->ascii_art) 
		{
			for (int i = 0; i < ui->ascii_height; i++) 
			{
				free(ui->ascii_art[i]);
			}
			free(ui->ascii_art);
			ui->ascii_art = NULL;
		}

    // free old cache path 
    if (ui->cached_image_path)
    {
      free(ui->cached_image_path);
      ui->cached_image_path = NULL;
    }
		
		// Expand tilde in path if present
		char* expanded_path = expand_tilde(image_path);
		if (!expanded_path) 
		{
			mvwprintw(ui->main_area, 2, 2, "Failed to expand path: %s", image_path);
			return;
		}
		
		// Load image
		int width, height, channels;
		unsigned char *image_data = stbi_load(expanded_path, &width, &height, &channels, 0);
		if (!image_data) 
		{
			mvwprintw(ui->main_area, 2, 2, "Failed to load image: %s", expanded_path);
			free(expanded_path);
			return;
		}
		
		free(expanded_path);

    // dimensions are correct 
    if (width <= 0 || height <= 0)
    {
      mvwprintw(ui->main_area, 2, 2, "Invalid image dimensions");
			stbi_image_free(image_data);
			return;
    }
		
		// Allocate memory for ASCII art
		ui->ascii_art = malloc(ascii_height * sizeof(char*));
    if (!ui->ascii_art) 
		{
			mvwprintw(ui->main_area, 2, 2, "Memory allocation failed");
			stbi_image_free(image_data);
			return;
		}
		for (int i = 0; i < ascii_height; i++) 
		{
			ui->ascii_art[i] = malloc((ascii_width + 1) * sizeof(char));
      
      if (!ui->ascii_art[i]) 
			{
				// Free already allocated rows
				for (int j = 0; j < i; j++) 
				{
					free(ui->ascii_art[j]);
				}
				free(ui->ascii_art);
				ui->ascii_art = NULL;
				stbi_image_free(image_data);
				printf("ascii art Memory allocation failed");
				return;
			}
		}

		ui->ascii_width = ascii_width;
		ui->ascii_height = ascii_height;
		
		// Update cached path
		ui->cached_image_path = strdup(image_path);
		
		// Convert image to ASCII and store in array
		for (int y = 0; y < ascii_height && y * height / ascii_height < height; y++) 
		{
			for (int x = 0; x < ascii_width && x * width / ascii_width < width; x++) 
			{
				int img_x = (x * width) / ascii_width;
				int img_y = (y * height) / ascii_height;

        // Clamp to image bounds (safety check)
				if (img_x >= width) img_x = width - 1;
				if (img_y >= height) img_y = height - 1;

				int idx = (img_y * width + img_x) * channels;
				
        // channel variations
				unsigned char r, g, b;
				if (channels >= 3) 
				{
					r = image_data[idx];
					g = image_data[idx + 1];
					b = image_data[idx + 2];
				}
				else if (channels == 1) 
				{
					// Grayscale
					r = g = b = image_data[idx];
				}
				else 
				{
					r = g = b = 0;
				}
				
        // standard luminance formula
        float luminance = (0.299f * r + 0.587f * g + 0.114f * b) / 255.0f;

        int char_idx = (int)(luminance * (charsLen -1));
        // Clamp to valid range
				if (char_idx < 0) char_idx = 0;
				if (char_idx >= charsLen) char_idx = charsLen - 1;

				ui->ascii_art[y][x] = chars[char_idx];
			}
			ui->ascii_art[y][ascii_width] = '\0';
		}
			
		stbi_image_free(image_data);
	}
	
	// Always render the cached ASCII art
	if (ui->ascii_art) 
	{
		// Clear the area first
		for (int y = 2; y < ui->ascii_height + 2; y++) 
		{
			mvwhline(ui->main_area, y, 2, ' ', win_width - 4);
		}
		
		// Render from cache
		for (int y = 0; y < ui->ascii_height; y++) 
		{
			for (int x = 0; x < ui->ascii_width; x++) 
			{
				mvwprintw(ui->main_area, y + 2, x * 2 + 2, "%c", ui->ascii_art[y][x]);
			}
		}
	}
}

