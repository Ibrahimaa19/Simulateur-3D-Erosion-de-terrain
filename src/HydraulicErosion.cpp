#include "HydraulicErosion.hpp"
#include <algorithm>

namespace {
    constexpr int MAX_STEPS = 256;
    constexpr float MIN_WATER = 0.01f;
}

HydraulicErosion::HydraulicErosion(int iterations,
                                   float rain,
                                   float erosionRate,
                                   float depositRate,
                                   float evaporation)
    : iterations(iterations),
      rain(rain),
      erosionRate(erosionRate),
      depositRate(depositRate),
      evaporation(evaporation),
      m_rng(0)
{
}

void HydraulicErosion::setSeed(unsigned int seed)
{
    m_rng.seed(seed);
}

void HydraulicErosion::setParams(int iterations, float rain, float erosionRate, float depositRate, float evaporation)
{
    this->iterations = iterations;
    this->rain = rain;
    this->erosionRate = erosionRate;
    this->depositRate = depositRate;
    this->evaporation = evaporation;
}

int HydraulicErosion::processOneDroplet(const Terrain& terrain, std::vector<float>& delta)
{
    const int H = terrain.get_terrain_height();
    const int W = terrain.get_terrain_width();
    if (H < 3 || W < 3 || delta.size() != static_cast<size_t>(W * H)) return 0;

    std::uniform_int_distribution<int> distI(1, H - 2);
    std::uniform_int_distribution<int> distJ(1, W - 2);

    int i = distI(m_rng);
    int j = distJ(m_rng);

    float water = rain;
    float sediment = 0.0f;

    for (int step = 0; step < MAX_STEPS; step++)
    {
        // Lecture du terrain uniquement (snapshot implicite : toutes les gouttes du batch lisent le même état)
        const float h = terrain.get_height(j, i);

        int lowestI = i;
        int lowestJ = j;
        float lowestH = h;

        for (int di = -1; di <= 1; di++)
        {
            for (int dj = -1; dj <= 1; dj++)
            {
                if (di == 0 && dj == 0) continue;

                int ni = i + di;
                int nj = j + dj;

                if (!terrain.inside(ni, nj)) continue;

                float hn = terrain.get_height(nj, ni);

                if (hn < lowestH)
                {
                    lowestH = hn;
                    lowestI = ni;
                    lowestJ = nj;
                }
            }
        }

        if (lowestI == i && lowestJ == j)
        {
            float depositAmount = sediment * depositRate;
            delta[i * W + j] += depositAmount;
            return 1;
        }

        float slope = h - lowestH;
        float erodeAmount = erosionRate * slope * water;
        erodeAmount = std::min(erodeAmount, h);

        delta[i * W + j] -= erodeAmount;
        sediment += erodeAmount;

        i = lowestI;
        j = lowestJ;

        water *= (1.0f - evaporation);
        if (water < MIN_WATER)
            return 1;
    }
    return 1;
}

void HydraulicErosion::apply(Terrain& terrain)
{
    int remaining = iterations;
    while (remaining > 0)
    {
        int batch = std::min(m_batchSize, remaining);
        applyBatch(terrain, batch);
        remaining -= batch;
    }
}

int HydraulicErosion::applyBatch(Terrain& terrain, int batchSize)
{
    const int W = terrain.get_terrain_width();
    const int H = terrain.get_terrain_height();
    if (H < 3 || W < 3) return 0;

    const size_t totalCells = static_cast<size_t>(W) * H;
    if (m_delta.size() != totalCells)
        m_delta.resize(totalCells);

    std::fill(m_delta.begin(), m_delta.end(), 0.0f);

    for (int k = 0; k < batchSize; k++)
        processOneDroplet(terrain, m_delta);

    std::vector<float>* data = terrain.get_data();
    for (size_t idx = 0; idx < totalCells; idx++)
        (*data)[idx] += m_delta[idx];

    return batchSize;
}
