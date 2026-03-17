#ifndef TEXTURE_H
#define TEXTURE_H

#include <vector>
#include <cstdint>
#include <glm/glm.hpp>
#include <GL/glew.h>

struct Region
{
    int mLowHeight;
    int mOptimalHeight;
    int mHighHeight;
};

struct Tiles
{
    Region mRegion;
    unsigned char* mTextureImage;

    uint16_t numTiles;
    uint16_t mWidth;
    uint16_t mHeight;
    uint16_t channels;

    glm::vec3 getColor(const int x,const int z);
};

class Texture
{
private:
    Tiles mTiles[4];
    unsigned int mNumTiles = 4;
    std::vector<glm::vec3> mTextureData;
public:
    Texture();
    ~Texture();

    void loadTile(const char* filname,const uint16_t numTile);
    void generateRegion(const float minHeight, const float maxHeight);
    float regionPercent(const float cHeight, const uint16_t numTiles);
    void generateTexture(const std::vector<float>& heightData,const int terrainWidth,const int terrainHeight);

    void setupTexture(const GLuint idTexture,const unsigned int width,const unsigned int height);
};

#endif

