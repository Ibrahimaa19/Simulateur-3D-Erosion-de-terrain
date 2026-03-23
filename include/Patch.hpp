#ifndef PATCH_H
#define PATCH_H

#include "Texture.hpp"
#include "Frustrum.hpp"
#include <GL/glew.h>
#include <algorithm>
#include <iostream>
#include <vector>

#define PATCH_SIZE 32 /**< Taille d'un patch en nombre de cellules */


/**
 * @brief Représente un sommet complet du terrain.
 *
 * Contient :
 * - la position 3D du sommet
 * - les coordonnées de texture associées
 */
struct Vertex{
    glm::vec3 position; /**< Position du sommet dans l'espace */
    glm::vec2 texture;  /**< Coordonnées de texture (u, v) */

    Vertex() = default;
    /**
     * @brief Construit un sommet à partir de sa position et de ses coordonnées de texture.
     * @param pos Position 3D
     * @param tex Coordonnées de texture
     */
    Vertex(glm::vec3 pos, glm::vec2 tex) : position(pos), texture(tex) {};
};

/**
 * @brief Structure représentant un niveau de détail (LOD)
 *
 * Contient les données de géométrie pour un niveau de détail spécifique.
 * Chaque patch a 5 niveaux de LOD (0 = détail maximal, 4 = détail minimal).
 */
struct Lod
{
    std::vector<Vertex> vertices;      /** Coordonnées des sommets (x,y,z) et texture (u,v) */
    std::vector<unsigned int> indices; /** Indices pour le rendu triangle */
};

/**
 * @class Patch
 * @brief Représente une portion du terrain avec gestion multi-LOD
 *
 * Un patch est une subdivision du terrain de taille fixe (PATCH_SIZE x PATCH_SIZE).
 * Il gère plusieurs niveaux de détail (LOD) pour optimiser le rendu :
 * - LOD 0 : résolution maximale (pas = 1)
 * - LOD 1 : pas = 2
 * - LOD 2 : pas = 4
 * - LOD 3 : pas = 8
 * - LOD 4 : pas = 16 (résolution minimale)
 *
 * Les patches sont également responsables de leurs voisins pour assurer
 * une transition cohérente entre différents niveaux de LOD.
 */
class Patch
{
  private:
    float mXzFactor;                      /** Facteur d'échelle sur les axes X et Z */
    unsigned int mPatchSize = PATCH_SIZE; /** Taille du patch (constante) */
    unsigned int mPatchX, mPatchZ;        /** Coordonnées du patch dans la grille */
    unsigned int mNbPatchX, mNbPatchZ;    /** Nombre total de patches en X et Z */

    Lod mLod[5];                         /** Données pour les 5 niveaux de LOD */
    int mLodSteps[5] = {1, 2, 4, 8, 16}; /** Pas de chaque niveau de LOD */

    GLuint mVbo[5]; /** Vertex Buffer Objects pour chaque LOD */
    GLuint mEbo[5]; /** Element Buffer Objects pour chaque LOD */
    GLuint mVao[5]; /** Vertex Array Objects pour chaque LOD */

    int mLodLevel; /** Niveau de LOD actuellement sélectionné */

    std::vector<Patch *> mNeighbors; /** Vecteur des patches voisins */

    Texture* mPatchTexture; /**< Texture associée au patch */

  public:
    /**
     * @brief Initialise les paramètres du patch
     * @param x Coordonnée X du patch dans la grille
     * @param z Coordonnée Z du patch dans la grille
     * @param xzFactor Facteur d'échelle XZ
     * @param nbPatchX Nombre total de patches en X
     * @param nbPatchZ Nombre total de patches en Z
     */
    void setPatch(unsigned int x, unsigned int z, float xzFactor, unsigned int nbPatchX, unsigned int nbPatchZ,Texture* texture);

    /**
     * @brief Ajoute un patch voisin
     * @param neighbor Pointeur vers le patch voisin
     */
    void addNeighbor(Patch *neighbor);

    /**
     * @brief Retourne la liste des patches voisins
     * @return Vecteur de pointeurs vers les patches voisins
     */
    std::vector<Patch *> getNeighbors();

    /**
     * @brief Retourne le niveau LOD d'un voisin
     * @param index Index du voisin
     * @return Niveau LOD du voisin
     */
    int getNeighborLodLevel(int index);

    /**
     * @brief Retourne la coordonnée X du patch
     * @return Coordonnée X
     */
    unsigned int getPatchX();

    /**
     * @brief Retourne la coordonnée Z du patch
     * @return Coordonnée Z
     */
    unsigned int getPatchZ();

    /**
     * @brief Retourne le VBO pour un niveau LOD donné
     * @param lodLevel Niveau de LOD
     * @return Identifiant OpenGL du VBO
     */
    GLuint getVbo(int lodLevel);

    /**
     * @brief Crée les buffers OpenGL pour tous les niveaux LOD
     *
     * Génère les VAO, VBO et EBO pour chaque niveau de LOD
     * et initialise les attributs de sommet.
     */
    void createBuffersGL();

    /**
     * @brief Génère les sommets pour tous les niveaux LOD
     * @param heights Vecteur des hauteurs du terrain
     * @param width Largeur du terrain
     * @param height Hauteur du terrain
     *
     * Pour chaque niveau LOD, génère les sommets avec :
     * - Échantillonnage adapté au pas du LOD
     * - Ajout de "skirt" (jupe) sur les bords pour masquer les trous
     */
    void generateLodVertices(std::vector<float> &heights, unsigned int width, unsigned int height);

    /**
     * @brief Génère les indices pour tous les niveaux LOD
     * @param heights Vecteur des hauteurs (non utilisé directement)
     * @param width Largeur du terrain (non utilisé)
     * @param height Hauteur du terrain (non utilisé)
     *
     * Crée les indices pour former une grille de triangles
     * pour chaque niveau de LOD.
     */
    void generateLodIndices(std::vector<float> &heights, unsigned int width, unsigned int height);

    /**
     * @brief Effectue le rendu du patch avec son LOD actuel
     *
     * Lie les buffers appropriés et dessine le patch
     * en utilisant le niveau LOD courant.
     */
    void render();

    /**
     * @brief Choisit le niveau LOD approprié en fonction de la caméra
     * @param cameraPos Position de la caméra
     * @param frustrum Frustum pour le test de visibilité
     * @return Niveau LOD choisi (0-4) ou -1 si hors frustum
     *
     * Le choix du LOD est basé sur :
     * 1. Test de visibilité dans le frustum
     * 2. Distance caméra-patch
     */
    int chooseLod(glm::vec3 cameraPos, Frustrum *frustrum);

    /**
     * @brief Retourne le niveau LOD actuel
     * @return Niveau LOD courant
     */
    int getLodLevel();

    /**
     * @brief Définit le niveau LOD actuel
     * @param level Nouveau niveau LOD
     */
    void setLodLevel(int level);

    /**
     * @brief Retourne le nombre total de patches en X
     * @return Nombre de patches en X
     */
    int getNbPatchX();

    /**
     * @brief Retourne le nombre total de patches en Z
     * @return Nombre de patches en Z
     */
    int getNbPatchZ();

    /**
     * @brief Met à jour sur le GPU les buffers d'un niveau de LOD donné.
     *
     * Recharge les sommets et les indices du niveau de détail indiqué
     * dans les buffers OpenGL déjà créés.
     *
     * @param lodLevel Niveau de détail à mettre à jour
     */
    void uploadSingleLodToGpu(int lodLevel);

    /**
     * @brief Met à jour sur le GPU tous les niveaux de LOD du patch.
     *
     * Cette méthode est utilisée après une modification des hauteurs
     * du terrain pour resynchroniser les buffers CPU et GPU.
     */
    void uploadLodToGpu();
};

#endif