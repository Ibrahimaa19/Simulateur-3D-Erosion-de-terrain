#include "PerlinNoiseTerrain.hpp"
#include "Terrain.hpp"
#include "ThermalErosion.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <numeric>
#include <utility>
#include <vector>

namespace
{
class TestTerrain : public Terrain
{
  public:
    TestTerrain(int width, int height, std::vector<float> data)
    {
        mWidth = width;
        mHeight = height;
        mData = std::move(data);
        auto minmax = std::minmax_element(mData.begin(), mData.end());
        mMinHeight = *minmax.first;
        mMaxHeight = *minmax.second;
    }
};

float sumHeights(const std::vector<float>& values)
{
    return std::accumulate(values.begin(), values.end(), 0.0f);
}
} // namespace

int main()
{
    PerlinNoiseTerrain generated;
    generated.CreatePerlinNoise(16, 12, 0.0f, 255.0f);
    assert(generated.getTerrainWidth() == 16);
    assert(generated.getTerrainHeight() == 12);
    assert(generated.getData()->size() == 16 * 12);

    TestTerrain terrain(5, 5,
                        {
                            0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 9.0f, 2.0f, 1.0f, 0.0f, 0.0f, 3.0f, 8.0f,
                            2.0f, 0.0f, 0.0f, 1.0f, 2.0f, 7.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                        });

    const float before = sumHeights(*terrain.getData());

    ThermalErosion erosion;
    erosion.loadTerrainInfo(terrain);
    erosion.useFourNeighbors();
    erosion.setTalusAngle(15.0f);
    erosion.setTransferRate(0.5f);
    const int modifiedCells = erosion.stepPureTwoPhase();

    const float after = sumHeights(*terrain.getData());
    assert(modifiedCells > 0);
    assert(std::fabs(before - after) < 1e-3f);

    return 0;
}
