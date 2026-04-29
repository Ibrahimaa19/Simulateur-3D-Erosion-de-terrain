#include "Terrain.hpp"

#include "stb_image.hpp"

#include <algorithm>
#include <iostream>

void Terrain::loadTerrain(const char* imagePath, float yFactor, float xzFactor)
{
    int t_channels;

    unsigned char* image = stbi_load(imagePath, &this->mWidth, &this->mHeight, &t_channels, 1);

    if (image)
    {
        std::cout << "Loaded heightmap of size " << mHeight << " x " << mWidth << std::endl;
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
        return;
    }

    this->mData.resize(mHeight * mWidth);

    this->mBorderSize = 10;
    this->mCellSpacing = 1;
    this->mYFactor = yFactor;
    this->mXzFactor = xzFactor;

    for (int i = 0; i < mWidth * mHeight; i++)
    {
        this->mData[i] = image[i];
    }

    this->mMaxHeight = *std::max_element(mData.begin(), mData.end());
    this->mMinHeight = *std::min_element(mData.begin(), mData.end());

    stbi_image_free(image);
}

bool Terrain::isInside(int i, int j) const
{
    return (i >= 0 && i < mHeight && j >= 0 && j < mWidth);
}

void Terrain::setData(int i, float value)
{
    this->mData[i] += value;
}

std::vector<float>* Terrain::getData()
{
    return &(this->mData);
}
