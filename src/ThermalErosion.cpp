#include "ThermalErosion.hpp"
#include <iostream>
#include <algorithm>

// Indices des directions (Moore 8-voisins)
// k=0: up (-1,0), 1: down (1,0), 2: left (0,-1), 3: right (0,1)
// k=4: up-left, 5: up-right, 6: down-left, 7: down-right
static const int DI[8] = {-1, 1, 0, 0, -1, -1, 1, 1};
static const int DJ[8] = {0, 0, -1, 1, -1, 1, -1, 1};
// Direction opposée : voisin en k envoie vers nous via OPPOSITE[k]
static const int OPPOSITE[8] = {1, 0, 3, 2, 7, 6, 5, 4};

void ThermalErosion::ensureOutflowBuffer()
{
    const size_t size = static_cast<size_t>(8) * m_width * m_height;
    if (m_outflow.size() != size) {
        m_outflow.resize(size, 0.0f);
    }
}

int ThermalErosion::step()
{
    const int W = m_width;
    const int H = m_height;

    if (!m_data) {
        std::cerr << "Error: Terrain data not loaded in ThermalErosion.\n";
        return 0;
    }

    ensureOutflowBuffer();
    std::fill(m_outflow.begin(), m_outflow.end(), 0.0f);

    int changes = 0;

    // Phase 1 : calcul des flux sortants (chaque cellule écrit uniquement dans son propre bloc)
    for (int i = 1; i < H - 1; i++) {
        for (int j = 1; j < W - 1; j++) {
            const int idx = i * W + j;
            float currentHeight = (*m_data)[idx];

            float dist[8];
            float totalDiff = 0.0f;
            int validNeighbors = 0;

            for (int k = 0; k < 8; k++) {
                int ni = i + DI[k], nj = j + DJ[k];
                dist[k] = currentHeight - (*m_data)[ni * W + nj];
                if (dist[k] > talusAngle) {
                    totalDiff += dist[k];
                    validNeighbors++;
                }
            }

            if (totalDiff > 0.0f && validNeighbors > 0) {
                float materialToMove = transferRate * (totalDiff / validNeighbors);
                materialToMove = std::min(materialToMove, currentHeight * transferRate);

                for (int k = 0; k < 8; k++) {
                    if (dist[k] > talusAngle) {
                        float proportion = dist[k] / totalDiff;
                        m_outflow[idx * 8 + k] = materialToMove * proportion;
                    }
                }
                changes++;
            }
        }
    }

    // Phase 2 : application des deltas (chaque cellule lit les flux voisins, écrit sa propre hauteur)
    for (int i = 1; i < H - 1; i++) {
        for (int j = 1; j < W - 1; j++) {
            const int idx = i * W + j;

            float totalOut = 0.0f;
            for (int k = 0; k < 8; k++) {
                totalOut += m_outflow[idx * 8 + k];
            }

            float totalIn = 0.0f;
            for (int k = 0; k < 8; k++) {
                int ni = i + DI[k], nj = j + DJ[k];
                int nidx = ni * W + nj;
                int ok = OPPOSITE[k];
                totalIn += m_outflow[nidx * 8 + ok];
            }

            (*m_data)[idx] += totalIn - totalOut;
        }
    }

    return changes;
}
