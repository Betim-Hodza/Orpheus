// art.hpp ascii art
#pragma once
#include <string>

#include "ui.hpp"

enum class ColorMode 
{
	GRAYSCALE,
	ANSI_256,
	TRUECOLOR
};

struct ImageData 
{
	// RGBA data
	std::vector<unsigned char> pixels;
	int width = 0; 
	int height = 0; 
	int channels = 0;
};

struct AsciiCell 
{
	wchar_t ch;
  // ANSI index or extended color index
	int fg; 
	int bg;
};

struct AsciiCanvas 
{
	int width = 0;
	int height = 0;
};

/* *
 * @brief ResolveImage path for a song (when taglib doesnt work)
 * @param song_path 
 * */
std::string ResolveImage(std::string song_path);

/* *
 * @brief get image data from path
 * @param cached_image where we'll store the data
 * @param album_image_path where the image is
 * */
void ProccessingImagePath(ImageData cached_image, std::string album_image_path);

/* *
 * @brief get image data from TagLib complex PICTURE
 * @param cached_image where we'll store the data
 * @param pictureMap 
 * */
void ProccessingImageTag(ImageData cached_image, const TagLib::VariantMap& pictureMap);
