#include <vector>
#include "ThermalErosion.h"

ThermalErosion::ThermalErosion(float talusAngle, float c)
    : talusAngle(talusAngle), c(c) {}


void ThermalErosion::step(Terrain& terrain)
{
    float cellSize = 1.0f;

    // Les 8 voisins
    int di[8] = { -1,-1,-1, 0, 1, 1, 1, 0 };
    int dj[8] = { -1, 0, 1, 1, 1, 0,-1,-1 };

    float dist[8] = {
        1.4142f, 1.0f, 1.4142f,
        1.0f,    1.4142f, 1.0f,
        1.4142f, 1.0f
    };

    std::vector<float> delta(terrain.height * terrain.width, 0.0f);

    for (int i = 0; i < terrain.height; i++)
    {
        for (int j = 0; j < terrain.width; j++)
        {
            float currentHeight = terrain.getHeight(i, j);

            int lowestI = -1; 
            int lowestJ = -1;
            float maxSlope = -1;
            int lowestIndex = -1;

            for (int k = 0; k < 8; k++)
            {
                int ni = i + di[k];
                int nj = j + dj[k];

                if (!terrain.inside(ni, nj))
                    continue;

                float diff = currentHeight - terrain.getHeight(ni, nj);

                if (diff > maxSlope && diff > 0.0f)
                {
                    maxSlope = diff;
                    lowestI = ni;
                    lowestJ = nj;
                    lowestIndex = k;
                }
            }

            if (lowestI >= 0)
            {
                float slopeAngle = maxSlope / (cellSize * dist[lowestIndex]);

                if (slopeAngle > talusAngle)
                {
                    float amount = c * maxSlope;
                    
                    delta[i * terrain.width + j] -= amount;
                    delta[lowestI * terrain.width + lowestJ] += amount;
                }
            }
        }
    }

    // appliquer les changements
    for (int i = 0; i < terrain.width * terrain.height; i++)
        terrain.data[i] += delta[i];
}