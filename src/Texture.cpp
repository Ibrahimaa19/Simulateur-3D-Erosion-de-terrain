#include "Texture.hpp"
#include "Terrain.hpp"


glm::vec3 Tiles::getColor(const int x,const int z){
    int index = (z*mWidth + x)*channels;
    return glm::vec3((int)mTextureImage[index],(int)mTextureImage[index+1],(int)mTextureImage[index+2]);
}

Texture::Texture(/* args */)
{
}

Texture::~Texture()
{
}

void Texture::loadTile(const char* filename,const uint16_t numTile)
{
    int t_channels,width,height;
    Tiles* tile = &this->mTiles[numTile];
    
    tile->mTextureImage = stbi_load(filename, &width, &height, &t_channels, 0);


    std::cout << "Erreur STBI: " << stbi_failure_reason() << std::endl;
    //std::cout << tile->mTextureImage << std::endl;
    
    tile->channels = t_channels;
    tile->mWidth = width;
    tile->mHeight = height;

    if (tile->mTextureImage)
    {
        std::cout << "Loaded Texture of size " << height << " x " << width << std::endl;
    }
    else
    {
        std::cout << "Failed to load texture." << std::endl;
    }

}

void Texture::generateRegion(const float minHeight, const float maxHeight)
{
    float rangeHeight = maxHeight - minHeight;
    int iLastHeight = -1;

    float rangePerTile = rangeHeight / mNumTiles;

    for(int i =0; i < mNumTiles;++i){
        mTiles[i].mRegion.mLowHeight = iLastHeight +1;

        iLastHeight += rangePerTile;
        mTiles[i].mRegion.mOptimalHeight = iLastHeight;

        mTiles[i].mRegion.mHighHeight = (mTiles[i].mRegion.mOptimalHeight)+rangePerTile;
    }
}

float Texture::regionPercent(const float cHeight,const uint16_t numTiles)
{
    float fTemp1,fTemp2;
    const auto& region = mTiles[numTiles].mRegion;

    if (cHeight > (float)mTiles[numTiles].mRegion.mHighHeight || cHeight < (float)mTiles[numTiles].mRegion.mLowHeight){
        return 0.f;
    }

    if(cHeight < mTiles[numTiles].mRegion.mOptimalHeight){
        float range = region.mOptimalHeight - region.mLowHeight;
        if (range <= 0.0f) return 0.0f; 
        return (cHeight - region.mLowHeight) / range;

    }else if(cHeight >= mTiles[numTiles].mRegion.mOptimalHeight){

        float range = region.mHighHeight - region.mOptimalHeight;
        if (range <= 0.0f) return 0.0f; 
        return (region.mHighHeight - cHeight) / range;

    }else{
        perror("Hors bordures de region.");
        exit(1);
    }
}

void Texture::generateTexture(const std::vector<float>& heightData,const int terrainWidth,const int terrainHeight)
{
    float totalBlend,fBlend,widthRatio,heightRatio;
    glm::vec3 color;

    mTextureData.clear();

    for(int i=0;i<terrainHeight;++i)
    {
        for(int j=0;j<terrainWidth;++j)
        {
            float red = 0.f;
            float green = 0.f;
            float blue = 0.f;
            totalBlend = 0.f;

            for(int t=0;t<mNumTiles;++t)
            {
                widthRatio = (float)terrainWidth/(float)mTiles[t].mWidth;
                heightRatio = (float)terrainHeight/(float)mTiles[t].mHeight;

                color = mTiles[t].getColor(i*widthRatio,j*heightRatio);

                fBlend = regionPercent(heightData[i*terrainWidth +j],t);

                totalBlend += fBlend;

                red += color.x*fBlend;   // Red
                green += color.y*fBlend;   // Green
                blue += color.z*fBlend;   // Blue
            }
            
            if(totalBlend > 0)
            {
                red = red/totalBlend;
                green = green/totalBlend;
                blue = blue/totalBlend;
            }

            red = glm::clamp(red / 255.0f, 0.0f, 1.0f);
            green = glm::clamp(green / 255.0f, 0.0f, 1.0f);
            blue = glm::clamp(blue / 255.0f, 0.0f, 1.0f);

            mTextureData.push_back(glm::vec3(red,green,blue));
            
        }
    }
}

void Texture::setupTexture(const GLuint idTexture,const unsigned int width,const unsigned int height)
{
    glBindTexture(GL_TEXTURE_2D, idTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, width, height, 0, GL_RGB, GL_FLOAT, mTextureData.data());
    
    // Optionnel : régénérer les mipmaps si nécessaire
    glGenerateMipmap(GL_TEXTURE_2D);
}