#ifndef TERRAIN_H
#define TERRAIN_H

#include <string.h>
#include <vector>
#include <iostream>
#include <mutex>
#include <thread>
#include <GL/glew.h>

#include "stb_image.h"

struct Terrain{
	std::vector<float> data; // matrice des valeurs de chaque pixel
	int height;
	int width;
    float yfactor;
    float xzfactor; // pour redimensionner l'axe x et z
	int borderSize;
    int cellSpacing;
	std::vector<float> vertices;
	std::vector<unsigned int> indices;

    /**
     * @brief Contructeur de la structure de données Terrain avec l'image passer en paramètre
     *
     * @param image_path Un chemin vers une image à lire
     * @return Une structure Terrain correspondant avec l'image passer en paramètre
     */
    Terrain (const char* image_path,float yfactor,float xzfactor){
        int t_channels;

        unsigned char* image = stbi_load(image_path, &this->width, &this->height, &t_channels, 1);

        if (image){
            std::cout << "Loaded heightmap of size " << height << " x " << width << std::endl;
        }else{
            std::cout << "Failed to load texture" << std::endl;
        }

        this->data.resize(height*width);

        this->borderSize = 10;
        this->cellSpacing = 1;
        this->yfactor = yfactor;
        this->xzfactor = xzfactor;
        

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

    /**
     * @brief Dessine les triangles avec les données du terrain
    */
    virtual void renderer();

    // Accès à une hauteur
    int getHeight(int i, int j) const;

    // mise à jour d'une hauteur
    void setHeight(int i, int j, float value);

    // vérifier si on n'est pas hors terrain
    bool inside(int i, int j) const;

};

/**
 * @brief Enumeration des différents types d'érosion
 *
 * @param Thermal pour l'érosion thermique
 * @param Hydraulic pour l'érosion hydraulique
 * 
 */
enum ErosionType{
    Thermal,
    Hydraulic
};

struct TerrainDynamique : public Terrain{
	std::vector<float> back_data; // les données sur lequels le thread écrira
	std::mutex verrou;
	bool need_swap = false; // Si true on mettra a jour le VBO
	GLint VBO;
    ErosionType type_erosion;
    size_t iteration_counter = 0; // Compteur du nombre d'iteration effectué

    /**
     * @brief Contructeur de la structure de données fille DynamiqueTerrain avec l'image passer en paramètre
     *
     * @param image_path Un chemin vers une image à lire
     * @param vbo Le buffer associé à ce terrain
     * @return Une structure Terrain correspondant avec l'image passer en paramètre
     */
    TerrainDynamique(const char* image_path,float _yfactor,float _xzfactor,GLuint vbo, ErosionType erosion) : Terrain(image_path,_yfactor,_xzfactor){
		this->VBO = vbo;
		this->back_data = data;
        this->type_erosion = erosion;
	}

    /**
     * @brief Lance le thread qui calculera l'érosion pour le terrain en fonction de son type d'érosion
     * @param max_iteration Le nombre maximum d'itération effectuer par le thread
     * @param seconds Les secondes a attendre avant de lancer la prochaine iteration
    */
    void startThread(size_t max_iteration,size_t seconds);

    /**
     * @brief La fonction qui sera lancer par le thread pour simuler l'erosion
     * @param max_iteration Le nombre maximum d'itération effectuer par le thread
     * @param seconds Les secondes a attendre avant de lancer la prochaine iteration
    */
    void updateTerrainThermal(size_t max_iteration,size_t seconds);

    /**
     * @brief Met à jour le vecteur vertices et le VBO du terrain avec les données de data
    */
    void updateVBO();

    /**
     * @brief Dessine les triangles avec les données mise à jour du terrain
    */
    void renderer();

};

#endif
