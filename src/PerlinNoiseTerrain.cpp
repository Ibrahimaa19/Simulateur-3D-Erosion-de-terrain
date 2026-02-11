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
    this->width = width;
    this->height = height;
    this->min_height = minHeight;
    this->max_height = maxHeight;
    this->yfactor = 1.0f;
    this->xzfactor = 1.0f / scale;
    this->borderSize = 0;

    this->data.assign(width * height, 0.0f);

    mBaseFrequency = frequency;
    mNumOctaves = octaves;
    mPersistenceCoef = persistence;
    mLacunarityCoef = lacunarity;

    InitPermutation();
    CreatePerlinNoiseInternal(minHeight, maxHeight);
    Normalize();
}

void PerlinNoiseTerrain::CreatePerlinNoiseInternal(float minH, float maxH)
{
    for (int z = 0; z < height; z++)
    {
        for (int x = 0; x < width; x++)
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

            set_height(x, z, noise);
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
    auto minMax = std::minmax_element(data.begin(), data.end());

    float min = *minMax.first;
    float max = *minMax.second;

    float minMaxDelta = max - min;
    float minMaxRange = max_height - min_height;

    for (auto& element : data)
    {
        element = (element - min) / minMaxDelta * minMaxRange + min_height;
    }
}
