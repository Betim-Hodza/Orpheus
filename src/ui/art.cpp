// art.cpp
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize2.h"

#include "art.hpp"
#include <sys/stat.h>
#include <cmath>
#include <filesystem>
#include <map>
#include <memory>

namespace Art
{

/* Private helpers used in Generate() */
static uint32_t GetPixelRGBFromBuffer(const std::vector<unsigned char> &buf,
                                      int width, int height, int x, int y);
static int GrayscaleIndex(uint32_t rgb);

/* *
 * @brief ResolveImage path for a song (when taglib doesnt work)
 * @param song_path
 * */
std::string ResolveImage(const std::string &song_path)
{
	std::filesystem::path image_path(song_path);
	std::vector<std::string> possible_names = {"cover.jpg",  "cover.png",
	                                           "front.jpg",  "front.png",
	                                           "album.jpg",  "album.png",
	                                           "folder.jpg", "folder.png"};

	for (std::string name : possible_names)
	{
		image_path.replace_filename(name);
		struct stat buffer;
		if (stat(image_path.string().c_str(), &buffer) == 0)
		{
			return image_path.string();
		}
	}

	return "no image";
}

/* *
 * @brief load image from path
 * @param out our ascii image data
 * @param path where the image is
 * */
bool LoadImageFile(ImageData &out, const std::string &path)
{
	int w, h, ch;
	unsigned char *data = stbi_load(path.c_str(), &w, &h, &ch, 4);
	if (!data)
	{
		return false;
	}

	out.pixels.assign(data, data + (w * h * 4));
	out.width = w;
	out.height = h;
	out.channels = 4;
	stbi_image_free(data);
	return true;
}

/* *
 * @brief load image from memory
 * @param out our ascii image data
 * @param data image data in memory
 * @param length length of the data in memory
 * */
bool LoadImageMemory(ImageData &out, const uint8_t *data, size_t length)
{
	int w, h, ch;
	unsigned char *img =
		stbi_load_from_memory(data, static_cast<int>(length), &w, &h, &ch, 4);

	if (!img)
	{
		return false;
	}

	out.pixels.assign(img, img + (w * h * 4));
	out.width = w;
	out.height = h;
	out.channels = 4;
	stbi_image_free(img);
	return true;
}

uint32_t GetPixelRGB(const ImageData &img, int x, int y)
{
	if (x < 0 || x >= img.width || y < 0 || y >= img.height)
	{
		return 0;
	}

	size_t idx = (static_cast<size_t>(y) * img.width + x) * 4;
	uint8_t r = img.pixels[idx];
	uint8_t g = img.pixels[idx + 1];
	uint8_t b = img.pixels[idx + 2];

	return ((static_cast<uint32_t>(r) << 16) | (static_cast<uint32_t>(g) << 8) |
	        static_cast<uint32_t>(b));
}

AsciiCanvas Generate(const ImageData &image, ColorMode mode, int max_cols,
                     int max_rows)
{
	AsciiCanvas canvas;
	canvas.mode = mode;

	if (image.pixels.empty() || image.width == 0 || image.height == 0)
	{
		return canvas;
	}

	float img_aspect =
		static_cast<float>(image.width) / static_cast<float>(image.height);
	float char_aspect = 0.5f;
	int target_w = max_cols;
	int target_h =
		static_cast<int>(static_cast<float>(target_w) * char_aspect / img_aspect);

	if (target_h > max_rows)
	{
		target_h = max_rows;
		target_w =
			static_cast<int>(static_cast<float>(target_h) * img_aspect / char_aspect);
	}

	if (target_w <= 0 || target_h <= 0)
	{
		return canvas;
	}

	canvas.width = target_w;
	canvas.height = target_h;

	int resize_h = target_h * 2;
	std::vector<unsigned char> resized(target_w * resize_h * 4);

	stbir_resize_uint8_linear(image.pixels.data(), image.width, image.height, 0,
	                          resized.data(), target_w, resize_h, 0,
	                          static_cast<stbir_pixel_layout>(4));

	canvas.cells.reserve(target_w * target_h);

	for (int y = 0; y < target_h; ++y)
	{
		for (int x = 0; x < target_w; ++x)
		{
			int top_y = y * 2;
			int bot_y = y * 2 + 1;

			uint32_t top_rgb =
				GetPixelRGBFromBuffer(resized, target_w, resize_h, x, top_y);
			uint32_t bot_rgb =
				GetPixelRGBFromBuffer(resized, target_w, resize_h, x, bot_y);

			AsciiCell cell;
			cell.ch = L'▀';

			switch (mode)
			{
			case ColorMode::GRAYSCALE:
				cell.fg = GrayscaleIndex(top_rgb);
				cell.bg = GrayscaleIndex(bot_rgb);
				break;

			case ColorMode::ANSI_256:
				cell.fg = RGBToANSI256(top_rgb);
				cell.bg = RGBToANSI256(bot_rgb);
				break;
			}

			canvas.cells.push_back(cell);
		}
	}

	return canvas;
}

static uint32_t GetPixelRGBFromBuffer(const std::vector<unsigned char> &buf,
                                      int width, int height, int x, int y)
{
	if (x < 0 || x >= width || y < 0 || y >= height)
	{
		return 0;
	}

	size_t idx = (static_cast<size_t>(y) * width + x) * 4;
	uint8_t r = buf[idx];
	uint8_t g = buf[idx + 1];
	uint8_t b = buf[idx + 2];

	return (static_cast<uint32_t>(r) << 16) | (static_cast<uint32_t>(g) << 8) |
	       static_cast<uint32_t>(b);
}

/* *
 * @brief 0-9 grayscale using: .:-=+*#%@
 * @param rgb color pixel
 * */
static int GrayscaleIndex(uint32_t rgb)
{
	uint8_t r = (rgb >> 16) & 0xFF;
	uint8_t g = (rgb >> 8) & 0xFF;
	uint8_t b = rgb & 0xFF;

	float lum = 0.299f * r + 0.587f * g + 0.114f * b;
	int idx = static_cast<int>((lum / 255.0f) * 9.0f);

	if (idx > 9)
	{
		idx = 9;
	}

	return idx;
}

/* *
 * @brief 6x6x6 color cube for ANSI256 color
 * @param rgb color pixel
 * */
int RGBToANSI256(uint32_t rgb)
{
	uint8_t r = (rgb >> 16) & 0xFF;
	uint8_t g = (rgb >> 8) & 0xFF;
	uint8_t b = rgb & 0xFF;

	if (r == 0 && g == 0 && b == 0)
	{
		return 235;
	}

	int ri = (r * 5 + 127) / 255;
	int gi = (g * 5 + 127) / 255;
	int bi = (b * 5 + 127) / 255;

	return 16 + 36 * ri + 6 * gi + bi;
}

} // namespace Art