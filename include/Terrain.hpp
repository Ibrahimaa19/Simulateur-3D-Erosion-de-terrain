#ifndef TERRAIN_H
#define TERRAIN_H

#include <string.h>
#include <vector>
#include <iostream>
#include <algorithm>
#include <GL/glew.h>

#include "stb_image.hpp"

class Terrain{ 
protected:

	std::vector<float> data; // matrice des valeurs de chaque pixel
	int height;
	int width;
    float yfactor;
    float xzfactor; // pour redimensionner l'axe x et z
    float max_height;
    float min_height;
	int borderSize;
    int cellSpacing;
	std::vector<float> vertices;
	std::vector<unsigned int> indices;

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


public:
    /**
     * @brief Contructeur de la structure de données Terrain avec l'image passer en paramètre
     *
     * @param image_path Un chemin vers une image à lire
     * @return Une structure Terrain correspondant avec l'image passer en paramètre
     */
    void load_terrain(const char* image_path,float yfactor,float xzfactor);

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

    /**
     * @brief Dessine les triangles avec les données du terrain
    */
    void renderer();

    /**
     * @brief Retourne la height au point (i,j)
     * @param i l'indice de la ligne
     * @param j l'indice de la colonne
    */
    int get_height(int i, int j) const{
        return data[j * width + i];
    };

    /**
     * @brief Met à jour la height au point (i,j)
     * @param i l'indice de la ligne
     * @param j l'indice de la colonne
     * @param value la valeur a insérer à la position(i,j)
    */
    void set_height(int i, int j, float value){
        data[j * width + i] = value;
    };

    /**
     * @brief Retourne la plus grande height
    */
    float get_max_height() const{
        return max_height;
    };

    /**
     * @brief Retourne la plus petite height
    */
    float get_min_height() const{
        return min_height;
    };

    /**
     * @brief Retourne la height du terrain
    */
    int get_terrain_height() const{
        return this->height;
    };

    /**
     * @brief Retourne la width du terrain
    */
    int get_terrain_width() const{
        return this->width;
    };

    /**
     * @brief Met à jour le vecteur data du terrain
     * @param value la valeur a insérer à data[i]
    */
    void set_data(int i, float value);

    /**
     * @brief Retourne le vecteur data du terrain
    */
    std::vector<float> get_data();

    /**
     * @brief Retourne la size du vecteur indices
    */
    int get_indices_size() const;

    /**
     * @brief Retourne la size du vecteur vertices
    */
    int get_vertices_size() const;

    /**
     * @brief Verifie si on est en dehors du terrain
    */
    bool inside(int i, int j) const;

};

#endif
