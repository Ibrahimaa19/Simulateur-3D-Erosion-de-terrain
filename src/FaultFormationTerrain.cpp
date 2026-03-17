#include "FaultFormationTerrain.hpp"
#include <algorithm>
#include <utility>
#include <cstdlib>
#include <cstdio>

FaultFormationTerrain::FaultFormationTerrain()
{
}

void FaultFormationTerrain::CreateFaultFormation(int width, int height, int iterations, float minHeight, float maxHeight, float scale, bool applyFilter, float filter)
{
    this->mWidth = width;
    this->mHeight = height;
    this->mMinHeight = minHeight;
    this->mMaxHeight = maxHeight;
    this->mYFactor = 1;
    this->mXzFactor = 1.0f / scale;
    this->mBorderSize = 0;

    this->mRenderer = (std::make_unique<RendererManager>(this));

    this->mData.assign(width * height, 0.0f);

    CreateFaultFormationInternal(iterations, minHeight, maxHeight, applyFilter, filter);

    Normalize();
    
    createPatches();
}

bool FaultFormationTerrain::TerrainPoint::IsEqual(TerrainPoint& p) const
{
    return (x == p.x) && (z == p.z);
}

void FaultFormationTerrain::CreateFaultFormationInternal(int iterations, float minHeight, float maxHeight, bool applyFilter, float filter)
{
    float deltaHeight = maxHeight - minHeight;

    for (int curIter = 0; curIter < iterations; ++curIter)
    {
        float iterationRatio = (float)curIter / (float)iterations;
        float h = maxHeight - iterationRatio * deltaHeight;

        TerrainPoint p1, p2;
        GenRandomTerrainPoints(p1, p2);

        int dirX = p2.x - p1.x;
        int dirZ = p2.z - p1.z;

        for (int z = 0; z < mHeight; ++z)
        {
            for (int x = 0; x < mWidth; ++x)
            {
                int crossProduct = dirX * (z - p1.z) - dirZ * (x - p1.x);

                if (crossProduct > 0)
                {
                    float val = getHeight(x, z) + h;
                    setHeight(x, z, val);
                }
            }
        }
    }

    if (applyFilter)
        ApplyFIRFilter(filter);
}

void FaultFormationTerrain::GenRandomTerrainPoints(TerrainPoint& p1, TerrainPoint& p2)
{
    p1.x = std::rand() % mWidth;
    p1.z = std::rand() % mHeight;

    int counter = 0;

    do
    {
        p2.x = std::rand() % mWidth;
        p2.z = std::rand() % mHeight;

        if (++counter == 1000)
        {
            printf("Endless loop detected in %s:%d\n", __FILE__, __LINE__);
            exit(1);
        }

    } while (p1.IsEqual(p2));
}

void FaultFormationTerrain::Normalize()
{
    auto minMax = std::minmax_element(mData.begin(), mData.end());

    float min = *minMax.first;
    float max = *minMax.second;

    float minMaxDelta = max - min;
    float minMaxRange = mMaxHeight - mMinHeight;
    for(auto& element: mData)
    {
        element = (element - min)/minMaxDelta * minMaxRange + mMinHeight;
    }
}

void FaultFormationTerrain::ApplyFIRFilter(float filter)
{
    for (int z = 0; z < mHeight; z++)
    {
        float PrevVal = getHeight(0, z);

        for (int x = 1; x < mWidth; x++)
        {
            PrevVal = FIRFilterSinglePoint(x, z, PrevVal, filter);
        }
    }

    for (int z = 0; z < mHeight; z++)
    {
        float PrevVal = getHeight(mWidth - 1, z);

        for (int x = mWidth - 2; x >= 0; x--)
        {
            PrevVal = FIRFilterSinglePoint(x, z, PrevVal, filter);
        }
    }

    for (int x = 0; x < mWidth; x++)
    {
        float PrevVal = getHeight(x, 0);

        for (int z = 1; z < mHeight; z++)
        {
            PrevVal = FIRFilterSinglePoint(x, z, PrevVal, filter);
        }
    }

    for (int x = 0; x < mWidth; x++)
    {
        float PrevVal = getHeight(x, mHeight - 1);

        for (int z = mHeight - 2; z >= 0; z--)
        {
            PrevVal = FIRFilterSinglePoint(x, z, PrevVal, filter);
        }
    }
}

float FaultFormationTerrain::FIRFilterSinglePoint(int x, int z, float PrevVal, float filter)
{
    float CurVal = getHeight(x, z);
    float NewVal = filter * PrevVal + (1.0f - filter) * CurVal;
    setHeight(x, z, NewVal);
    return NewVal;
}