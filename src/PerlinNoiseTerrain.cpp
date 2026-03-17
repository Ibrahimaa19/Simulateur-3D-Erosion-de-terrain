#include "PerlinNoiseTerrain.hpp"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <cstdlib>

PerlinNoiseTerrain::PerlinNoiseTerrain() {}

void PerlinNoiseTerrain::CreatePerlinNoise(int width, int height,
                                           float minHeight, float maxHeight,
                                           float scale,
                                           float frequency,
                                           int octaves,
                                           float persistence,
                                           float lacunarity)
{
    this->mWidth = width;
    this->mHeight = height;
    this->mMinHeight = minHeight;
    this->mMaxHeight = maxHeight;
    this->mYFactor = 1.0f;
    this->mXzFactor = 1.0f / scale;
    this->mBorderSize = 0;

    this->mData.assign(width * height, 0.0f);

    this->mRenderer = (std::make_unique<RendererManager>(this));

    mBaseFrequency = frequency;
    mNumOctaves = octaves;
    mPersistenceCoef = persistence;
    mLacunarityCoef = lacunarity;

    InitPermutation();
    CreatePerlinNoiseInternal(minHeight, maxHeight);
    Normalize();

    createPatches();
}

void PerlinNoiseTerrain::CreatePerlinNoiseInternal(float minH, float maxH)
{
    for (int z = 0; z < mHeight; z++)
    {
        for (int x = 0; x < mWidth; x++)
        {
            float xf = static_cast<float>(x) * mBaseFrequency;
            float yf = static_cast<float>(z) * mBaseFrequency;

            float noise = 0.0f;
            float amplitude = 1.0f;
            float freq = 1.0f;

            for (int o = 0; o < mNumOctaves; o++)
            {
                noise += amplitude * Noise2D(xf * freq, yf * freq);
                amplitude *= mPersistenceCoef;
                freq *= mLacunarityCoef;
            }

            setHeight(x, z, noise);
        }
    }
}

void PerlinNoiseTerrain::InitPermutation()
{
    mPermutation.resize(512);
    std::vector<int> p(256);
    std::iota(p.begin(), p.end(), 0);

    for (int i = 255; i > 0; --i)
    {
        int j = rand() % (i + 1);
        std::swap(p[i], p[j]);
    }

    for (int i = 0; i < 512; i++)
        mPermutation[i] = p[i % 256];
}

float PerlinNoiseTerrain::Noise2D(float x, float y) const
{
    int X = static_cast<int>(std::floor(x)) & 255;
    int Y = static_cast<int>(std::floor(y)) & 255;

    float xf = x - std::floor(x);
    float yf = y - std::floor(y);

    auto dot = [&](int hash, float dx, float dy)
    {
        float gx, gy;
        Gradient(hash, gx, gy);
        return gx * dx + gy * dy;
    };

    float tl = dot(mPermutation[mPermutation[X] + Y],     xf,       yf);
    float tr = dot(mPermutation[mPermutation[X+1] + Y],   xf-1.0f,  yf);
    float bl = dot(mPermutation[mPermutation[X] + Y+1],   xf,       yf-1.0f);
    float br = dot(mPermutation[mPermutation[X+1] + Y+1], xf-1.0f,  yf-1.0f);

    float u = Fade(xf);
    float v = Fade(yf);

    float lerp1 = Lerp(tl, tr, u);
    float lerp2 = Lerp(bl, br, u);

    return Lerp(lerp1, lerp2, v);
}

void PerlinNoiseTerrain::Gradient(int hash, float& gx, float& gy) const
{
    switch (hash & 7)
    {
    case 0: gx = 1; gy = 1; break;
    case 1: gx = -1; gy = 1; break;
    case 2: gx = 1; gy = -1; break;
    case 3: gx = -1; gy = -1; break;
    case 4: gx = 1; gy = 0; break;
    case 5: gx = -1; gy = 0; break;
    case 6: gx = 0; gy = 1; break;
    case 7: gx = 0; gy = -1; break;
    }
}

inline float PerlinNoiseTerrain::Fade(float t) const
{
    return t * t * t * (t * (t * 6 - 15) + 10);
}

inline float PerlinNoiseTerrain::Lerp(float a, float b, float t) const
{
    return a + t * (b - a);
}

void PerlinNoiseTerrain::Normalize()
{
    auto minMax = std::minmax_element(mData.begin(), mData.end());

    float min = *minMax.first;
    float max = *minMax.second;

    float minMaxDelta = max - min;
    float minMaxRange = mMaxHeight - mMinHeight;

    for (auto& element : mData)
    {
        element = (element - min) / minMaxDelta * minMaxRange + mMinHeight;
    }
}