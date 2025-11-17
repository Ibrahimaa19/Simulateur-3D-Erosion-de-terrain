#include "HydraulicErosion.h"
#include <cstdlib>

HydraulicErosion::HydraulicErosion(int iterations,
                                   float rain,
                                   float erosionRate,
                                   float depositRate,
                                   float evaporation)
    : iterations(iterations),
      rain(rain),
      erosionRate(erosionRate),
      depositRate(depositRate),
      evaporation(evaporation)
{
}

void HydraulicErosion::apply(HeightField& terrain)
{
    int n = terrain.size;

    for (int it = 0; it < iterations; it++)
    {
        int i = rand() % n;
        int j = rand() % n;

        float water = rain;
        float sediment = 0.0f;

        for (int step = 0; step < 15; step++)
        {
            float h = terrain.GetHeight(i, j);

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

                    float hn = terrain.GetHeight(ni, nj);

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
                terrain.SetHeight(i, j, sediment * depositRate);
                break;
            }

            float slope = h - lowestH;

            float erodeAmount = erosionRate * slope * water;

            sediment += erodeAmount;
            terrain.SetHeight(i, j, -erodeAmount);

            i = lowestI;
            j = lowestJ;

            water *= (1.0f - evaporation);
            if (water < 0.01f)
                break;
        }
    }
}
