#ifndef TERRAIN_H
#define TERRAIN_H

#include <GL/glew.h>
#include <algorithm>
#include <fstream>
#include <glm/glm.hpp>
#include <iostream>
#include <memory>
#include <string.h>
#include <vector>

#include "Patch.hpp"
#include "stb_image.hpp"
#include "Texture.hpp"

class RendererManager;

/**
 * @class Terrain
 * @brief Classe de base représentant un terrain avec gestion des hauteurs et LOD
 *
 * Gère les données de hauteur, la génération de géométrie, et l'interface
 * avec le système de rendu et les patches pour le LOD.
 */
class Terrain
{
  protected:
    std::vector<float> mData; /**< Matrice des valeurs de hauteur */
    int mHeight;              /**< Hauteur du terrain en nombre de cellules */
    int mWidth;               /**< Largeur du terrain en nombre de cellules */
    float mYFactor;           /**< Facteur d'échelle sur l'axe Y (hauteur) */
    float mXzFactor;          /**< Facteur d'échelle sur les axes X et Z */
    float mMaxHeight;         /**< Hauteur maximale du terrain */
    float mMinHeight;         /**< Hauteur minimale du terrain */
    int mBorderSize;          /**< Taille de la bordure (aplatie) */
    int mCellSpacing;         /**< Espacement entre les cellules */

    std::vector<Vertex> mVertex; /**< Vecteur des sommets (x,y,z,u,v) */

    std::vector<std::unique_ptr<Patch>> mPatches; /**< Patches pour le LOD */

    Frustrum mFrustrum;                         /**< Frustum pour le culling */
    std::unique_ptr<RendererManager> mRenderer; /**< Gestionnaire de rendu */

    GLuint mTextureID;
    Texture* mTexture;

    /**
     * @brief Met à jour les vertices pour tous les niveaux LOD
     *
     * Délègue la génération des vertices à chaque patch via
     * Patch::generateLodVertices().
     */
    void loadVerticesLod();

    /**
     * @brief Met à jour les indices pour tous les niveaux LOD
     *
     * Délègue la génération des indices à chaque patch via
     * Patch::generateLodIndices().
     */
    void loadIndicesLod();

  public:
    /**
     * @brief Destructeur virtuel par défaut
     *
     * Nécessaire pour permettre le polymorphisme et assurer
     * la destruction correcte des classes filles.
     */
    virtual ~Terrain() = default;

    /**
     * @brief Charge un terrain à partir d'une image heightmap
     * @param imagePath Chemin vers l'image à lire
     * @param yFactor Facteur d'échelle verticale
     * @param xzFactor Facteur d'échelle horizontale
     */
    void loadTerrain(const char *imagePath, float yFactor, float xzFactor);

    /**
     * @brief Configure les buffers OpenGL pour le rendu avec LOD
     * @param vao Vertex Array Object
     * @param vbo Vertex Buffer Object
     * @param ebo Element Buffer Object
     */
    void setupTerrainLod(GLuint &vao, GLuint &vbo, GLuint &ebo);

    /**
     * @brief Effectue le rendu du terrain avec LOD
     * @param cameraPos Position de la caméra
     * @param projection Matrice de projection
     * @param view Matrice de vue
     */
    void renderLod(const glm::vec3 &cameraPos, glm::mat4 &projection, glm::mat4 &view);

    /**
     * @brief Crée les patches pour le système LOD
     *
     * Divise le terrain en patches de taille PATCH_SIZE et initialise
     * leurs paramètres (position, facteur d'échelle).
     */
    void createPatches();

    /**
     * @brief Retourne le frustum du terrain
     * @return Référence vers le frustum
     */
    Frustrum &getFrustrum();

    /**
     * @brief Retourne la liste des patches
     * @return Référence vers le vecteur de patches
     */
    std::vector<std::unique_ptr<Patch>> &getPatches();

    /**
     * @brief Retourne le gestionnaire de rendu
     * @return Pointeur vers le RendererManager
     */
    RendererManager *getRendererManager();

    /**
     * @brief Définit le gestionnaire de rendu
     * @param renderer Pointeur unique vers le RendererManager
     */
    void setRenderer(std::unique_ptr<RendererManager> renderer);

    /**
     * @brief Retourne la hauteur au point (i,j)
     * @param i Indice de la colonne
     * @param j Indice de la ligne
     * @return Hauteur au point (i,j)
     */
    float getHeight(int i, int j) const
    {
        return mData[j * mWidth + i];
    };

    /**
     * @brief Met à jour la hauteur au point (i,j)
     * @param i Indice de la colonne
     * @param j Indice de la ligne
     * @param value Nouvelle hauteur
     */
    void setHeight(int i, int j, float value)
    {
        mData[j * mWidth + i] = value;
    };

    /**
     * @brief Retourne la hauteur maximale du terrain
     * @return Hauteur maximale
     */
    float getMaxHeight() const
    {
        return mMaxHeight;
    };

    /**
     * @brief Retourne la hauteur minimale du terrain
     * @return Hauteur minimale
     */
    float getMinHeight() const
    {
        return mMinHeight;
    };

    /**
     * @brief Retourne la hauteur du terrain (nombre de cellules)
     * @return Hauteur en nombre de cellules
     */
    int getTerrainHeight() const
    {
        return mHeight;
    };

    /**
     * @brief Retourne la largeur du terrain (nombre de cellules)
     * @return Largeur en nombre de cellules
     */
    int getTerrainWidth() const
    {
        return mWidth;
    };

    /**
     * @brief Met à jour une valeur dans le vecteur de données
     * @param i Index dans le vecteur
     * @param value Nouvelle valeur
     */
    void setData(int i, float value);

    /**
     * @brief Retourne le vecteur de données
     * @return Pointeur vers le vecteur de hauteurs
     */
    std::vector<float> *getData();

    /**
     * @brief Retourne la taille du vecteur d'indices
     * @return Nombre d'indices
     */
    int getIndicesSize() const;

    /**
     * @brief Retourne la taille du vecteur de sommets
     * @return Nombre de sommets
     */
    int getVerticesSize() const;

    /**
     * @brief Vérifie si un point est dans les limites du terrain
     * @param i Coordonnée X
     * @param j Coordonnée Z
     * @return true si le point est dans le terrain
     */
    bool isInside(int i, int j) const;

    /**
     * @brief Met à jour les sommets du terrain
     */
    void updateVertices();

    /**
     * @brief Retourne le vecteur de sommets (lecture seule)
     * @return Référence constante vers le vecteur de sommets
     */
    const std::vector<Vertex> &getVertex() const
    {
        return mVertex;
    }

    /**
     * @brief Met à jour les sommets LOD sur le GPU
     */
    void updateVerticesGpuLod();

    void initTexture();

    GLuint getTextureId();

    Texture* getTexture();
};

#endif