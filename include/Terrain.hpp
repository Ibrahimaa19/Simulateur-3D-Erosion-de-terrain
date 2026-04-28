#ifndef TERRAIN_H
#define TERRAIN_H

#include <vector>

/**
 * @brief Terrain CPU indépendant du rendu.
 *
 * Cette classe ne dépend ni d'OpenGL, ni de GLM. Elle sert de structure de
 * données commune pour la génération, l'érosion, les tests et les exécutables
 * HPC/headless. Le rendu consomme ces données via RendererManager.
 */
class Terrain
{
  protected:
    std::vector<float> mData; /**< Matrice des valeurs de hauteur */
    int mHeight = 0;          /**< Hauteur du terrain en nombre de cellules */
    int mWidth = 0;           /**< Largeur du terrain en nombre de cellules */
    float mYFactor = 1.0f;    /**< Facteur d'échelle sur l'axe Y (hauteur) */
    float mXzFactor = 1.0f;   /**< Facteur d'échelle sur les axes X et Z */
    float mMaxHeight = 0.0f;  /**< Hauteur maximale du terrain */
    float mMinHeight = 0.0f;  /**< Hauteur minimale du terrain */
    int mBorderSize = 0;      /**< Taille de la bordure (aplatie) */
    int mCellSpacing = 1;     /**< Espacement entre les cellules */

  public:
    virtual ~Terrain() = default;

    /**
     * @brief Charge un terrain à partir d'une image heightmap.
     */
    void loadTerrain(const char* imagePath, float yFactor, float xzFactor);

    float getHeight(int i, int j) const
    {
        return mData[j * mWidth + i];
    }

    void setHeight(int i, int j, float value)
    {
        mData[j * mWidth + i] = value;
    }

    float getMaxHeight() const
    {
        return mMaxHeight;
    }

    float getMinHeight() const
    {
        return mMinHeight;
    }

    int getTerrainHeight() const
    {
        return mHeight;
    }

    int getTerrainWidth() const
    {
        return mWidth;
    }

    float getXzFactor() const
    {
        return mXzFactor;
    }

    void setData(int i, float value);
    std::vector<float>* getData();

    bool isInside(int i, int j) const;
};

#endif
