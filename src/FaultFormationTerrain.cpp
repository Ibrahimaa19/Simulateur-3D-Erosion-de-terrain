#include "FaultFormationTerrain.hpp"
#include <algorithm>
#include <utility>



FaultFormationTerrain::FaultFormationTerrain()
{
    
}

void FaultFormationTerrain::CreateFaultFormation(int width, int height, int iterations, float minHeight, float maxHeight, float scale, bool applyFilter, float filter)
{
    this->width = width;
    this->height = height;
    this->min_height = minHeight;
    this->max_height = maxHeight;
    this->yfactor = 1;
    this->xzfactor = 1.0f / scale;
    this->borderSize = 0;
    this->data.resize(width * height, 0.0f);
    CreateFaultFormationInternal(iterations, minHeight, maxHeight, applyFilter, filter);

    Normalize();
}

bool FaultFormationTerrain::TerrainPoint::IsEqual(TerrainPoint& p) const
{
    return ((x == p.x) && (z == p.z));
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
        int crossProduct;
        for (int z = 0; z < this->height; ++z)
        {
            for (int x = 0; x < this->width; ++x)
            {
                crossProduct = dirX * (z - p1.z) - dirZ * (x - p1.x);
                if(crossProduct > 0)
                {
                    data[z * this->width + x] = data[z * this->width + x] + h;
                }
            }
            
        }
        
    }
    if(applyFilter)
        ApplyFIRFilter(filter);
}

void FaultFormationTerrain::GenRandomTerrainPoints(TerrainPoint& p1, TerrainPoint& p2)
{
    p1.x = std::rand() % width;
    p1.z = std::rand() % height;
    int counter = 0;

    do
    {
        p2.x = rand() % width;
        p2.z = rand() % height;
        
        if(++counter == 1000)
        {
            printf("Endless loop detected in %s:%d\n", __FILE__, __LINE__);
            exit(1);
        }
    }while(p1.IsEqual(p2));
}

void FaultFormationTerrain::Normalize()
{
    auto minMax = std::minmax_element(data.begin(), data.end());

    float min = *minMax.first;
    float max = *minMax.second;

    float minMaxDelta = max - min;
    float minMaxRange = max_height - min_height;
    for(auto& element: data)
    {
        element = (element - min)/minMaxDelta * minMaxRange + min_height;
    }
}

void FaultFormationTerrain::ApplyFIRFilter(float filter)
{
    // left to right
    for (int z = 0 ; z < height ; z++) {
        float PrevVal = data[z * width];
        for (int x = 1 ; x < width ; x++) {
            PrevVal = FIRFilterSinglePoint(x, z, PrevVal, filter);
        }
    }

    // right to left
    for (int z = 0 ; z < height ; z++) {
        float PrevVal = data[z * width + width - 1];
        for (int x = width - 2 ; x >= 0 ; x--) {
            PrevVal = FIRFilterSinglePoint(x, z, PrevVal, filter);
        }
    }

    // bottom to top
    for (int x = 0 ; x < width ; x++) {
        float PrevVal = data[x];
        for (int z = 1 ; z < height ; z++) {
            PrevVal = FIRFilterSinglePoint(x, z, PrevVal, filter);
        }
    }

    // top to bottom
    for (int x = 0 ; x < width ; x++) {
        float PrevVal = data[(height - 1)* width + x];
        for (int z = height - 2 ; z >= 0 ; z--) {
            PrevVal = FIRFilterSinglePoint(x, z, PrevVal, filter);
        }
    }
}


float FaultFormationTerrain::FIRFilterSinglePoint(int x, int z, float PrevVal, float filter)
{
    float CurVal = data[z * width + x];
    float NewVal = filter * PrevVal + (1 - filter) * CurVal;
    data[z * width + x] = NewVal;
    return NewVal;
}