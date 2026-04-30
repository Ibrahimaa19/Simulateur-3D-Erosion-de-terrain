#include "ThermalErosionCore.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

float thermalTalusThresholdFromDegrees(float angleDegrees)
{
    constexpr float pi = 3.14159265f;
    return std::tan(angleDegrees * pi / 180.0f);
}

bool isThermalNeighborModeAvailable(ThermalNeighborMode mode)
{
    return mode == ThermalNeighborMode::Eight;
}

ThermalErosionStepStats runThermalErosionStep(std::vector<float>& data,
                                              int width,
                                              int height,
                                              float talusThreshold,
                                              float transferRate,
                                              ThermalNeighborMode mode)
{
    if (!isThermalNeighborModeAvailable(mode))
    {
        throw std::invalid_argument("Le voisinage 4 n'est pas implemente dans cette branche");
    }

    const long long expectedSize = static_cast<long long>(width) * static_cast<long long>(height);
    if (width <= 0 || height <= 0 || static_cast<long long>(data.size()) != expectedSize)
    {
        throw std::invalid_argument("Dimensions invalides pour l'erosion thermique");
    }

    ThermalErosionStepStats stats;
    if (width < 3 || height < 3)
    {
        return stats;
    }

    const int offsets[8][2] = {
        {-1, 0}, {1, 0}, {0, -1}, {0, 1},
        {-1, -1}, {-1, 1}, {1, -1}, {1, 1}
    };

    std::vector<float> nextData = data;

    // Boucle 8 voisins identique au comportement historique de ThermalErosion.
    for (int i = 1; i < height - 1; i++)
    {
        for (int j = 1; j < width - 1; j++)
        {
            const float currentHeight = data[i * width + j];

            const float dist[8] = {
                currentHeight - data[(i - 1) * width + j],
                currentHeight - data[(i + 1) * width + j],
                currentHeight - data[i * width + (j - 1)],
                currentHeight - data[i * width + (j + 1)],
                currentHeight - data[(i - 1) * width + (j - 1)],
                currentHeight - data[(i - 1) * width + (j + 1)],
                currentHeight - data[(i + 1) * width + (j - 1)],
                currentHeight - data[(i + 1) * width + (j + 1)]
            };

            float totalDiff = 0.0f;
            int validNeighbors = 0;

            for (int k = 0; k < 8; k++)
            {
                if (dist[k] > talusThreshold)
                {
                    totalDiff += dist[k];
                    validNeighbors++;
                }
            }

            if (totalDiff > 0.0f && validNeighbors > 0)
            {
                float materialToMove = transferRate * (totalDiff / validNeighbors);
                materialToMove = std::min(materialToMove, currentHeight * transferRate);

                nextData[i * width + j] -= materialToMove;

                for (int k = 0; k < 8; k++)
                {
                    if (dist[k] > talusThreshold)
                    {
                        const float proportion = dist[k] / totalDiff;
                        const float moveAmount = materialToMove * proportion;
                        const int ni = i + offsets[k][0];
                        const int nj = j + offsets[k][1];

                        nextData[ni * width + nj] += moveAmount;
                    }
                }

                stats.modifiedCells++;
            }
        }
    }

    data.swap(nextData);
    return stats;
}
