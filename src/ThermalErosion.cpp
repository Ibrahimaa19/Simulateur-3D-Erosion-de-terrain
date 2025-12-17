#include "ThermalErosion.hpp"
#include <iostream>

int ThermalErosion::step()
{
    const int W = m_width;
    const int H = m_height;

    if (!m_data) {
        std::cerr << "Error: Terrain data not loaded in ThermalErosion.\n";
        return 0;
    }

    std::vector<float> data = *m_data;// Copie
        
    int changes = 0;

    // Boucle sur le terrain
    for (int i = 1; i < H - 1; i++) {
        for (int j = 1; j < W - 1; j++) {

            float currentHeight = (*m_data)[i * W + j];

            // Hauteurs des 8 voisins (Moore neighborhood)
            float diffUp         = currentHeight - (*m_data)[(i - 1) * W + j];
            float diffDown       = currentHeight - (*m_data)[(i + 1) * W + j];
            float diffLeft       = currentHeight - (*m_data)[i * W + (j - 1)];
            float diffRight      = currentHeight - (*m_data)[i * W + (j + 1)];
            float diffUpLeft     = currentHeight - (*m_data)[(i - 1) * W + (j - 1)];
            float diffUpRight    = currentHeight - (*m_data)[(i - 1) * W + (j + 1)];
            float diffDownLeft   = currentHeight - (*m_data)[(i + 1) * W + (j - 1)];
            float diffDownRight  = currentHeight - (*m_data)[(i + 1) * W + (j + 1)];

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
                data[i * W + j] -= materialToMove;

                // Redistribution aux voisins
                for (int k = 0; k < 8; k++) {
                    if (dist[k] > talusAngle) {
                        float proportion = dist[k] / totalDiff;
                        float moveAmount = materialToMove * proportion;

                        int ni = i + neighbors[k][0];
                        int nj = j + neighbors[k][1];

                        data[ni * W + nj] += moveAmount;
                    }
                }

                changes++;
            }
        }
    }

    *m_data = data;

    //std::cout << "Cells modified: " << changes << std::endl;
    return changes;
}