#include <iostream>
#include "ThermalErosion.h"

ThermalErosion::ThermalErosion(float talusAngle, float c)
    : talusAngle(talusAngle), c(c) {}


void ThermalErosion::step(HeightField& terrain)
{
    int n = terrain.size;
    float cellSize = terrain.cellSpacing;

    // Les 8 voisins
    int di[8] = { -1,-1,-1, 0, 1, 1, 1, 0 };
    int dj[8] = { -1, 0, 1, 1, 1, 0,-1,-1 };

    float dist[8] = {
        1.4142f, 1.0f, 1.4142f,
        1.0f,    1.4142f, 1.0f,
        1.4142f, 1.0f
    };

    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < n; j++)
        {
            float currentHeight = terrain.GetHeight(i, j);

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

                float diff = currentHeight - terrain.GetHeight(ni, nj);

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
                    terrain.SetHeight(i, j, -c * maxSlope);
                    terrain.SetHeight(lowestI, lowestJ, c * maxSlope);
                }
            }
        }
    }
}
