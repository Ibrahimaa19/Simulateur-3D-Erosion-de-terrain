#include "HydraulicErosion.h"

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

void HydraulicErosion::apply(Terrain& terrain)
{
    for (int it = 0; it < iterations; it++)
    {
        int i = rand() % terrain.height;
        int j = rand() % terrain.width;

        float water = rain;
        float sediment = 0.0f;

        for (int step = 0; step < 15; step++)
        {
            float h = terrain.getHeight(i, j);

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

                    float hn = terrain.getHeight(ni, nj);

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
                terrain.setHeight(i, j, sediment * depositRate);
                break;
            }

            float slope = h - lowestH;

            float erodeAmount = erosionRate * slope * water;
 
            terrain.setHeight(i, j, h - erodeAmount);
            sediment += erodeAmount; // la matière transportée devient du sédiment

            i = lowestI;
            j = lowestJ;

            water *= (1.0f - evaporation);
            if (water < 0.01f)
                break;
        }
    }
}
