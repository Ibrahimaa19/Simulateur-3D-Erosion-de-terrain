#include "Terrain.hpp"

#include "RendererManager.hpp"
#include "ThermalErosion.hpp"
#include <fstream>

void Terrain::loadTerrain(const char *imagePath, float yFactor, float xzFactor)
{
    int t_channels;

    unsigned char *image = stbi_load(imagePath, &this->mWidth, &this->mHeight, &t_channels, 1);

    if (image)
    {
        std::cout << "Loaded heightmap of size " << mHeight << " x " << mWidth << std::endl;
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
    }

    this->mData.resize(mHeight * mWidth);

    this->mBorderSize = 10;
    this->mCellSpacing = 1;
    this->mYFactor = yFactor;
    this->mXzFactor = xzFactor;

    for (int i = 0; i < mWidth * mHeight; i++)
    {
        this->mData[i] = image[i] / 255.0f;
    }

    this->mMaxHeight = *std::max_element(mData.begin(), mData.end());
    this->mMinHeight = *std::min_element(mData.begin(), mData.end());

    this->mRenderer = (std::make_unique<RendererManager>(this));

    stbi_image_free(image);

    createPatches();
}

void Terrain::createPatches()
{
    int nbPatchX = std::ceil(mWidth / 32);
    int nbPatchZ = std::ceil(mHeight / 32);

    std::cout << "nb_patch_x : " << nbPatchX << " ,nb_patch_z : " << nbPatchZ << std::endl;
    for (int i = 0; i < nbPatchX; ++i)
    {
        for (int j = 0; j < nbPatchZ; ++j)
        {
            std::unique_ptr<Patch> p = std::make_unique<Patch>();
            p->setPatch(i, j, mXzFactor, nbPatchX, nbPatchZ,mTexture);
            this->mPatches.push_back(std::move(p));
        }
    }
}

bool Terrain::isInside(int i, int j) const
{
    return (i >= 0 && i < mHeight && j >= 0 && j < mWidth);
}

void Terrain::setData(int i, float value)
{
    this->mData[i] += value;
}

std::vector<float> *Terrain::getData()
{
    return &(this->mData);
}

void Terrain::loadVerticesLod()
{
    for (int i = 0; i < mPatches.size(); ++i)
    {
        mPatches[i]->generateLodVertices(mData, mWidth, mHeight);
    }
}

void Terrain::loadIndicesLod()
{
    for (int i = 0; i < mPatches.size(); ++i)
    {
        mPatches[i]->generateLodIndices(mData, mWidth, mHeight);
    }
}

void Terrain::setupTerrainLod(GLuint &VAO, GLuint &VBO, GLuint &EBO)
{
    this->loadIndicesLod();
    this->loadVerticesLod();

    for (int i = 0; i < mPatches.size(); ++i)
    {
        mPatches[i]->createBuffersGL();
    }
}

void Terrain::updateVerticesGpuLod()
{
    loadVerticesLod();
}

Frustrum &Terrain::getFrustrum()
{
    return this->mFrustrum;
}

std::vector<std::unique_ptr<Patch>> &Terrain::getPatches()
{
    return this->mPatches;
}

RendererManager *Terrain::getRendererManager()
{
    return mRenderer.get();
}

void Terrain::setRenderer(std::unique_ptr<RendererManager> renderer)
{
    mRenderer = std::move(renderer);
    if (mRenderer)
    {
        mRenderer->setTerrain(this);
    }
}

void Terrain::initTexture()
{
    this->mTexture = new Texture();
    mTexture->generateRegion(mMinHeight,mMaxHeight);

    glBindTexture(GL_TEXTURE_2D, mTextureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    mTexture->loadTile("../src/texture/IMGP5525_seamless.jpg",0);
    mTexture->loadTile("../src/texture/IMGP5487_seamless.jpg",1);
    mTexture->loadTile("../src/texture/grass.jpg",2);
    mTexture->loadTile("../src/texture/water.jpg",3);
}

GLuint Terrain::getTextureId()
{
    return this->mTextureID;
}

Texture* Terrain::getTexture()
{
    return this->mTexture;
}