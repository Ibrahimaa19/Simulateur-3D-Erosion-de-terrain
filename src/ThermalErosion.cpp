#include "ThermalErosion.hpp"
#include <cmath>
#include <iostream>
#include <algorithm>

ThermalErosion::ThermalErosion(float talusAngle, float transferRate)
    : talusAngle(talusAngle), transferRate(transferRate) {}

void ThermalErosion::step(Terrain& terrain)
{
    const int W = terrain.get_terrain_width();
    const int H = terrain.get_terrain_height();
    const float talus = tan(talusAngle);

    int changes = 0;
    
    for (int i = 1; i < H - 1; i++) {
        for (int j = 1; j < W - 1; j++) {

            float currentHeight = terrain.get_height(i, j);
            
            // Voisins directs
            float diffUp = currentHeight - terrain.get_height(i-1, j);
            float diffDown = currentHeight - terrain.get_height(i+1, j);
            float diffLeft = currentHeight - terrain.get_height(i, j-1);
            float diffRight = currentHeight - terrain.get_height(i, j+1);
            
            float dist[4] = {diffUp, diffDown, diffLeft, diffRight};
            int neighbors[4][2] = {{-1,0}, {1,0}, {0,-1}, {0,1}};
            
            float totalDiff = 0.0f;
            int validNeighbors = 0;
            
            // Calculer les différences valides
            for (int k = 0; k < 4; k++) {
                if (dist[k] > talus) {
                    totalDiff += dist[k];
                    validNeighbors++;
                }
            }
            
            // Appliquer l'érosion si nécessaire
            if (totalDiff > 0 && validNeighbors > 0) {
                float materialToMove = transferRate * (totalDiff / validNeighbors);
                
                materialToMove = std::min(materialToMove, currentHeight*transferRate);

                terrain.set_height(i, j, currentHeight - materialToMove);
                
                // Répartir le gain de matière aux voisins
                for (int k = 0; k < 4; k++) {
                    if (dist[k] > talus) {
                        float proportion = dist[k] / totalDiff;
                        float moveAmount = materialToMove * proportion;
                        
                        int ni = i + neighbors[k][0];
                        int nj = j + neighbors[k][1];
                        
                        float neighborHeight = terrain.get_height(ni, nj);
                        terrain.set_height(ni, nj, neighborHeight + moveAmount);
                    }
                }
            }

            if (totalDiff > 0) {
                changes++;
            }
        }
    }
    std::cout << "Cells modified: " << changes << std::endl;
}