#pragma once

#include "FaultFormationTerrain.hpp"
#include "MidpointDisplacement.hpp"
#include "PerlinNoiseTerrain.hpp"
#include "ThermalErosion.hpp"
#include "stb_image_write.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <mpi.h>
#include <numeric>
#include <omp.h>
#include <string>

int neighbors[8][2] = {{1, -1}, {1, 0}, {1, 1}, {0, -1}, {0, 1}, {-1, -1}, {-1, 0}, {-1, 1}};

bool boolNeightbors[8];

void savePngHeightmap(const char* nom_fichier, float* heightmap, int largeur, int hauteur)
{
    unsigned char* pixels = new unsigned char[largeur * hauteur];

    for (int i = 0; i < largeur * hauteur; i++)
    {
        pixels[i] = (unsigned char)(heightmap[i] * 255);
    }

    stbi_write_png(nom_fichier, largeur, hauteur, 1, pixels, largeur);
    delete[] pixels;

    printf("La heightmap a été enregistré avec succès, sous le nom %s \n", nom_fichier);
}

static inline bool isPowerOfTwo(int n)
{
    return n > 0 && (n & (n - 1)) == 0;
}

int stepChunkMPIBlockVect2(float* __restrict__ initialData, float* __restrict__ fluxData,
                           float* __restrict__ bottomFlux, float* __restrict__ topFlux, const int width,
                           const int height) noexcept
{
    const float transferRate = 0.5f;
    const float PI = 3.14159265f;
    const float talusAngle = std::tan(30.f * PI / 180.0f);

    const int W = width;
    const int H = height;

    memset(fluxData, 0, sizeof(float) * (H + 2) * W);
    int changes = 0;

    const int BLOCKSIZE = 32;
    float dist[8];

    for (int ii = 2; ii < H - 1; ii += BLOCKSIZE)
    {
        int i_end = std::min(H - 1, ii + BLOCKSIZE);

        for (int jj = 2; jj < W - 2; jj += BLOCKSIZE)
        {
            int j_end = std::min(W - 2, jj + BLOCKSIZE);

            for (int i = ii; i < i_end; i++)
            {
                const float* __restrict__ rowDown = initialData + (i + 1) * W;
                const float* __restrict__ rowCurr = initialData + i * W;
                const float* __restrict__ rowUp = initialData + (i - 1) * W;

                float* __restrict__ fluxDown = fluxData + (i + 1) * W;
                float* __restrict__ fluxCurr = fluxData + i * W;
                float* __restrict__ fluxUp = fluxData + (i - 1) * W;

#pragma omp simd reduction(+ : changes)
                for (int j = jj; j < j_end; j++)
                {

                    const float currentHeight = initialData[i * W + j];

                    const float d0 = currentHeight - rowDown[j - 1];
                    const float d1 = currentHeight - rowDown[j];
                    const float d2 = currentHeight - rowDown[j + 1];
                    const float d3 = currentHeight - rowCurr[j - 1];
                    const float d4 = currentHeight - rowCurr[j + 1];
                    const float d5 = currentHeight - rowUp[j - 1];
                    const float d6 = currentHeight - rowUp[j];
                    const float d7 = currentHeight - rowUp[j + 1];

                    const float mask0 = (d0 > talusAngle) ? 1.0f : 0.0f;
                    const float mask1 = (d1 > talusAngle) ? 1.0f : 0.0f;
                    const float mask2 = (d2 > talusAngle) ? 1.0f : 0.0f;
                    const float mask3 = (d3 > talusAngle) ? 1.0f : 0.0f;
                    const float mask4 = (d4 > talusAngle) ? 1.0f : 0.0f;
                    const float mask5 = (d5 > talusAngle) ? 1.0f : 0.0f;
                    const float mask6 = (d6 > talusAngle) ? 1.0f : 0.0f;
                    const float mask7 = (d7 > talusAngle) ? 1.0f : 0.0f;

                    const float totalDiff = d0 * mask0 + d1 * mask1 + d2 * mask2 + d3 * mask3 + d4 * mask4 +
                                            d5 * mask5 + d6 * mask6 + d7 * mask7;
                    const float validNeighbors = (mask0 + mask1 + mask2 + mask3 + mask4 + mask5 + mask6 + mask7);

                    const float move0 = d0 * mask0;
                    const float move1 = d1 * mask1;
                    const float move2 = d2 * mask2;
                    const float move3 = d3 * mask3;
                    const float move4 = d4 * mask4;
                    const float move5 = d5 * mask5;
                    const float move6 = d6 * mask6;
                    const float move7 = d7 * mask7;

                    if (totalDiff > 0.f && validNeighbors > 0.f)
                    {

                        float materialToMove = transferRate * (totalDiff / validNeighbors);
                        materialToMove = std::min(materialToMove, currentHeight * transferRate);

                        fluxCurr[j] -= materialToMove;

                        const float material = materialToMove / totalDiff;

                        fluxDown[j - 1] += move0 * material;
                        fluxDown[j] += move1 * material;
                        fluxDown[j + 1] += move2 * material;
                        fluxCurr[j - 1] += move3 * material;
                        fluxCurr[j + 1] += move4 * material;
                        fluxUp[j - 1] += move5 * material;
                        fluxUp[j] += move6 * material;
                        fluxUp[j + 1] += move7 * material;

                        changes++;
                    }
                }
            }
        }
    }

    // -------------------------- BOTTOM
    for (int j = 2; j < W - 2; ++j)
    {
        int i = H - 1;
        float currentHeight = initialData[i * W + j];

        for (int p = 0; p < 8; ++p)
        {
            dist[p] = currentHeight - initialData[(i + neighbors[p][0]) * W + (j + neighbors[p][1])];
        }
        float totalDiff = 0.f;
        int validNeighbors = 0;

        for (int p = 0; p < 8; ++p)
        {
            if (dist[p] > talusAngle)
            {
                totalDiff = totalDiff + dist[p];
                ++validNeighbors;
            }
        }

        if (totalDiff > 0 && validNeighbors > 0)
        {

            float materialToMove = transferRate * (totalDiff / validNeighbors);
            materialToMove = std::min(materialToMove, currentHeight * transferRate);

            fluxData[i * W + j] -= materialToMove;

            // bas cad dans la ghost cell
            for (int k = 0; k < 8; k++)
            {
                float mask = (dist[k] > talusAngle) ? 1.0f : 0.0f;
                float move = materialToMove * (dist[k] / totalDiff) * mask;

                int ni = i + neighbors[k][0];
                int nj = j + neighbors[k][1];

                if (ni == H)
                {
                    if (nj >= 0 && nj < W)
                        bottomFlux[nj] += move;
                }
                else
                    fluxData[ni * W + nj] += move;
            }

            changes++;
        }
    }

    // -------------------------- TOP
    for (int j = 2; j < W - 2; ++j)
    {
        int i = 1;
        float currentHeight = initialData[i * W + j];

        for (int p = 0; p < 8; ++p)
        {
            dist[p] = currentHeight - initialData[(i + neighbors[p][0]) * W + (j + neighbors[p][1])];
        }

        float totalDiff = 0.f;
        int validNeighbors = 0;

        for (int p = 0; p < 8; ++p)
        {
            if (dist[p] > talusAngle)
            {
                totalDiff = totalDiff + dist[p];
                ++validNeighbors;
            }
        }

        if (totalDiff > 0 && validNeighbors > 0)
        {

            float materialToMove = transferRate * (totalDiff / validNeighbors);
            materialToMove = std::min(materialToMove, currentHeight * transferRate);

            fluxData[i * W + j] -= materialToMove;

            // côté et bas
            for (int k = 0; k < 8; k++)
            {

                float mask = (dist[k] > talusAngle) ? 1.0f : 0.0f;
                float move = materialToMove * (dist[k] / totalDiff) * mask;
                int ni = i + neighbors[k][0];
                int nj = j + neighbors[k][1];

                // haut cad dans la ghost cell
                if (ni == 0)
                {
                    if (nj >= 0 && nj < W)
                        topFlux[nj] += move;
                }
                else
                {
                    fluxData[ni * W + nj] += move;
                }
            }

            changes++;
        }
    }

    // -------------------------- RIGHT
    for (int i = 2; i < H - 2; ++i)
    {
        int j = W - 2;

        float currentHeight = initialData[i * W + j];

        for (int p = 0; p < 8; ++p)
        {
            dist[p] = currentHeight - initialData[(i + neighbors[p][0]) * W + (j + neighbors[p][1])];
        }
        float totalDiff = 0.f;
        int validNeighbors = 0;

        for (int p = 0; p < 8; ++p)
        {
            if (dist[p] > talusAngle)
            {
                totalDiff = totalDiff + dist[p];
                ++validNeighbors;
            }
        }

        if (totalDiff > 0 && validNeighbors > 0)
        {

            float materialToMove = transferRate * (totalDiff / validNeighbors);
            materialToMove = std::min(materialToMove, currentHeight * transferRate);

            fluxData[i * W + j] -= materialToMove;

            for (int k = 0; k < 8; k++)
            {

                float mask = (dist[k] > talusAngle) ? 1.0f : 0.0f;
                float move = materialToMove * (dist[k] / totalDiff) * mask;
                int ni = i + neighbors[k][0];
                int nj = j + neighbors[k][1];

                // sortie à droite
                if (nj == W - 1)
                {
                    fluxData[i * W + j] += move;
                }
                else if (ni == 0)
                {
                    topFlux[nj] += move;
                }
                else if (ni == H)
                {
                    bottomFlux[nj] += move;
                }
                else
                {
                    fluxData[ni * W + nj] += move;
                }
            }

            changes++;
        }
    }

    // -------------------------- LEFT
    for (int i = 2; i < H - 2; ++i)
    {
        int j = 1;

        float currentHeight = initialData[i * W + j];

        for (int p = 0; p < 8; ++p)
        {
            dist[p] = currentHeight - initialData[(i + neighbors[p][0]) * W + (j + neighbors[p][1])];
        }

        float totalDiff = 0.f;
        int validNeighbors = 0;

        for (int p = 0; p < 8; ++p)
        {
            if (dist[p] > talusAngle)
            {
                totalDiff = totalDiff + dist[p];
                ++validNeighbors;
            }
        }

        if (totalDiff > 0 && validNeighbors > 0)
        {

            float materialToMove = transferRate * (totalDiff / validNeighbors);
            materialToMove = std::min(materialToMove, currentHeight * transferRate);

            fluxData[i * W + j] -= materialToMove;

            for (int k = 0; k < 8; k++)
            {

                float mask = (dist[k] > talusAngle) ? 1.0f : 0.0f;
                float move = materialToMove * (dist[k] / totalDiff) * mask;
                int ni = i + neighbors[k][0];
                int nj = j + neighbors[k][1];

                if (nj == 0)
                {
                    fluxData[i * W + j] += move; // rebond
                }
                else if (ni == 0)
                {
                    topFlux[nj] += move;
                }
                else if (ni == H)
                {
                    bottomFlux[nj] += move;
                }
                else
                {
                    fluxData[ni * W + nj] += move;
                }
            }

            changes++;
        }
    }

#pragma omp simd
    for (int j = 0; j < W * H; j++)
    {
        initialData[j] += fluxData[j];
    }

    return changes;
}

int stepChunkMPIBlockVect(float* __restrict__ initialData, float* __restrict__ fluxData, float* __restrict__ bottomFlux,
                          float* __restrict__ topFlux, const int width, const int height)
{
    const float transferRate = 0.5f;
    const float PI = 3.14159265f;
    const float talusAngle = std::tan(30.f * PI / 180.0f);

    const int W = width;
    const int H = height;

    memset(fluxData, 0, sizeof(float) * (H + 2) * W);
    int changes = 0;

    const int BLOCKSIZE = 32;
    float dist[8];

    for (int ii = 2; ii < H - 1; ii += BLOCKSIZE)
    {
        int i_end = std::min(H - 1, ii + BLOCKSIZE);

        for (int jj = 2; jj < W - 2; jj += BLOCKSIZE)
        {
            int j_end = std::min(W - 2, jj + BLOCKSIZE);

            for (int i = ii; i < i_end; i++)
            {
#pragma omp simd
                for (int j = jj; j < j_end; j++)
                {

                    float currentHeight = initialData[i * W + j];

#pragma omp simd
                    for (int p = 0; p < 8; ++p)
                    {
                        dist[p] = currentHeight - initialData[(i + neighbors[p][0]) * W + (j + neighbors[p][1])];
                    }

                    float totalDiff = 0.f;
                    int validNeighbors = 0;

#pragma omp simd reduction(+ : totalDiff, validNeighbors)
                    for (int p = 0; p < 8; ++p)
                    {
                        if (dist[p] > talusAngle)
                        {
                            totalDiff = totalDiff + dist[p];
                            ++validNeighbors;
                        }
                    }

                    if (totalDiff > 0 && validNeighbors > 0)
                    {

                        float materialToMove = transferRate * (totalDiff / validNeighbors);
                        materialToMove = std::min(materialToMove, currentHeight * transferRate);

                        fluxData[i * W + j] -= materialToMove;

                        for (int k = 0; k < 8; k++)
                        {
                            float mask = (dist[k] > talusAngle) ? 1.0f : 0.0f;
                            float move = materialToMove * (dist[k] / totalDiff) * mask;

                            int ni = i + neighbors[k][0];
                            int nj = j + neighbors[k][1];

                            fluxData[ni * W + nj] += move;
                        }

                        changes++;
                    }
                }
            }
        }
    }

    // -------------------------- BOTTOM
    for (int j = 2; j < W - 2; ++j)
    {
        int i = H - 1;
        float currentHeight = initialData[i * W + j];

        for (int p = 0; p < 8; ++p)
        {
            dist[p] = currentHeight - initialData[(i + neighbors[p][0]) * W + (j + neighbors[p][1])];
        }
        float totalDiff = 0.f;
        int validNeighbors = 0;

        for (int p = 0; p < 8; ++p)
        {
            if (dist[p] > talusAngle)
            {
                totalDiff = totalDiff + dist[p];
                ++validNeighbors;
            }
        }

        if (totalDiff > 0 && validNeighbors > 0)
        {

            float materialToMove = transferRate * (totalDiff / validNeighbors);
            materialToMove = std::min(materialToMove, currentHeight * transferRate);

            fluxData[i * W + j] -= materialToMove;

            // bas cad dans la ghost cell
            for (int k = 0; k < 8; k++)
            {
                float mask = (dist[k] > talusAngle) ? 1.0f : 0.0f;
                float move = materialToMove * (dist[k] / totalDiff) * mask;

                int ni = i + neighbors[k][0];
                int nj = j + neighbors[k][1];

                if (ni == H)
                {
                    if (nj >= 0 && nj < W)
                        bottomFlux[nj] += move;
                }
                else
                    fluxData[ni * W + nj] += move;
            }

            changes++;
        }
    }
    // --------------------------

    // -------------------------- TOP
    for (int j = 2; j < W - 2; ++j)
    {
        int i = 1;
        float currentHeight = initialData[i * W + j];

        for (int p = 0; p < 8; ++p)
        {
            dist[p] = currentHeight - initialData[(i + neighbors[p][0]) * W + (j + neighbors[p][1])];
        }

        float totalDiff = 0.f;
        int validNeighbors = 0;

        for (int p = 0; p < 8; ++p)
        {
            if (dist[p] > talusAngle)
            {
                totalDiff = totalDiff + dist[p];
                ++validNeighbors;
            }
        }

        if (totalDiff > 0 && validNeighbors > 0)
        {

            float materialToMove = transferRate * (totalDiff / validNeighbors);
            materialToMove = std::min(materialToMove, currentHeight * transferRate);

            fluxData[i * W + j] -= materialToMove;

            // côté et bas
            for (int k = 0; k < 8; k++)
            {

                float mask = (dist[k] > talusAngle) ? 1.0f : 0.0f;
                float move = materialToMove * (dist[k] / totalDiff) * mask;
                int ni = i + neighbors[k][0];
                int nj = j + neighbors[k][1];

                // haut cad dans la ghost cell
                if (ni == 0)
                {
                    if (nj >= 0 && nj < W)
                        topFlux[nj] += move;
                }
                else
                {
                    fluxData[ni * W + nj] += move;
                }
            }

            changes++;
        }
    }
    // --------------------------

    // -------------------------- RIGHT
    for (int i = 2; i < H - 2; ++i)
    {
        int j = W - 2;

        float currentHeight = initialData[i * W + j];

        for (int p = 0; p < 8; ++p)
        {
            dist[p] = currentHeight - initialData[(i + neighbors[p][0]) * W + (j + neighbors[p][1])];
        }
        float totalDiff = 0.f;
        int validNeighbors = 0;

        for (int p = 0; p < 8; ++p)
        {
            if (dist[p] > talusAngle)
            {
                totalDiff = totalDiff + dist[p];
                ++validNeighbors;
            }
        }

        if (totalDiff > 0 && validNeighbors > 0)
        {

            float materialToMove = transferRate * (totalDiff / validNeighbors);
            materialToMove = std::min(materialToMove, currentHeight * transferRate);

            fluxData[i * W + j] -= materialToMove;

            for (int k = 0; k < 8; k++)
            {

                float mask = (dist[k] > talusAngle) ? 1.0f : 0.0f;
                float move = materialToMove * (dist[k] / totalDiff) * mask;
                int ni = i + neighbors[k][0];
                int nj = j + neighbors[k][1];

                // sortie à droite
                if (nj == W - 1)
                {
                    fluxData[i * W + j] += move;
                }
                else if (ni == 0)
                {
                    topFlux[nj] += move;
                }
                else if (ni == H)
                {
                    bottomFlux[nj] += move;
                }
                else
                {
                    fluxData[ni * W + nj] += move;
                }
            }

            changes++;
        }
    }

    // -------------------------- LEFT
    for (int i = 2; i < H - 2; ++i)
    {
        int j = 1;

        float currentHeight = initialData[i * W + j];

        for (int p = 0; p < 8; ++p)
        {
            dist[p] = currentHeight - initialData[(i + neighbors[p][0]) * W + (j + neighbors[p][1])];
        }

        float totalDiff = 0.f;
        int validNeighbors = 0;

        for (int p = 0; p < 8; ++p)
        {
            if (dist[p] > talusAngle)
            {
                totalDiff = totalDiff + dist[p];
                ++validNeighbors;
            }
        }

        if (totalDiff > 0 && validNeighbors > 0)
        {

            float materialToMove = transferRate * (totalDiff / validNeighbors);
            materialToMove = std::min(materialToMove, currentHeight * transferRate);

            fluxData[i * W + j] -= materialToMove;

            for (int k = 0; k < 8; k++)
            {

                float mask = (dist[k] > talusAngle) ? 1.0f : 0.0f;
                float move = materialToMove * (dist[k] / totalDiff) * mask;
                int ni = i + neighbors[k][0];
                int nj = j + neighbors[k][1];

                if (nj == 0)
                {
                    fluxData[i * W + j] += move; // rebond
                }
                else if (ni == 0)
                {
                    topFlux[nj] += move;
                }
                else if (ni == H)
                {
                    bottomFlux[nj] += move;
                }
                else
                {
                    fluxData[ni * W + nj] += move;
                }
            }

            changes++;
        }
    }

#pragma omp simd
    for (int j = 0; j < W * H; j++)
    {
        initialData[j] += fluxData[j];
    }

    return changes;
}

int stepChunkMPIBlock(float* initialData, float* fluxData, float* bottomFlux, float* topFlux, const int width,
                      const int height)
{
    const float transferRate = 0.5f;
    const float PI = 3.14159265f;
    const float talusAngle = std::tan(30.f * PI / 180.0f);

    const int W = width;
    const int H = height;

    memset(fluxData, 0, sizeof(float) * (H + 2) * W);
    int changes = 0;

    const int BLOCKSIZE = 32;

    for (int ii = 1; ii < H; ii += BLOCKSIZE)
    {
        int i_end = std::min(H, ii + BLOCKSIZE);

        for (int jj = 1; jj < W - 1; jj += BLOCKSIZE)
        {
            int j_end = std::min(W - 1, jj + BLOCKSIZE);

            for (int i = ii; i < i_end; i++)
            {
                for (int j = jj; j < j_end; j++)
                {

                    float currentHeight = initialData[i * W + j];

                    float dist[8] = {currentHeight - initialData[(i + 1) * W + (j - 1)],
                                     currentHeight - initialData[(i + 1) * W + j],
                                     currentHeight - initialData[(i + 1) * W + (j + 1)],
                                     currentHeight - initialData[i * W + (j - 1)],
                                     currentHeight - initialData[i * W + (j + 1)],
                                     currentHeight - initialData[(i - 1) * W + (j - 1)],
                                     currentHeight - initialData[(i - 1) * W + j],
                                     currentHeight - initialData[(i - 1) * W + (j + 1)]};

                    int neighbors[8][2] = {{1, -1}, {1, 0}, {1, 1}, {0, -1}, {0, 1}, {-1, -1}, {-1, 0}, {-1, 1}};

                    float totalDiff = 0.f;
                    int validNeighbors = 0;

                    for (int k = 0; k < 8; k++)
                    {
                        if (dist[k] > talusAngle)
                        {
                            totalDiff += dist[k];
                            validNeighbors++;
                        }
                    }

                    if (totalDiff > 0 && validNeighbors > 0)
                    {

                        float materialToMove = transferRate * (totalDiff / validNeighbors);
                        materialToMove = std::min(materialToMove, currentHeight * transferRate);

                        fluxData[i * W + j] -= materialToMove;

                        for (int k = 0; k < 8; k++)
                        {
                            if (dist[k] > talusAngle)
                            {

                                float move = materialToMove * (dist[k] / totalDiff);
                                int ni = i + neighbors[k][0];
                                int nj = j + neighbors[k][1];

                                if (i == H - 1 && ni == H)
                                {
                                    if (nj >= 0 && nj < W)
                                        bottomFlux[nj] += move;
                                }
                                else if (i == 1 && ni == 0)
                                {
                                    if (nj >= 0 && nj < W)
                                        topFlux[nj] += move;
                                }
                                else if (nj <= 0 || nj >= W - 1)
                                {
                                    fluxData[i * W + j] += move;
                                }
                                else
                                {
                                    fluxData[ni * W + nj] += move;
                                }
                            }
                        }

                        changes++;
                    }
                }
            }
        }
    }

    for (int i = 1; i < H; i++)
    {
        for (int j = 0; j < W; j++)
        {
            initialData[i * W + j] += fluxData[i * W + j];
        }
    }

    return changes;
}

#pragma omp declare simd uniform(initialData, fluxData, bottomFlux, topFlux, width, height)
int stepChunkMPIVect(float* initialData, float* fluxData, float* bottomFlux, float* topFlux, const int width,
                     const int height) noexcept
{
    float transferRate = 0.5;

    const float PI = 3.14159265f;
    float talusAngle = std::tan(30.f * PI / 180.0f);

    const int W = width;
    const int H = height;

    if (!initialData)
    {
        std::cerr << "Error: Terrain data not loaded in ThermalErosion.\n";
        return 0;
    }

    for (int i = 0; i < (H + 2) * W; i++)
    {
        fluxData[i] = 0.0f;
    }

    int changes = 0;
    float dist[8];

    // Boucle sur le terrain
    for (int i = 2; i <= H - 1; i++)
    {
#pragma omp simd
        for (int j = 2; j < W - 2; j++)
        {
            float currentHeight = initialData[i * W + j];

#pragma omp simd
            for (int p = 0; p < 8; ++p)
            {
                dist[p] = currentHeight - initialData[(i + neighbors[p][0]) * W + (j + neighbors[p][1])];
            }

            float totalDiff = 0.f;
            int validNeighbors = 0;

#pragma omp simd reduction(+ : totalDiff, validNeighbors)
            for (int p = 0; p < 8; ++p)
            {
                if (dist[p] > talusAngle)
                {
                    totalDiff = totalDiff + dist[p];
                    ++validNeighbors;
                }
            }

            if (totalDiff > 0 && validNeighbors > 0)
            {

                float materialToMove = transferRate * (totalDiff / validNeighbors);
                materialToMove = std::min(materialToMove, currentHeight * transferRate);

                fluxData[i * W + j] -= materialToMove;

                for (int k = 0; k < 8; k++)
                {
                    float mask = (dist[k] > talusAngle) ? 1.0f : 0.0f;
                    float move = materialToMove * (dist[k] / totalDiff) * mask;

                    int ni = i + neighbors[k][0];
                    int nj = j + neighbors[k][1];

                    fluxData[ni * W + nj] += move;
                }
            }
        }
    }

    // -------------------------- BOTTOM
    for (int j = 2; j < W - 2; ++j)
    {
        int i = H - 1;
        float currentHeight = initialData[i * W + j];

        for (int p = 0; p < 8; ++p)
        {
            dist[p] = currentHeight - initialData[(i + neighbors[p][0]) * W + (j + neighbors[p][1])];
        }
        float totalDiff = 0.f;
        int validNeighbors = 0;

        for (int p = 0; p < 8; ++p)
        {
            if (dist[p] > talusAngle)
            {
                totalDiff = totalDiff + dist[p];
                ++validNeighbors;
            }
        }

        if (totalDiff > 0 && validNeighbors > 0)
        {

            float materialToMove = transferRate * (totalDiff / validNeighbors);
            materialToMove = std::min(materialToMove, currentHeight * transferRate);

            fluxData[i * W + j] -= materialToMove;

            // bas cad dans la ghost cell
            for (int k = 0; k < 8; k++)
            {
                float mask = (dist[k] > talusAngle) ? 1.0f : 0.0f;
                float move = materialToMove * (dist[k] / totalDiff) * mask;

                int ni = i + neighbors[k][0];
                int nj = j + neighbors[k][1];

                if (ni == H)
                {
                    if (nj >= 0 && nj < W)
                        bottomFlux[nj] += move;
                }
                else
                    fluxData[ni * W + nj] += move;
            }

            changes++;
        }
    }
    // --------------------------

    // -------------------------- TOP
    for (int j = 2; j < W - 2; ++j)
    {
        int i = 1;
        float currentHeight = initialData[i * W + j];

        for (int p = 0; p < 8; ++p)
        {
            dist[p] = currentHeight - initialData[(i + neighbors[p][0]) * W + (j + neighbors[p][1])];
        }

        float totalDiff = 0.f;
        int validNeighbors = 0;

        for (int p = 0; p < 8; ++p)
        {
            if (dist[p] > talusAngle)
            {
                totalDiff = totalDiff + dist[p];
                ++validNeighbors;
            }
        }

        if (totalDiff > 0 && validNeighbors > 0)
        {

            float materialToMove = transferRate * (totalDiff / validNeighbors);
            materialToMove = std::min(materialToMove, currentHeight * transferRate);

            fluxData[i * W + j] -= materialToMove;

            // côté et bas
            for (int k = 0; k < 8; k++)
            {

                float mask = (dist[k] > talusAngle) ? 1.0f : 0.0f;
                float move = materialToMove * (dist[k] / totalDiff) * mask;
                int ni = i + neighbors[k][0];
                int nj = j + neighbors[k][1];

                // haut cad dans la ghost cell
                if (ni == 0)
                {
                    if (nj >= 0 && nj < W)
                        topFlux[nj] += move;
                }
                else
                {
                    fluxData[ni * W + nj] += move;
                }
            }

            changes++;
        }
    }
    // --------------------------

    // -------------------------- RIGHT
    for (int i = 2; i < H - 2; ++i)
    {
        int j = W - 2;

        float currentHeight = initialData[i * W + j];

        for (int p = 0; p < 8; ++p)
        {
            dist[p] = currentHeight - initialData[(i + neighbors[p][0]) * W + (j + neighbors[p][1])];
        }
        float totalDiff = 0.f;
        int validNeighbors = 0;

        for (int p = 0; p < 8; ++p)
        {
            if (dist[p] > talusAngle)
            {
                totalDiff = totalDiff + dist[p];
                ++validNeighbors;
            }
        }

        if (totalDiff > 0 && validNeighbors > 0)
        {

            float materialToMove = transferRate * (totalDiff / validNeighbors);
            materialToMove = std::min(materialToMove, currentHeight * transferRate);

            fluxData[i * W + j] -= materialToMove;

            for (int k = 0; k < 8; k++)
            {

                float mask = (dist[k] > talusAngle) ? 1.0f : 0.0f;
                float move = materialToMove * (dist[k] / totalDiff) * mask;
                int ni = i + neighbors[k][0];
                int nj = j + neighbors[k][1];

                // sortie à droite
                if (nj == W - 1)
                {
                    fluxData[i * W + j] += move;
                }
                else if (ni == 0)
                {
                    topFlux[nj] += move;
                }
                else if (ni == H)
                {
                    bottomFlux[nj] += move;
                }
                else
                {
                    fluxData[ni * W + nj] += move;
                }
            }

            changes++;
        }
    }

    // -------------------------- LEFT
    for (int i = 2; i < H - 2; ++i)
    {
        int j = 1;

        float currentHeight = initialData[i * W + j];

        for (int p = 0; p < 8; ++p)
        {
            dist[p] = currentHeight - initialData[(i + neighbors[p][0]) * W + (j + neighbors[p][1])];
        }

        float totalDiff = 0.f;
        int validNeighbors = 0;

        for (int p = 0; p < 8; ++p)
        {
            if (dist[p] > talusAngle)
            {
                totalDiff = totalDiff + dist[p];
                ++validNeighbors;
            }
        }

        if (totalDiff > 0 && validNeighbors > 0)
        {

            float materialToMove = transferRate * (totalDiff / validNeighbors);
            materialToMove = std::min(materialToMove, currentHeight * transferRate);

            fluxData[i * W + j] -= materialToMove;

            for (int k = 0; k < 8; k++)
            {

                float mask = (dist[k] > talusAngle) ? 1.0f : 0.0f;
                float move = materialToMove * (dist[k] / totalDiff) * mask;
                int ni = i + neighbors[k][0];
                int nj = j + neighbors[k][1];

                if (nj == 0)
                {
                    fluxData[i * W + j] += move; // rebond
                }
                else if (ni == 0)
                {
                    topFlux[nj] += move;
                }
                else if (ni == H)
                {
                    bottomFlux[nj] += move;
                }
                else
                {
                    fluxData[ni * W + nj] += move;
                }
            }

            changes++;
        }
    }

#pragma omp simd
    for (int i = 1; i <= H * W; i++)
    {
        initialData[i] += fluxData[i];
    }

    return changes;
}

int stepChunkMPI(float* initialData, float* fluxData, float* bottomFlux, float* topFlux, const int width,
                 const int height)
{
    float transferRate = 0.5;

    const float PI = 3.14159265f;
    float talusAngle = std::tan(30.f * PI / 180.0f);

    const int W = width;
    const int H = height;

    if (!initialData)
    {
        std::cerr << "Error: Terrain data not loaded in ThermalErosion.\n";
        return 0;
    }

    memset(fluxData, 0, sizeof(float) * (H + 2) * W);
    int changes = 0;

    // Boucle sur le terrain
    for (int i = 1; i <= H; i++)
    {
        for (int j = 1; j < W - 1; j++)
        {
            float currentHeight = initialData[i * W + j];

            float dist[8] = {
                currentHeight - initialData[(i + 1) * W + (j - 1)], currentHeight - initialData[(i + 1) * W + j],
                currentHeight - initialData[(i + 1) * W + (j + 1)], currentHeight - initialData[i * W + (j - 1)],
                currentHeight - initialData[i * W + (j + 1)],       currentHeight - initialData[(i - 1) * W + (j - 1)],
                currentHeight - initialData[(i - 1) * W + j],       currentHeight - initialData[(i - 1) * W + (j + 1)]};

            int neighbors[8][2] = {{1, -1}, {1, 0}, {1, 1}, {0, -1}, {0, 1}, {-1, -1}, {-1, 0}, {-1, 1}};

            float totalDiff = 0.0f;
            int validNeighbors = 0;

            // Accumulation des différences valides
            for (int k = 0; k < 8; k++)
            {
                if (dist[k] > talusAngle)
                {
                    totalDiff += dist[k];
                    validNeighbors++;
                }
            }

            // Érosion
            if (totalDiff > 0 && validNeighbors > 0)
            {

                float materialToMove = transferRate * (totalDiff / validNeighbors);
                materialToMove = std::min(materialToMove, currentHeight * transferRate);

                // On retire la matière de la cellule actuelle
                fluxData[i * W + j] -= materialToMove;

                // Redistribution aux voisins
                for (int k = 0; k < 8; k++)
                {
                    if (dist[k] > talusAngle)
                    {
                        float move = materialToMove * (dist[k] / totalDiff);

                        int ni = i + neighbors[k][0];
                        int nj = j + neighbors[k][1];

                        if (i == H && ni == H + 1)
                        {
                            if (nj >= 0 && nj < W)
                                bottomFlux[nj] += move;
                        }
                        else if (i == 1 && ni == 0)
                        {
                            if (nj >= 0 && nj < W)
                                topFlux[nj] += move;
                        }
                        else if (nj <= 0 || nj >= W - 1)
                        {
                            fluxData[i * W + j] += move; // reflexion
                        }
                        else
                        {
                            fluxData[ni * W + nj] += move;
                        }
                    }
                }

                changes++;
            }
        }
    }

    for (int i = 1; i <= H; i++)
    {
        for (int j = 0; j < W; j++)
        {
            initialData[i * W + j] += fluxData[i * W + j];
        }
    }

    return changes;
}

void generateTerrain(std::unique_ptr<Terrain>& terrain, int width, int height, std::string terrainType)
{
    if (terrainType == "faultFormation")
    {
        auto generator = std::make_unique<FaultFormationTerrain>();
        generator->CreateFaultFormation(width, height, 1000, 0, 255, 1);
        terrain = std::move(generator);
    }
    else if (terrainType == "midpointDisplacement")
    {
        auto generator = std::make_unique<MidpointDisplacement>();
        generator->CreateMidpointDisplacement(width, 0, 255, 1, 0.5);
        terrain = std::move(generator);
    }
    else
    {
        if (terrainType != "perlinNoise")
            printf("Default terrain : PerlinNoise \n");

        auto generator = std::make_unique<PerlinNoiseTerrain>();
        generator->CreatePerlinNoise(width, height, 0, 255, 1, 0.005);
        terrain = std::move(generator);
    }
}

double checksum(float* tab, int size)
{
    double sum = 0.;
    for (int i = 0; i < size; ++i)
    {
        sum += tab[i];
    }
    return sum;
}

enum COMM
{
    SEND,
    RECV
};

struct Mesh
{
    float* __restrict__ meshData;
    float* __restrict__ meshFluxData;

    float* __restrict__ bottomFlux;
    float* __restrict__ topFlux;
    float* __restrict__ tempFlux;

    int meshWidth;
    int meshHeight;

    int meshSize;
    int meshBufferSize;

    int meshTopId;
    int meshBottomId;

    void initMesh(int width, int height, int topId, int botId)
    {
        meshWidth = width;
        meshHeight = height;

        meshSize = meshHeight * meshWidth;
        meshBufferSize = (meshHeight + 2) * (meshWidth);

        meshData = (float*)aligned_alloc(32, meshBufferSize * sizeof(float));
        meshFluxData = (float*)aligned_alloc(32, meshBufferSize * sizeof(float));

        memset(meshData, 0, meshBufferSize * sizeof(float));
        memset(meshFluxData, 0, meshBufferSize * sizeof(float));

        bottomFlux = (float*)aligned_alloc(32, meshWidth * sizeof(float));
        topFlux = (float*)aligned_alloc(32, meshWidth * sizeof(float));
        tempFlux = (float*)aligned_alloc(32, meshWidth * 2 * sizeof(float));

        memset(bottomFlux, 0, meshWidth * sizeof(float));
        memset(topFlux, 0, meshWidth * sizeof(float));
        memset(tempFlux, 0, meshWidth * 2 * sizeof(float));

        meshTopId = topId;
        meshBottomId = botId;
    }

    ~Mesh()
    {
        free(meshData);
        free(meshFluxData);

        free(bottomFlux);
        free(topFlux);

        free(tempFlux);
    }
};

void initSplitMesh(int rank, int sizeProc, Mesh& mesh, int terrainWidth, int terrainHeight, int* sizes, int* offsets)
{
    int sizeBlock = terrainHeight / sizeProc;
    int sizeBlockRest = terrainHeight % sizeProc;

    int meshHeight = sizeBlock;
    int meshWidth = terrainWidth;

    int meshNbElement = meshHeight * meshWidth;

    if (rank == 0)
    {
        mesh.initMesh(meshWidth, meshHeight, MPI_PROC_NULL, rank + 1);
    }
    else if (rank == sizeProc - 1)
    {
        mesh.initMesh(meshWidth, meshHeight + sizeBlockRest, rank - 1, MPI_PROC_NULL);
    }
    else
    {
        mesh.initMesh(meshWidth, meshHeight, rank - 1, rank + 1);
    }

    for (int i = 0; i < sizeProc; ++i)
    {
        if (i == sizeProc - 1)
            sizes[i] = meshNbElement + sizeBlockRest * meshWidth;
        else
            sizes[i] = meshNbElement;
    }

    for (int i = 0; i < sizeProc; ++i)
        offsets[i] = i * meshNbElement;
}

double testConservation(float* initialData, float* finalData, int size)
{
    if (!initialData || !finalData)
    {
        printf("intialData or finalData is null ! \n");
        return 1;
    }

    double mass_before = checksum(initialData, size);
    double mass_after = checksum(finalData, size);
    // printf("Mass before: %.10f, Mass after: %.10f\n", mass_before, mass_after);
    if (mass_before == 0)
        return 0;
    return std::abs(mass_after - mass_before) / mass_before;
}

void transferFluxTopBot(float* __restrict__ top, float* __restrict__ bot, float* __restrict__ flux, int size)
{
#pragma omp simd
    for (int i = 0; i < size; i++)
    {
        top[i] += flux[i];
        bot[i] += flux[size + i];
    }

    memset(flux, 0, sizeof(float) * size * 2);
}

int lauchMPI(int argc, char* argv[])
{

    int rank, size, nbChanges;

    float* data = nullptr;

    ThermalErosion erosion;
    std::unique_ptr<Terrain> terrain;

    std::string terrainType = argv[2];
    int terrainWidth = atoi(argv[3]);
    int terrainHeight = atoi(argv[4]);
    int terrainStep = atoi(argv[5]);

    int terrainSize = terrainHeight * terrainWidth;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size < 2)
    {
        printf("You must launch this code with atleast 2 processors \n");
        return 1;
    }

    float* initialData = nullptr;

    std::vector<double> elapsedAll(size, 0.0);

    if (terrainType == "midpointDisplacement" && (terrainWidth != terrainHeight || !isPowerOfTwo(terrainWidth - 1)))
    {
        perror("Invalid terrain size : (size must be 2^n + 1)");
        return 1;
    }

    if (rank == 0)
    {

        generateTerrain(terrain, terrainWidth, terrainHeight, terrainType);
        erosion.loadTerrainInfo(*terrain);

        data = terrain->getData()->data();

        initialData = (float*)malloc(sizeof(float) * terrainHeight * terrainWidth);
        memcpy(initialData, data, terrainSize * sizeof(float));
    }

    int* scatterOffset = (int*)malloc(sizeof(int) * size);
    int* scatterSize = (int*)malloc(sizeof(int) * size);

    Mesh myTerrain;
    initSplitMesh(rank, size, myTerrain, terrainWidth, terrainHeight, scatterSize, scatterOffset);

    auto start = MPI_Wtime();

    MPI_Scatterv(data, scatterSize, scatterOffset, MPI_FLOAT, myTerrain.meshData + myTerrain.meshWidth,
                 scatterSize[rank], MPI_FLOAT, 0, MPI_COMM_WORLD);

    if (rank == 0)
    {
        savePngHeightmap("MPI_heightmap_before.png", data, terrainWidth, terrainHeight);
    }

    int ghostStartIndex = myTerrain.meshBufferSize - myTerrain.meshWidth;
    int lastLineIndex = myTerrain.meshBufferSize - (myTerrain.meshWidth * 2);
    int tagCpt = 0;

    for (int i = 1; i <= terrainStep; ++i)
    {

        MPI_Sendrecv(myTerrain.meshData, myTerrain.meshWidth, MPI_FLOAT, myTerrain.meshTopId, tagCpt,

                     myTerrain.meshData + ghostStartIndex, myTerrain.meshWidth, MPI_FLOAT, myTerrain.meshBottomId,
                     tagCpt,

                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        tagCpt++;

        MPI_Sendrecv(myTerrain.meshData + lastLineIndex, myTerrain.meshWidth, MPI_FLOAT, myTerrain.meshBottomId, tagCpt,

                     myTerrain.meshData, myTerrain.meshWidth, MPI_FLOAT, myTerrain.meshTopId, tagCpt,

                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        tagCpt++;

        nbChanges = stepChunkMPIBlockVect2(myTerrain.meshData, myTerrain.meshFluxData, myTerrain.bottomFlux,
                                           myTerrain.topFlux, myTerrain.meshWidth, myTerrain.meshHeight);
        memset(myTerrain.tempFlux, 0, myTerrain.meshWidth * 2 * sizeof(float));

        MPI_Sendrecv(myTerrain.topFlux, myTerrain.meshWidth, MPI_FLOAT, myTerrain.meshTopId, tagCpt,

                     myTerrain.tempFlux, myTerrain.meshWidth, MPI_FLOAT, myTerrain.meshBottomId, tagCpt,

                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        tagCpt++;

        MPI_Sendrecv(myTerrain.bottomFlux, myTerrain.meshWidth, MPI_FLOAT, myTerrain.meshBottomId, tagCpt,

                     myTerrain.tempFlux + myTerrain.meshWidth, myTerrain.meshWidth, MPI_FLOAT, myTerrain.meshTopId,
                     tagCpt,

                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        tagCpt++;

        transferFluxTopBot(myTerrain.meshData + myTerrain.meshWidth, myTerrain.meshData + lastLineIndex,
                           myTerrain.tempFlux, myTerrain.meshWidth);

        if (myTerrain.meshTopId == MPI_PROC_NULL)
        {
            for (int i = 0; i < myTerrain.meshWidth; ++i)
                myTerrain.meshData[myTerrain.meshWidth + i] += myTerrain.topFlux[i];
        }

        if (myTerrain.meshBottomId == MPI_PROC_NULL)
        {
            for (int i = 0; i < myTerrain.meshWidth; ++i)
                myTerrain.meshData[lastLineIndex + i] += myTerrain.bottomFlux[i];
        }

        memset(myTerrain.topFlux, 0, myTerrain.meshWidth * sizeof(float));
        memset(myTerrain.bottomFlux, 0, myTerrain.meshWidth * sizeof(float));
    }

    MPI_Gatherv(myTerrain.meshData + myTerrain.meshWidth, scatterSize[rank], MPI_FLOAT, data, scatterSize,
                scatterOffset, MPI_FLOAT, 0, MPI_COMM_WORLD);

    auto stop = MPI_Wtime();
    double elapsed = stop - start;

    MPI_Gather(&elapsed, 1, MPI_DOUBLE, elapsedAll.data(), 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    if (rank == 0)
    {
        double elapsedMoy = 0.0;
        for (int i = 0; i < size; i++)
        {
            elapsedMoy += elapsedAll[i];
            if (i == size - 1)
                elapsedMoy = elapsedMoy / size;
        }
        printf("-------------- RESULT -------------- \n");
        savePngHeightmap("MPI_heightmap_After.png", data, terrainWidth, terrainHeight);
        printf("Relative error : %f\n", testConservation(initialData, data, terrainSize));
        printf("Temps moyen d'excution de l'érosion : %lf \n", elapsedMoy);
    }

    free(scatterOffset);
    free(scatterSize);
    free(initialData);

    MPI_Finalize();

    return 0;
}

int lauchSequential(int argc, char* argv[])
{
    if (argc < 5)
    {
        printf("Usage: %s <terrainType> <width> <height> <steps>\n", argv[0]);
        printf("terrainType: perlinNoise, faultFormation, midpointDisplacement\n");
        return 1;
    }

    std::string terrainType = argv[2];
    int terrainWidth = atoi(argv[3]);
    int terrainHeight = atoi(argv[4]);
    int terrainStep = atoi(argv[5]);
    int terrainSize = terrainHeight * terrainWidth;

    // Vérification pour midpointDisplacement
    if (terrainType == "midpointDisplacement" && (terrainWidth != terrainHeight || !isPowerOfTwo(terrainWidth - 1)))
    {
        printf("Invalid terrain size : (size must be 2^n + 1)\n");
        return 1;
    }

    // Génération du terrain
    std::unique_ptr<Terrain> terrain;
    generateTerrain(terrain, terrainWidth, terrainHeight, terrainType);

    float* data = terrain->getData()->data();

    // Sauvegarde avant érosion
    savePngHeightmap("heightmap_before.png", data, terrainWidth, terrainHeight);

    // Copie des données initiales pour vérification
    float* initialData = (float*)malloc(sizeof(float) * terrainSize);
    memcpy(initialData, data, terrainSize * sizeof(float));

    // Allocation des buffers (avec lignes fantômes)
    const int W = terrainWidth;
    const int H = terrainHeight;
    const int bufferSize = (H + 2) * W;

    float* fluxData = (float*)calloc(bufferSize, sizeof(float));
    float* bottomFlux = (float*)calloc(W, sizeof(float));
    float* topFlux = (float*)calloc(W, sizeof(float));

    // Timer
    auto start = std::chrono::high_resolution_clock::now();

    // Copie des données dans le buffer avec lignes fantômes
    float* meshData = (float*)calloc(bufferSize, sizeof(float));
    for (int i = 0; i < H; i++)
    {
        memcpy(meshData + (i + 1) * W, data + i * W, W * sizeof(float));
    }
    // Recopie des lignes fantômes (bords)
    memcpy(meshData, meshData + W, W * sizeof(float));                   // haut
    memcpy(meshData + (H + 1) * W, meshData + H * W, W * sizeof(float)); // bas

    int changes = 0;
    int lastLineIndex = H * W; // Dernière ligne réelle (index de début)

    for (int step = 1; step <= terrainStep; step++)
    {
        // Érosion (version non MPI)
        changes = stepChunkMPIBlock(meshData, fluxData, bottomFlux, topFlux, W, H);

        // Appliquer les flux de bordure
        // Transfert topFlux vers la première ligne réelle
        for (int j = 0; j < W; j++)
        {
            meshData[W + j] += topFlux[j];
        }

        // Transfert bottomFlux vers la dernière ligne réelle
        for (int j = 0; j < W; j++)
        {
            meshData[lastLineIndex + j] += bottomFlux[j];
        }

        // Réinitialiser les flux
        memset(fluxData, 0, bufferSize * sizeof(float));
        memset(topFlux, 0, W * sizeof(float));
        memset(bottomFlux, 0, W * sizeof(float));

        // if (step % 100 == 0) {
        //     printf("[%d/%d] changes: %d\n", step, terrainStep, changes);
        // }
    }

    // Appliquer les flux finaux aux données
    for (int i = 0; i < H; i++)
    {
        for (int j = 0; j < W; j++)
        {
            meshData[(i + 1) * W + j] += fluxData[(i + 1) * W + j];
        }
    }

    // Récupérer les données finales
    for (int i = 0; i < H; i++)
    {
        memcpy(data + i * W, meshData + (i + 1) * W, W * sizeof(float));
    }

    auto end = std::chrono::high_resolution_clock::now();
    double elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() / 1000.0;

    // Sauvegarde après érosion
    savePngHeightmap("heightmap_after.png", data, terrainWidth, terrainHeight);

    printf("-------------- RESULT -------------- \n");
    printf("Relative error : %f\n", testConservation(initialData, data, terrainSize));
    printf("Temps d'exécution de l'érosion : %.6f s\n", elapsed);

    // Nettoyage
    free(initialData);
    free(fluxData);
    free(bottomFlux);
    free(topFlux);
    free(meshData);

    return 0;
}
