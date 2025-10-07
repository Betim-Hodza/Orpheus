#include "../include/img_to_ascii.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

/**
* @brief display ascii art 
*
* @param Window (main area screen)
* @return none
*/
void image_to_ascii(const char *image_path) 
{
	// Characters that represent pixel brightness
	char chars[] = "`^\",:;Il!i~+_-?][}(1)(|\\/tfjrxnuvczXYUJCLQ0OZmwqpdbkhao#MW&8%B@S";
	// Length
	int charsLen = strlen(chars);
	// Image Properties
	int width, height, pixelSize;

	unsigned char *ImageData = stbi_load(image_path, &width, &height, &pixelSize, 0);

	if (ImageData)
	{

		unsigned char *pixels = ImageData;
		for (int rowIdx = 0; rowIdx < height; rowIdx++)
		{
			for (int colIdx = 0; colIdx < width; colIdx++)
			{
				// get rgb data 
				unsigned char R = *pixels++;
				unsigned char G = *pixels++;
				unsigned char B = *pixels++;

				if (pixelSize >= 4)
					unsigned char A = *pixels++;

				// calcualte avg 
				float avg = (R+G+B) / 3.0f; 

				// calculate al to index the ascii str 
				int charIdx = (int)(charsLen * (avg / 255.0f));

				// print out the index char 
				putchar(chars[charIdx]); 

			}
			putchar('\n');

		}
	}
	else 
  {
		printf("Failed to load image, %s", image_path);	
	}

}

