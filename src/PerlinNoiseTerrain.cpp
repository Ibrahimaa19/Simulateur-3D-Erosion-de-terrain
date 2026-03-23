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
    float* data = mData.data();
    const int width = mWidth;
    const int height = mHeight;
    const float baseFrequency = mBaseFrequency;
    const int numOctaves = mNumOctaves;
    const float persistence = mPersistenceCoef;
    const float lacunarity = mLacunarityCoef;

    #pragma omp parallel for schedule(static)
    for (int z = 0; z < height; ++z)
    {
        const int rowOffset = z * width;
        const float yf = static_cast<float>(z) * baseFrequency;

        for (int x = 0; x < width; ++x)
        {
            const float xf = static_cast<float>(x) * baseFrequency;

            float noise = 0.0f;
            float amplitude = 1.0f;
            float freq = 1.0f;

            for (int o = 0; o < numOctaves; ++o)
            {
                noise += amplitude * Noise2D(xf * freq, yf * freq);
                amplitude *= persistence;
                freq *= lacunarity;
            }

            data[rowOffset + x] = noise;
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
    const int x0 = static_cast<int>(std::floor(x));
    const int y0 = static_cast<int>(std::floor(y));

    const int X = x0 & 255;
    const int Y = y0 & 255;

    const float xf = x - static_cast<float>(x0);
    const float yf = y - static_cast<float>(y0);

    const int pX = mPermutation[X];
    const int pX1 = mPermutation[X + 1];

    auto dot = [&](int hash, float dx, float dy)
    {
        float gx, gy;
        Gradient(hash, gx, gy);
        return gx * dx + gy * dy;
    };

    const float tl = dot(mPermutation[pX + Y],     xf,      yf);
    const float tr = dot(mPermutation[pX1 + Y],    xf - 1,  yf);
    const float bl = dot(mPermutation[pX + Y + 1], xf,      yf - 1);
    const float br = dot(mPermutation[pX1 + Y + 1],xf - 1,  yf - 1);

    const float u = Fade(xf);
    const float v = Fade(yf);

    const float lerp1 = Lerp(tl, tr, u);
    const float lerp2 = Lerp(bl, br, u);

    return Lerp(lerp1, lerp2, v);
}

void PerlinNoiseTerrain::Gradient(int hash, float& gx, float& gy) const
{
    static const float gradients[8][2] = {
        { 1.0f,  1.0f},
        {-1.0f,  1.0f},
        { 1.0f, -1.0f},
        {-1.0f, -1.0f},
        { 1.0f,  0.0f},
        {-1.0f,  0.0f},
        { 0.0f,  1.0f},
        { 0.0f, -1.0f}
    };

    const int idx = hash & 7;
    gx = gradients[idx][0];
    gy = gradients[idx][1];
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

    const float minVal = *minMax.first;
    const float maxVal = *minMax.second;
    const float minMaxDelta = maxVal - minVal;
    const float minMaxRange = mMaxHeight - mMinHeight;

    if (minMaxDelta == 0.0f)
    {
        std::fill(mData.begin(), mData.end(), mMinHeight);
        return;
    }

    const float scale = minMaxRange / minMaxDelta;

    #pragma omp parallel for schedule(static)
    for (int i = 0; i < static_cast<int>(mData.size()); ++i)
    {
        mData[i] = (mData[i] - minVal) * scale + mMinHeight;
    }
}