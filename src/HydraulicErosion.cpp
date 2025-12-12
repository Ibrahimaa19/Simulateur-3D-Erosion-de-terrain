#include "HydraulicErosion.h"
#include <cstdlib>
#include <cmath>
#include <algorithm>

HydraulicErosion::HydraulicErosion(int iterations,
                                   float rain,
                                   float erosionRate,
                                   float depositRate,
                                   float evaporation)
    : iterations(iterations), rain(rain), 
      erosionRate(erosionRate), depositRate(depositRate), 
      evaporation(evaporation)
{
}

void HydraulicErosion::apply(Terrain& terrain)
{
    const int W = terrain.get_terrain_width();
    const int H = terrain.get_terrain_height();
    
    for (int it = 0; it < iterations; it++)
    {
        // Position aléatoire de départ
        int i = rand() % (H - 2) + 1;
        int j = rand() % (W - 2) + 1;

        float water = rain;
        float sediment = 0.0f;

        for (int step = 0; step < 15; step++)
        {
            float h = terrain.get_height(i, j);

            int lowestI = i;
            int lowestJ = j;
            float lowestH = h;

            // Voisins directs
            int neighbors[4][2] = {{-1,0}, {1,0}, {0,-1}, {0,1}};
            
            for (int k = 0; k < 4; k++) {
                int ni = i + neighbors[k][0];
                int nj = j + neighbors[k][1];
                
                if (ni >= 0 && ni < H && nj >= 0 && nj < W) {
                    float hn = terrain.get_height(ni, nj);
                    if (hn < lowestH) {
                        lowestH = hn;
                        lowestI = ni;
                        lowestJ = nj;
                    }
                }
            }

            // Si pas de pente, déposer et arrêter
            if (lowestI == i && lowestJ == j) {
                terrain.set_height(i, j, h + sediment * depositRate);
                break;
            }

            // Érosion
            float slope = h - lowestH;
            float erodeAmount = erosionRate * slope * water;
            
            // Limiter pour éviter les valeurs négatives
            erodeAmount = std::min(erodeAmount, h * 0.5f);
            
            terrain.set_height(i, j, h - erodeAmount);
            sediment += erodeAmount;

            // Passer à la cellule suivante
            i = lowestI;
            j = lowestJ;

            // Évaporation
            water *= (1.0f - evaporation);
            if (water < 0.01f) break;
        }
        
        // Dépôt final
        if (sediment > 0.0f) {
            float currentHeight = terrain.get_height(i, j);
            terrain.set_height(i, j, currentHeight + sediment * depositRate);
        }
    }
}