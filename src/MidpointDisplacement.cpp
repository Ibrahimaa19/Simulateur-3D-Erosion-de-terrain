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
    
    this->width  = size;
    this->height = size;
    this->min_height = minHeight;
    this->max_height = maxHeight;
    this->yfactor = 1.0f;
    this->xzfactor = 1.0f / scale;
    this->borderSize = 0;

    this->data.assign(width * height, 0.0f);
    
    if(!isPowerOfTwo(size - 1))
    {
        printf("Invalid terrain size : %d (size must be 2^n + 1)\n", size);
        return;
    }
    CreateMidpointDisplacementInterne(roughness);
    Normalize();
}

void MidpointDisplacement::CreateMidpointDisplacementInterne(float roughness)
{
    int rectSize = height - 1;
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
    auto minMax = std::minmax_element(data.begin(), data.end());
    float min = *minMax.first;
    float max = *minMax.second;

    float minMaxDelta = max - min;
    float targetRange = max_height - min_height;

    for(auto& element : data)
    {
        element = (element - min) / minMaxDelta * targetRange + min_height;
    }
}

void MidpointDisplacement::DiamondStep(int rectSize, float curHeight)
{
    int halfRectSize = rectSize / 2;

    for (int z = 0; z < height; z += rectSize)
    {
        for (int x = 0; x < width; x += rectSize)
        {
            int nextZ = (z + rectSize) % height;
            int nextX = (x + rectSize) % width;

            float bottomLeft  = get_height(z, x);
            float bottomRight = get_height(z, nextX);
            float topLeft     = get_height(nextZ, x);
            float topRight    = get_height(nextZ, nextX);

            int midX = (x + halfRectSize) % width;
            int midZ = (z + halfRectSize) % height;

            float average = (bottomLeft + bottomRight + topLeft + topRight) / 4.0f;

            set_height(midX, midZ, average + RandomFloat(-curHeight, curHeight));
        }
    }
}

void MidpointDisplacement::SquareStep(int rectSize, float curHeight)
{
    int halfRectSize = rectSize / 2;

    for (int z = 0; z < height; z += halfRectSize)
    {
        for (int x = (z + halfRectSize) % rectSize; x < width; x += rectSize)
        {
            float sum = 0.0f;
            int count = 0;

            if (inside(z - halfRectSize, x))
            {
                sum += get_height(x, z - halfRectSize);
                count++;
            }

            if (inside(z + halfRectSize, x))
            {
                sum += get_height(x, z + halfRectSize);
                count++;
            }

            if (inside(z, x - halfRectSize))
            {
                sum += get_height(x - halfRectSize, z);
                count++;
            }

            if (inside(z, x + halfRectSize))
            {
                sum += get_height(x + halfRectSize, z);
                count++;
            }

            float average = sum / count;

            set_height(x, z, average + RandomFloat(-curHeight, curHeight));
        }
    }
}
