#ifndef TERRAIN_H
#define TERRAIN_H

#include <vector>
#include <iostream>
#include <GL/glew.h>

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

        if (image){
            std::cout << "Loaded heightmap of size " << height << " x " << width << std::endl;
        }else{
            std::cout << "Failed to load texture" << std::endl;
        }

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

    /**
     * @brief Créer un vecteur d'indices représentant les triangles à afficher.
     * 
     * Génère les indices des chaques triangles a afficher et les stocke dans le vecteur indices. Chaque carré du plan 2D, sera représenter avec deux triangles.
     * 
     * Par exemple, le premier carré de notre heightmap sera celui en haut à gauche, si on suppose que notre terrain est une matrice T de M*N. 
     * Alors le premier carré sera T(0,0) T(0,1) T(1,0) T(1,1), donc deux triangles seront générer avec comme indice de sommets : [T(0,0);T(1,0);T(0,1)] et [T(1,0);T(0,0);T(1,1)] 
     */
    void load_incides();

    /**
     * @brief Met en place les buffers pour le terrain
     * 
     * Remplis les buffers passer en argument avec les valeurs correspondant au terrain courant.
     * 
     * @param VAO le vertex array object, mode de lecture du VBO
     * @param VBO le vertex buffer object, buffer contenant tout les sommets
     * @param EBO le element buffer object, les triangles a afficher
     */
    void setup_terrain(GLuint &VAO, GLuint &VBO, GLuint &EBO);


    // Accès à une hauteur
    int getHeight(int i, int j) const;

    // mise à jour d'une hauteur
    void setHeight(int i, int j, float value);

    // vérifier si on n'est pas hors terrain
    bool inside(int i, int j) const;

};

#endif