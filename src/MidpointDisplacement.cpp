#include "MidpointDisplacement.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>

static float RandomFloat(float a, float b)
{
    return a + (b - a) * (float(rand()) / float(RAND_MAX));
}
static bool isPowerOfTwo(int n) {
    return n > 0 && (n & (n - 1)) == 0;
}

MidpointDisplacement::MidpointDisplacement()
{
}

void MidpointDisplacement::CreateMidpointDisplacement(int size, float minHeight, float maxHeight, float scale, float roughness)
{
    
    this->mWidth  = size;
    this->mHeight = size;
    this->mMinHeight = minHeight;
    this->mMaxHeight = maxHeight;
    this->mYFactor = 1.0f;
    this->mXzFactor = 1.0f / scale;
    this->mBorderSize = 0;

    this->mData.assign(mWidth * mHeight, 0.0f);

    this->mRenderer = (std::make_unique<RendererManager>(this));
    
    if(!isPowerOfTwo(size - 1))
    {
        printf("Invalid terrain size : %d (size must be 2^n + 1)\n", size);
        return;
    }
    CreateMidpointDisplacementInterne(roughness);
    Normalize();

    createPatches();
}

void MidpointDisplacement::CreateMidpointDisplacementInterne(float roughness)
{
    int rectSize = mHeight - 1;
    float curHeight = (float)rectSize / 2.0f;
    float heightReduce = std::pow(2.0f, -roughness);

    while (rectSize > 1)
    {
        DiamondStep(rectSize, curHeight);
        SquareStep(rectSize, curHeight);

        rectSize /= 2;
        curHeight *= heightReduce;
    }
}

void MidpointDisplacement::Normalize()
{
    auto minMax = std::minmax_element(mData.begin(), mData.end());
    float min = *minMax.first;
    float max = *minMax.second;

    float minMaxDelta = max - min;
    float targetRange = mMaxHeight - mMinHeight;

    for(auto& element : mData)
    {
        element = (element - min) / minMaxDelta * targetRange + mMinHeight;
    }
}

void MidpointDisplacement::DiamondStep(int rectSize, float curHeight)
{
    int halfRectSize = rectSize / 2;

    for (int z = 0; z < mHeight; z += rectSize)
    {
        for (int x = 0; x < mWidth; x += rectSize)
        {
            int nextZ = (z + rectSize) % mHeight;
            int nextX = (x + rectSize) % mWidth;

            float bottomLeft  = getHeight(z, x);
            float bottomRight = getHeight(z, nextX);
            float topLeft     = getHeight(nextZ, x);
            float topRight    = getHeight(nextZ, nextX);

            int midX = (x + halfRectSize) % mWidth;
            int midZ = (z + halfRectSize) % mHeight;

            float average = (bottomLeft + bottomRight + topLeft + topRight) / 4.0f;

            setHeight(midX, midZ, average + RandomFloat(-curHeight, curHeight));
        }
    }
}

void MidpointDisplacement::SquareStep(int rectSize, float curHeight)
{
    int halfRectSize = rectSize / 2;

    for (int z = 0; z < mHeight; z += halfRectSize)
    {
        for (int x = (z + halfRectSize) % rectSize; x < mWidth; x += rectSize)
        {
            float sum = 0.0f;
            int count = 0;

            if (isInside(z - halfRectSize, x))
            {
                sum += getHeight(x, z - halfRectSize);
                count++;
            }

            if (isInside(z + halfRectSize, x))
            {
                sum += getHeight(x, z + halfRectSize);
                count++;
            }

            if (isInside(z, x - halfRectSize))
            {
                sum += getHeight(x - halfRectSize, z);
                count++;
            }

            if (isInside(z, x + halfRectSize))
            {
                sum += getHeight(x + halfRectSize, z);
                count++;
            }

            float average = sum / count;

            setHeight(x, z, average + RandomFloat(-curHeight, curHeight));
        }
    }
}