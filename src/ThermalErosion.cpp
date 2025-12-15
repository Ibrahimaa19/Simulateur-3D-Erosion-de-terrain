#include "ThermalErosion.hpp"
#include <iostream>

void ThermalErosion::step()
{
    const int W = m_width;
    const int H = m_height;

    if (!m_data) {
        std::cerr << "Error: Terrain data not loaded in ThermalErosion.\n";
        return;
    }

    std::vector<float>& data = *m_data;
    std::vector<float> newData = data; // Copie
        
    int changes = 0;

    // Boucle sur le terrain
    for (int i = 1; i < H - 1; i++) {
        for (int j = 1; j < W - 1; j++) {

            float currentHeight = data[i * W + j];

            // Hauteurs des 8 voisins (Moore neighborhood)
            float diffUp         = currentHeight - data[(i - 1) * W + j];
            float diffDown       = currentHeight - data[(i + 1) * W + j];
            float diffLeft       = currentHeight - data[i * W + (j - 1)];
            float diffRight      = currentHeight - data[i * W + (j + 1)];
            float diffUpLeft     = currentHeight - data[(i - 1) * W + (j - 1)];
            float diffUpRight    = currentHeight - data[(i - 1) * W + (j + 1)];
            float diffDownLeft   = currentHeight - data[(i + 1) * W + (j - 1)];
            float diffDownRight  = currentHeight - data[(i + 1) * W + (j + 1)];

            // Stockage des différences et indices des voisins
            float dist[8] = { diffUp, diffDown, diffLeft, diffRight,
                              diffUpLeft, diffUpRight, diffDownLeft, diffDownRight };
            
            int neighbors[8][2] = { 
                {-1, 0}, {1, 0}, {0, -1}, {0, 1},     // 4 voisins directs
                {-1, -1}, {-1, 1}, {1, -1}, {1, 1}    // 4 voisins diagonaux
            };

            float totalDiff = 0.0f;
            int validNeighbors = 0;

            // Accumulation des différences valides
            for (int k = 0; k < 8; k++) {
                if (dist[k] > talusAngle) {
                    totalDiff += dist[k];
                    validNeighbors++;
                }
            }

            // Érosion
            if (totalDiff > 0 && validNeighbors > 0) {
                
                float materialToMove = transferRate * (totalDiff / validNeighbors);
                materialToMove = std::min(materialToMove, currentHeight * transferRate);

                // On retire la matière de la cellule actuelle
                newData[i * W + j] -= materialToMove;

                // Redistribution aux voisins
                for (int k = 0; k < 8; k++) {
                    if (dist[k] > talusAngle) {
                        float proportion = dist[k] / totalDiff;
                        float moveAmount = materialToMove * proportion;

                        int ni = i + neighbors[k][0];
                        int nj = j + neighbors[k][1];

                        newData[ni * W + nj] += moveAmount;
                    }
                }

                changes++;
            }
        }
    }

    data = newData;

    //std::cout << "Cells modified: " << changes << std::endl;
}