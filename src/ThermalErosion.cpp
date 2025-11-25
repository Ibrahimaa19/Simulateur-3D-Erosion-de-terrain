#include "ThermalErosion.h"
#include <cmath>
#include <iostream>
#include <algorithm>

ThermalErosion::ThermalErosion(float talusAngle, float transferRate)
    : talusAngle(talusAngle), transferRate(transferRate) {}

void ThermalErosion::step(Terrain& terrain)
{
    const int W = terrain.width;
    const int H = terrain.height;
    const float talus = tan(talusAngle);
    const float amount = transferRate;

    for (int i = 1; i < H - 1; i++) {
        for (int j = 1; j < W - 1; j++) {
            float h = terrain.getHeight(i, j);
            
            // Voisins directs
            float diffUp = h - terrain.getHeight(i-1, j);
            float diffDown = h - terrain.getHeight(i+1, j);
            float diffLeft = h - terrain.getHeight(i, j-1);
            float diffRight = h - terrain.getHeight(i, j+1);
            
            float diffs[4] = {diffUp, diffDown, diffLeft, diffRight};
            int neighbors[4][2] = {{-1,0}, {1,0}, {0,-1}, {0,1}};
            
            float totalDiff = 0.0f;
            int validNeighbors = 0;
            
            // Calculer les différences valides
            for (int k = 0; k < 4; k++) {
                if (diffs[k] > talus) {
                    totalDiff += diffs[k];
                    validNeighbors++;
                }
            }
            
            // Appliquer l'érosion si nécessaire
            if (totalDiff > 0 && validNeighbors > 0) {
                float materialToMove = amount * (totalDiff / validNeighbors);
                
                // Éviter de descendre en dessous de 0
                if (materialToMove > h) {
                    materialToMove = h * 0.5f;
                }
                
                terrain.setHeight(i, j, h - materialToMove);
                
                // Répartir le gain de matière aux voisins
                for (int k = 0; k < 4; k++) {
                    if (diffs[k] > talus) {
                        float proportion = diffs[k] / totalDiff;
                        float moveAmount = materialToMove * proportion;
                        
                        int ni = i + neighbors[k][0];
                        int nj = j + neighbors[k][1];
                        
                        float neighborHeight = terrain.getHeight(ni, nj);
                        terrain.setHeight(ni, nj, neighborHeight + moveAmount);
                    }
                }
            }
        }
    }
}