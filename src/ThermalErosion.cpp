#include "ThermalErosion.hpp"
#include <cmath>
#include <algorithm>
#include <iostream>

void ThermalErosion::step()
{
    const int W = m_width;
    const int H = m_height;

    float talus = 0.f;

    if (!m_data) {
        std::cerr << "Error: Terrain data not loaded in ThermalErosion.\n";
        return;
    }

    std::vector<float>& data = *m_data;
    std::vector<float> newData = data; // Copie pour éviter les effets immédiats

    int changes = 0;

    // Boucle sur le terrain
    for (int i = 1; i < H - 1; i++) {
        for (int j = 1; j < W - 1; j++) {

            float currentHeight = data[i * W + j];

            // Hauteurs des voisins
            float diffUp    = currentHeight - data[(i - 1) * W + j];
            float diffDown  = currentHeight - data[(i + 1) * W + j];
            float diffLeft  = currentHeight - data[i * W + (j - 1)];
            float diffRight = currentHeight - data[i * W + (j + 1)];

            //std::cout << diffUp <<" "<<  diffDown <<" "<< diffLeft <<" "<< diffRight <<" "<< std::endl;

            float dist[4] = { diffUp, diffDown, diffLeft, diffRight };
            int neighbors[4][2] = { {-1,0}, {1,0}, {0,-1}, {0,1} };

            float totalDiff = 0.0f;
            int validNeighbors = 0;

            // Accumulation des différences valides
            for (int k = 0; k < 4; k++) {
                if (dist[k] > talus) {
                    totalDiff += dist[k];
                    validNeighbors++;
                }
            }
            
            //if(totalDiff != 0){
                //std::cout << totalDiff << std::endl;
            //}

            // Érosion
            if (totalDiff > 0 && validNeighbors > 0) {

                float materialToMove =
                    transferRate * (totalDiff / validNeighbors);

                // Sécurité : ne retire pas plus qu’une fraction de la hauteur actuelle
                materialToMove = std::min(materialToMove, currentHeight * transferRate);

                // On retire la matière de la cellule courante
                newData[i * W + j] -= materialToMove;

                // Redistribution aux voisins
                for (int k = 0; k < 4; k++) {
                    if (dist[k] > talus) {
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

    // Mise à jour globale
    data = newData;

    std::cout << "Cells modified: " << changes << std::endl;
}
