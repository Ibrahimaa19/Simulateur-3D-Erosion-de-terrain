#include <vector>
#include "stb_image.h"

struct Terrain{
	std::vector<float> data; // matrice des valeurs de chaque pixel
	int height;
	int width;
	int borderSize;
	std::vector<float> vertices;
	std::vector<unsigned int> indices;

    /**
     * @brief Contructeur de la structure de données Terrain avec l'image passer en paramètre
     *
     * @param image_path Un chemin vers une image à lire
     * @return Une structure Terrain correspondant avec l'image passer en paramètre
     */
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

    /**
     * @brief Met à jours le vecteur vertices, en fonction des valeurs dans data
     * 
     * Pour chaque point du terrain, on va générer un triplet x,y,z où x et z représenterons une position sur un plan 2D et y l'élévation sur ce point(x,z).
     * 
     * Lorsque nous serons sur les bords, nous définirons la hauteur/élévation à 0. 
     * C'est à dire, si x est compris en [0; borderSize] ou [width-borderSize;width] ou si, z est compris entre [0; borderSize] ou [height-borderSize;height]
     */
    void load_vectices();
    
    void load_incides();
};