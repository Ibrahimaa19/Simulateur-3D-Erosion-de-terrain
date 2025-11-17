#include <vector>
#include "stb_image.h"

struct Terrain{
	std::vector<float> data; // matrice des valeurs de chaque pixel
	int height;
	int width;
	int borderSize;
	std::vector<float> vertices;
	std::vector<unsigned int> indices;

    Terrain (const char* image_path){
        int t_channels;
        unsigned char* image = stbi_load(image_path, &this->width, &this->height, &t_channels, 1);
        
        this->data.resize(height*width);

        this->borderSize = 10;

        for (int i = 0; i < width * height; i++) {
            this->data[i] = image[i] / 255.0f;
        }

        stbi_image_free(image);
    }
};