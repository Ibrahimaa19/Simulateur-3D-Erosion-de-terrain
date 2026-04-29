
#include <mpi.h>
#include <omp.h>

#include "stb_image_write.h"
#include <chrono>
#include <fstream>
#include <numeric>
#include <cstring>
#include <algorithm>
#include <cmath>
#include <vector>

#include "FaultFormationTerrain.hpp"
#include "MidpointDisplacement.hpp"
#include "PerlinNoiseTerrain.hpp"
#include "ThermalErosion.hpp"


#define TAG_GHOST_ROW 100
#define TAG_GHOST_COL 101
#define TAG_GHOST_CORNER 102
#define TAG_FLUX_ROW 200
#define TAG_FLUX_COL 201
#define TAG_FLUX_CORNER 202


int neighbors[8][2] = {{1, -1}, {1, 0}, {1, 1}, {0, -1}, {0, 1}, {-1, -1}, {-1, 0}, {-1, 1}};

bool boolNeightbors[8];

void savePngHeightmap(const char *nom_fichier, float *heightmap, int largeur, int hauteur)
{
    unsigned char *pixels = new unsigned char[largeur * hauteur];

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


void generateTerrain(std::unique_ptr<Terrain> &terrain, int width, int height, std::string terrainType)
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


double checksum(float *tab, int size)
{
    double sum = 0.;
    for (int i = 0; i < size; ++i)
    {
        sum += tab[i];
    }
    return sum;
}

/**
 * Calcule l'erreur relative de conservation de masse entre deux tableaux.
 * @param initialData Tableau de référence (état initial).
 * @param finalData Tableau final (après simulation).
 * @param size Taille totale des tableaux (nombre de cellules intérieures).
 * @return Erreur relative (0 = conservation parfaite).
 */
static float testConservation(const float *initialData, const float *finalData, int size)
{
    // 1. Calcul des sommes globales (en ignorant les ghost cells)
    double sumInitial = 0.0;
    double sumFinal = 0.0;

    for (int i = 0; i < size; ++i)
    {
        sumInitial += initialData[i];
        sumFinal += finalData[i];
    }

    // 2. Évite la division par zéro et normalise
    if (sumInitial < 1e-10)
    {
        return 0.0f; // Cas dégénéré : masse nulle
    }

    // 3. Erreur relative (valeur absolue pour éviter les problèmes de signe)
    float relativeError = fabs((sumFinal - sumInitial) / sumInitial);

    return relativeError;
}

// -----------------------------------------------------------
// Rang dans la grille 2D
// -----------------------------------------------------------
static inline int rankOf2D(int row, int col, int P_cols)
{
    return row * P_cols + col;
}

// -----------------------------------------------------------
// ÉTAPE 1 — calcul du flux UNIQUEMENT (aucune modification de data)
// Travaille sur les cellules réelles [1..H][1..W].
// Les ghost cells (ligne 0, ligne H+1, col 0, col W+1)
// doivent déjà contenir les hauteurs des voisins.
// -----------------------------------------------------------
static void stepMpi2D(const float *__restrict__ data, float *__restrict__ flux, int H, int W)
{
    const float transferRate = 0.5f;
    const float PI = 3.14159265f;
    const float talusAngle = std::tan(30.f * PI / 180.f);
    const int stride = W + 2;

    memset(flux, 0, sizeof(float) * (H + 2) * stride);

    const int di[8] = {1, 1, 1, 0, 0, -1, -1, -1};
    const int dj[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
    const int nb_v = 8;

    // const int di[4] = {1, -1, 0, 0};
    // const int dj[4] = {0, 0, -1, 1};
    // const int nb_v = 4;

    for (int i = 1; i <= H; ++i)
    {
        for (int j = 1; j <= W; ++j)
        {
            float cur = data[i * stride + j];
            float diff[nb_v], totalDiff = 0.f, validCount = 0.f;

            for (int k = 0; k < nb_v; ++k)
            {
                diff[k] = cur - data[(i + di[k]) * stride + (j + dj[k])];
                if (diff[k] > talusAngle)
                {
                    totalDiff += diff[k];
                    validCount += 1.f;
                }
            }

            if (totalDiff > 0.f && validCount > 0.f)
            {
                float mat = transferRate * (totalDiff / validCount);
                mat = std::min(mat, cur * transferRate);

                flux[i * stride + j] -= mat;

                for (int k = 0; k < nb_v; ++k)
                {
                    if (diff[k] > talusAngle)
                        flux[(i + di[k]) * stride + (j + dj[k])] += mat * (diff[k] / totalDiff);
                }
            }
        }
    }
}

static void stepMpi2DBlocked(const float *__restrict__ data, float *__restrict__ flux, int H, int W)
{
    const float transferRate = 0.5f;
    const float PI = 3.14159265f;
    const float talusAngle = std::tan(30.f * PI / 180.f);
    const int stride = W + 2;

    memset(flux, 0, sizeof(float) * (H + 2) * stride);

    const int di[8] = {1, 1, 1, 0, 0, -1, -1, -1};
    const int dj[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
    const int nb_v = 8;

    // const int di[4] = {1, -1, 0, 0};
    // const int dj[4] = {0, 0, -1, 1};
    // const int nb_v = 4;

    // Paramètres de blocage - à ajuster selon votre cache L1/L2
    const int BLOCK_SIZE_I = 64;  // Hauteur du bloc (lignes)
    const int BLOCK_SIZE_J = 64;  // Largeur du bloc (colonnes)

    // Parcours par blocs
    for (int ii = 1; ii <= H; ii += BLOCK_SIZE_I)
    {
        int i_max = std::min(ii + BLOCK_SIZE_I - 1, H);
        
        for (int jj = 1; jj <= W; jj += BLOCK_SIZE_J)
        {
            int j_max = std::min(jj + BLOCK_SIZE_J - 1, W);
            
            // Traitement du bloc [ii..i_max] x [jj..j_max]
            for (int i = ii; i <= i_max; ++i)
            {
                for (int j = jj; j <= j_max; ++j)
                {
                    float cur = data[i * stride + j];
                    float diff[nb_v], totalDiff = 0.f, validCount = 0.f;
                    
                    for (int k = 0; k < nb_v; ++k)
                    {
                        diff[k] = cur - data[(i + di[k]) * stride + (j + dj[k])];
                        if (diff[k] > talusAngle)
                        {
                            totalDiff += diff[k];
                            validCount += 1.f;
                        }
                    }
                    
                    if (totalDiff > 0.f && validCount > 0.f)
                    {
                        float mat = transferRate * (totalDiff / validCount);
                        mat = std::min(mat, cur * transferRate);
                        
                        flux[i * stride + j] -= mat;
                        
                        for (int k = 0; k < nb_v; ++k)
                        {
                            if (diff[k] > talusAngle)
                                flux[(i + di[k]) * stride + (j + dj[k])] += mat * (diff[k] / totalDiff);
                        }
                    }
                }
            }
        }
    }
}

static void stepMpi2DCheckboard(const float *__restrict__ data, float *__restrict__ flux, int H, int W)
{
    const float transferRate = 0.5f;
    const float PI = 3.14159265f;
    const float talusAngle = std::tan(30.f * PI / 180.f);
    const int stride = W + 2;

    memset(flux, 0, sizeof(float) * (H + 2) * stride);

    const int di[4] = {1, -1, 0, 0};
    const int dj[4] = {0, 0, -1, 1};

    for(int color = 0 ; color <2 ; ++color){
        for (int i = 1; i <= H; ++i)
        {
            for (int j = 1; j <= W; ++j)
            {
                if (((i + j) & 1) != color)
                {
                    continue;
                }

                float cur = data[i * stride + j];
                float diff[8], totalDiff = 0.f, validCount = 0.f;

                for (int k = 0; k < 4; ++k)
                {
                    diff[k] = cur - data[(i + di[k]) * stride + (j + dj[k])];
                    if (diff[k] > talusAngle)
                    {
                        totalDiff += diff[k];
                        validCount += 1.f;
                    }
                }

                if (totalDiff > 0.f && validCount > 0.f)
                {
                    float mat = transferRate * (totalDiff / validCount);
                    mat = std::min(mat, cur * transferRate);

                    flux[i * stride + j] -= mat;

                    for (int k = 0; k < 4; ++k)
                    {
                        if (diff[k] > talusAngle)
                            flux[(i + di[k]) * stride + (j + dj[k])] += mat * (diff[k] / totalDiff);
                    }
                }
            }
        }
    }

}

static void stepMpi2DCheckboardBlocked(const float *__restrict__ data, float *__restrict__ flux, int H, int W)
{
    const float transferRate = 0.5f;
    const float PI = 3.14159265f;
    const float talusAngle = std::tan(30.f * PI / 180.f);
    const int stride = W + 2;

    memset(flux, 0, sizeof(float) * (H + 2) * stride);

    const int di[4] = {1, -1, 0, 0};
    const int dj[4] = {0, 0, -1, 1};

    const int BLOCK_SIZE_I = 64;
    const int BLOCK_SIZE_J = 64;

    for (int color=0;color<2;++color){
        for (int ii = 1; ii <= H; ii += BLOCK_SIZE_I)
        {
            int i_max = std::min(ii + BLOCK_SIZE_I - 1, H);
            
            for (int jj = 1; jj <= W; jj += BLOCK_SIZE_J)
            {
                int j_max = std::min(jj + BLOCK_SIZE_J - 1, W);
                
                // Traitement des cellules de la couleur actuelle dans le bloc
                for (int i = ii; i <= i_max; ++i)
                {
                    // Calcul du premier j qui correspond à la couleur
                    int start_j = jj;
                    if ((i + start_j) % 2 != color)
                        start_j++;
                    
                    for (int j = start_j; j <= j_max; j += 2)
                    {
                        float cur = data[i * stride + j];
                        float diff[4], totalDiff = 0.f, validCount = 0.f;
                        
                        for (int k = 0; k < 4; ++k)
                        {
                            diff[k] = cur - data[(i + di[k]) * stride + (j + dj[k])];
                            if (diff[k] > talusAngle)
                            {
                                totalDiff += diff[k];
                                validCount += 1.f;
                            }
                        }
                        
                        if (totalDiff > 0.f && validCount > 0.f)
                        {
                            float mat = transferRate * (totalDiff / validCount);
                            mat = std::min(mat, cur * transferRate);
                            
                            flux[i * stride + j] -= mat;
                            
                            for (int k = 0; k < 4; ++k)
                            {
                                if (diff[k] > talusAngle)
                                    flux[(i + di[k]) * stride + (j + dj[k])] += mat * (diff[k] / totalDiff);
                            }
                        }
                    }
                }
            }
        }
    }

}

static void stepMpi2DCheckboardInplace(const float *__restrict__ data, float *__restrict__ flux, int H, int W)
{
    const float transferRate = 0.5f;
    const float PI = 3.14159265f;
    const float talusAngle = std::tan(30.f * PI / 180.f);
    const int stride = W + 2;

    memset(flux, 0, sizeof(float) * (H + 2) * stride);

    const int di[4] = {1, -1, 0, 0};
    const int dj[4] = {0, 0, -1, 1};


    for (int color=0 ;color <2;++color){
        for (int i = 1; i <= H; ++i)
        {
            for (int j = 1; j <= W; ++j)
            {
                if (((i + j) & 1) != color)
                {
                    continue;
                }

                float cur = data[i * stride + j] + flux[i * stride + j];
                float diff[8], totalDiff = 0.f, validCount = 0.f;

                for (int k = 0; k < 4; ++k)
                {
                    diff[k] = cur - (data[(i + di[k]) * stride + (j + dj[k])] + flux[(i + di[k]) * stride + (j + dj[k])]);
                    if (diff[k] > talusAngle)
                    {
                        totalDiff += diff[k];
                        validCount += 1.f;
                    }
                }

                if (totalDiff > 0.f && validCount > 0.f)
                {
                    float mat = transferRate * (totalDiff / validCount);
                    mat = std::min(mat, cur * transferRate);

                    flux[i * stride + j] -= mat;

                    for (int k = 0; k < 4; ++k)
                    {
                        if (diff[k] > talusAngle)
                            flux[(i + di[k]) * stride + (j + dj[k])] += mat * (diff[k] / totalDiff);
                    }
                }
            }
        }
    }

}

static void stepMpi2DCheckboardInplaceBlocked(const float *__restrict__ data, float *__restrict__ flux, int H, int W)
{
    const float transferRate = 0.5f;
    const float PI = 3.14159265f;
    const float talusAngle = std::tan(30.f * PI / 180.f);
    const int stride = W + 2;

    memset(flux, 0, sizeof(float) * (H + 2) * stride);

    const int di[4] = {1, -1, 0, 0};
    const int dj[4] = {0, 0, -1, 1};

    const int BLOCK_SIZE_J = 64;  // Bloquer uniquement en largeur

    // Pour chaque couleur (0 puis 1)
    for (int color = 0; color < 2; ++color)
    {
        // Parcours dans l'ORDRE NORMAL (ligne par ligne)
        for (int i = 1; i <= H; ++i)
        {
            // Mais découpage en blocs horizontaux pour améliorer la localité
            for (int jj = 1; jj <= W; jj += BLOCK_SIZE_J)
            {
                int j_max = std::min(jj + BLOCK_SIZE_J - 1, W);
                
                // Calcul du premier j qui correspond à la couleur
                int start_j = jj;
                if (((i + jj) & 1) != color) {
                    start_j = jj + 1;
                }
                
                for (int j = start_j; j <= j_max; j += 2)
                {
                    float cur = data[i * stride + j] + flux[i * stride + j];
                    float diff[4], totalDiff = 0.f, validCount = 0.f;
                    
                    for (int k = 0; k < 4; ++k)
                    {
                        diff[k] = cur - (data[(i + di[k]) * stride + (j + dj[k])] + 
                                        flux[(i + di[k]) * stride + (j + dj[k])]);
                        if (diff[k] > talusAngle)
                        {
                            totalDiff += diff[k];
                            validCount += 1.f;
                        }
                    }
                    
                    if (totalDiff > 0.f && validCount > 0.f)
                    {
                        float mat = transferRate * (totalDiff / validCount);
                        mat = std::min(mat, cur * transferRate);
                        
                        flux[i * stride + j] -= mat;
                        
                        for (int k = 0; k < 4; ++k)
                        {
                            if (diff[k] > talusAngle)
                                flux[(i + di[k]) * stride + (j + dj[k])] += mat * (diff[k] / totalDiff);
                        }
                    }
                }
            }
        }
    }

}


// -----------------------------------------------------------
// ÉTAPE 2 — appliquer le flux sur les cellules réelles
// -----------------------------------------------------------
static void applyFlux2D(float *__restrict__ data, const float *__restrict__ flux, int H, int W)
{
    const int stride = W + 2;
    for (int i = 1; i <= H; ++i)
        for (int j = 1; j <= W; ++j)
            data[i * stride + j] += flux[i * stride + j];
}

// -----------------------------------------------------------
// ÉTAPE 3 — échange des hauteurs (ghost cells)
// Envoie la première/dernière ligne et colonne réelle,
// reçoit dans la ghost cell opposée.
// -----------------------------------------------------------
static void exchangeGhosts2D(float *data, int H, int W, int top, int bot, int left, int right, int topLeft,
                             int topRight, int botLeft, int botRight)
{
    const int stride = W + 2;

    // ---- lignes ----
    if (top != -1)
        MPI_Sendrecv(&data[1 * stride], stride, MPI_FLOAT, top, TAG_GHOST_ROW, &data[0], stride, MPI_FLOAT, top,
                     TAG_GHOST_ROW, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    if (bot != -1)
        MPI_Sendrecv(&data[H * stride], stride, MPI_FLOAT, bot, TAG_GHOST_ROW, &data[(H + 1) * stride], stride,
                     MPI_FLOAT, bot, TAG_GHOST_ROW, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    // ---- colonnes ----
    std::vector<float> sendCol(H + 2), recvCol(H + 2);

    if (left != -1)
    {
        for (int i = 0; i <= H + 1; ++i)
            sendCol[i] = data[i * stride + 1];
        MPI_Sendrecv(sendCol.data(), H + 2, MPI_FLOAT, left, TAG_GHOST_COL, recvCol.data(), H + 2, MPI_FLOAT, left,
                     TAG_GHOST_COL, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        for (int i = 0; i <= H + 1; ++i)
            data[i * stride + 0] = recvCol[i];
    }

    if (right != -1)
    {
        for (int i = 0; i <= H + 1; ++i)
            sendCol[i] = data[i * stride + W];
        MPI_Sendrecv(sendCol.data(), H + 2, MPI_FLOAT, right, TAG_GHOST_COL, recvCol.data(), H + 2, MPI_FLOAT, right,
                     TAG_GHOST_COL, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        for (int i = 0; i <= H + 1; ++i)
            data[i * stride + W + 1] = recvCol[i];
    }

    // ---- coins diagonaux ----
    // Chaque coin est un scalaire unique : on envoie notre cellule réelle
    // de coin et on reçoit dans notre ghost cell de coin opposé.
    float send1, recv1;

    // coin haut-gauche  [0][0]  ← voisin topLeft envoie son [H][W]
    if (topLeft != -1)
    {
        send1 = data[1 * stride + 1];
        MPI_Sendrecv(&send1, 1, MPI_FLOAT, topLeft, TAG_GHOST_CORNER, &recv1, 1, MPI_FLOAT, topLeft, TAG_GHOST_CORNER,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        data[0 * stride + 0] = recv1;
    }

    // coin haut-droit  [0][W+1]  ← voisin topRight envoie son [H][1]
    if (topRight != -1)
    {
        send1 = data[1 * stride + W];
        MPI_Sendrecv(&send1, 1, MPI_FLOAT, topRight, TAG_GHOST_CORNER, &recv1, 1, MPI_FLOAT, topRight, TAG_GHOST_CORNER,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        data[0 * stride + W + 1] = recv1;
    }

    // coin bas-gauche  [H+1][0]  ← voisin botLeft envoie son [1][W]
    if (botLeft != -1)
    {
        send1 = data[H * stride + 1];
        MPI_Sendrecv(&send1, 1, MPI_FLOAT, botLeft, TAG_GHOST_CORNER, &recv1, 1, MPI_FLOAT, botLeft, TAG_GHOST_CORNER,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        data[(H + 1) * stride + 0] = recv1;
    }

    // coin bas-droit  [H+1][W+1]  ← voisin botRight envoie son [1][1]
    if (botRight != -1)
    {
        send1 = data[H * stride + W];
        MPI_Sendrecv(&send1, 1, MPI_FLOAT, botRight, TAG_GHOST_CORNER, &recv1, 1, MPI_FLOAT, botRight, TAG_GHOST_CORNER,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        data[(H + 1) * stride + W + 1] = recv1;
    }
}

// -----------------------------------------------------------
// ÉTAPE 4 — échange des flux de bord
//
// Après stepMpi2D, certains flux ont atterri dans les
// ghost cells (ex: un pixel en i=1 envoie du flux à i=0).
// Ces valeurs appartiennent aux voisins : on les leur envoie
// et on additionne ce qu'on reçoit à nos propres bordures.
// Ensuite on remet les ghost cells de flux à 0.
// -----------------------------------------------------------
static void exchangeFluxBorders2D(float *flux, int H, int W, int top, int bot, int left, int right, int topLeft,
                                  int topRight, int botLeft, int botRight)
{
    const int stride = W + 2;

    // ---- ghost row top ----
    if (top != -1)
    {
        // Envoyer notre ghost row 0 au voisin, recevoir le sien
        std::vector<float> sendBuf(stride), recvBuf(stride, 0.f);
        memcpy(sendBuf.data(), &flux[0], stride * sizeof(float));
        MPI_Sendrecv(sendBuf.data(), stride, MPI_FLOAT, top, TAG_FLUX_ROW, recvBuf.data(), stride, MPI_FLOAT, top,
                     TAG_FLUX_ROW, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        for (int j = 0; j < stride; ++j)
            flux[1 * stride + j] += recvBuf[j];
    }
    else
    {
        // Bord du domaine : réflexion — le flux retourne à la ligne réelle 1
        for (int j = 0; j < stride; ++j)
            flux[1 * stride + j] += flux[0 * stride + j];
    }
    memset(&flux[0], 0, stride * sizeof(float));

    // ---- ghost row bottom ----
    if (bot != -1)
    {
        std::vector<float> sendBuf(stride), recvBuf(stride, 0.f);
        memcpy(sendBuf.data(), &flux[(H + 1) * stride], stride * sizeof(float));
        MPI_Sendrecv(sendBuf.data(), stride, MPI_FLOAT, bot, TAG_FLUX_ROW, recvBuf.data(), stride, MPI_FLOAT, bot,
                     TAG_FLUX_ROW, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        for (int j = 0; j < stride; ++j)
            flux[H * stride + j] += recvBuf[j];
    }
    else
    {
        // Bord du domaine : réflexion
        for (int j = 0; j < stride; ++j)
            flux[H * stride + j] += flux[(H + 1) * stride + j];
    }
    memset(&flux[(H + 1) * stride], 0, stride * sizeof(float));

    // ---- ghost col left ----
    std::vector<float> sendCol(H + 2), recvCol(H + 2, 0.f);

    if (left != -1)
    {
        for (int i = 0; i <= H + 1; ++i)
            sendCol[i] = flux[i * stride + 0];
        MPI_Sendrecv(sendCol.data(), H + 2, MPI_FLOAT, left, TAG_FLUX_COL, recvCol.data(), H + 2, MPI_FLOAT, left,
                     TAG_FLUX_COL, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        for (int i = 0; i <= H + 1; ++i)
            flux[i * stride + 1] += recvCol[i];
    }
    else
    {
        // Bord du domaine : réflexion
        for (int i = 0; i <= H + 1; ++i)
            flux[i * stride + 1] += flux[i * stride + 0];
    }
    for (int i = 0; i <= H + 1; ++i)
        flux[i * stride + 0] = 0.f;

    // ---- ghost col right ----
    if (right != -1)
    {
        for (int i = 0; i <= H + 1; ++i)
            sendCol[i] = flux[i * stride + W + 1];
        MPI_Sendrecv(sendCol.data(), H + 2, MPI_FLOAT, right, TAG_FLUX_COL, recvCol.data(), H + 2, MPI_FLOAT, right,
                     TAG_FLUX_COL, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        for (int i = 0; i <= H + 1; ++i)
            flux[i * stride + W] += recvCol[i];
    }
    else
    {
        // Bord du domaine : réflexion
        for (int i = 0; i <= H + 1; ++i)
            flux[i * stride + W] += flux[i * stride + W + 1];
    }
    for (int i = 0; i <= H + 1; ++i)
        flux[i * stride + W + 1] = 0.f;

    // ---- coins diagonaux ----
    float send1, recv1;

    // coin haut-gauche [0][0]
    if (topLeft != -1)
    {
        send1 = flux[0 * stride + 0];
        MPI_Sendrecv(&send1, 1, MPI_FLOAT, topLeft, TAG_FLUX_CORNER, &recv1, 1, MPI_FLOAT, topLeft, TAG_FLUX_CORNER,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        flux[1 * stride + 1] += recv1;
    }
    else
    {
        // Bord du domaine : réflexion vers la cellule réelle de coin
        flux[1 * stride + 1] += flux[0 * stride + 0];
    }
    flux[0 * stride + 0] = 0.f;

    // coin haut-droit [0][W+1]
    if (topRight != -1)
    {
        send1 = flux[0 * stride + W + 1];
        MPI_Sendrecv(&send1, 1, MPI_FLOAT, topRight, TAG_FLUX_CORNER, &recv1, 1, MPI_FLOAT, topRight, TAG_FLUX_CORNER,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        flux[1 * stride + W] += recv1;
    }
    else
    {
        flux[1 * stride + W] += flux[0 * stride + W + 1];
    }
    flux[0 * stride + W + 1] = 0.f;

    // coin bas-gauche [H+1][0]
    if (botLeft != -1)
    {
        send1 = flux[(H + 1) * stride + 0];
        MPI_Sendrecv(&send1, 1, MPI_FLOAT, botLeft, TAG_FLUX_CORNER, &recv1, 1, MPI_FLOAT, botLeft, TAG_FLUX_CORNER,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        flux[H * stride + 1] += recv1;
    }
    else
    {
        flux[H * stride + 1] += flux[(H + 1) * stride + 0];
    }
    flux[(H + 1) * stride + 0] = 0.f;

    // coin bas-droit [H+1][W+1]
    if (botRight != -1)
    {
        send1 = flux[(H + 1) * stride + W + 1];
        MPI_Sendrecv(&send1, 1, MPI_FLOAT, botRight, TAG_FLUX_CORNER, &recv1, 1, MPI_FLOAT, botRight, TAG_FLUX_CORNER,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        flux[H * stride + W] += recv1;
    }
    else
    {
        flux[H * stride + W] += flux[(H + 1) * stride + W + 1];
    }
    flux[(H + 1) * stride + W + 1] = 0.f;
}

// -----------------------------------------------------------
// Scatter : rang 0 distribue les blocs
// -----------------------------------------------------------
static void scatter2D(float *globalData, float *localData, int GH, int GW, int baseBlockH, int baseBlockW, int P_rows,
                      int P_cols, int myBlockH, int myBlockW, int rank)
{
    const int localStride = myBlockW + 2;
    const int globalStride = GW;

    if (rank == 0)
    {
        for (int pr = 0; pr < P_rows; ++pr)
        {
            int bH = (pr == P_rows - 1) ? (GH - pr * baseBlockH) : baseBlockH;
            for (int pc = 0; pc < P_cols; ++pc)
            {
                int bW = (pc == P_cols - 1) ? (GW - pc * baseBlockW) : baseBlockW;
                int dest = rankOf2D(pr, pc, P_cols);

                std::vector<float> buf(bH * bW);
                for (int i = 0; i < bH; ++i)
                    memcpy(&buf[i * bW], &globalData[(pr * baseBlockH + i) * globalStride + pc * baseBlockW],
                           bW * sizeof(float));

                if (dest == 0)
                {
                    // Copie directe dans le buffer local du rang 0
                    for (int i = 0; i < bH; ++i)
                        memcpy(&localData[(i + 1) * localStride + 1], &buf[i * bW], bW * sizeof(float));
                }
                else
                {
                    int dims[2] = {bH, bW};
                    MPI_Send(dims, 2, MPI_INT, dest, 0, MPI_COMM_WORLD);
                    MPI_Send(buf.data(), bH * bW, MPI_FLOAT, dest, 1, MPI_COMM_WORLD);
                }
            }
        }
    }
    else
    {
        int dims[2];
        MPI_Recv(dims, 2, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        int bH = dims[0], bW = dims[1];
        std::vector<float> buf(bH * bW);
        MPI_Recv(buf.data(), bH * bW, MPI_FLOAT, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        for (int i = 0; i < bH; ++i)
            memcpy(&localData[(i + 1) * localStride + 1], &buf[i * bW], bW * sizeof(float));
    }
}

// -----------------------------------------------------------
// Gather : chaque rang renvoie son bloc au rang 0
// -----------------------------------------------------------
static void gather2D(float *globalData, float *localData, int GH, int GW, int baseBlockH, int baseBlockW, int P_rows,
                     int P_cols, int myBlockH, int myBlockW, int rank)
{
    const int localStride = myBlockW + 2;
    const int globalStride = GW;

    if (rank != 0)
    {
        std::vector<float> buf(myBlockH * myBlockW);
        for (int i = 0; i < myBlockH; ++i)
            memcpy(&buf[i * myBlockW], &localData[(i + 1) * localStride + 1], myBlockW * sizeof(float));
        MPI_Send(buf.data(), myBlockH * myBlockW, MPI_FLOAT, 0, 2, MPI_COMM_WORLD);
    }
    else
    {
        for (int pr = 0; pr < P_rows; ++pr)
        {
            int bH = (pr == P_rows - 1) ? (GH - pr * baseBlockH) : baseBlockH;
            for (int pc = 0; pc < P_cols; ++pc)
            {
                int bW = (pc == P_cols - 1) ? (GW - pc * baseBlockW) : baseBlockW;
                int src = rankOf2D(pr, pc, P_cols);

                if (src == 0)
                {
                    // Rang 0 se copie lui-même
                    const int ls = baseBlockW + 2;
                    for (int i = 0; i < bH; ++i)
                        memcpy(&globalData[(pr * baseBlockH + i) * globalStride + pc * baseBlockW],
                               &localData[(i + 1) * ls + 1], bW * sizeof(float));
                }
                else
                {
                    std::vector<float> buf(bH * bW);
                    MPI_Recv(buf.data(), bH * bW, MPI_FLOAT, src, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    for (int i = 0; i < bH; ++i)
                        memcpy(&globalData[(pr * baseBlockH + i) * globalStride + pc * baseBlockW], &buf[i * bW],
                               bW * sizeof(float));
                }
            }
        }
    }
}

// -----------------------------------------------------------
//  launchMPI2D
//  mpirun -np <N> ./prog mpi2d <type> <W> <H> <steps> <P_rows> <P_cols>
// -----------------------------------------------------------
int launchMPI2D(int argc, char *argv[])
{
    if (argc < 8)
    {
        if (argc > 1)
            fprintf(stderr, "Usage: %s mpi2d <terrainType> <W> <H> <steps> <P_rows> <P_cols>\n", argv[0]);
        return 1;
    }

    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    std::string terrainType = argv[2];
    int GW = atoi(argv[3]);
    int GH = atoi(argv[4]);
    int steps = atoi(argv[5]);
    int P_rows = atoi(argv[6]);
    int P_cols = atoi(argv[7]);

    if (P_rows * P_cols != size)
    {
        if (rank == 0)
            fprintf(stderr, "Erreur : P_rows(%d) * P_cols(%d) != nb_procs(%d)\n", P_rows, P_cols, size);
        MPI_Finalize();
        return 1;
    }

    int myRow = rank / P_cols;
    int myCol = rank % P_cols;

    int baseBlockH = GH / P_rows;
    int baseBlockW = GW / P_cols;

    int myBlockH = (myRow == P_rows - 1) ? (GH - myRow * baseBlockH) : baseBlockH;
    int myBlockW = (myCol == P_cols - 1) ? (GW - myCol * baseBlockW) : baseBlockW;

    int topNeigh = (myRow > 0) ? rankOf2D(myRow - 1, myCol, P_cols) : -1;
    int botNeigh = (myRow < P_rows - 1) ? rankOf2D(myRow + 1, myCol, P_cols) : -1;
    int leftNeigh = (myCol > 0) ? rankOf2D(myRow, myCol - 1, P_cols) : -1;
    int rightNeigh = (myCol < P_cols - 1) ? rankOf2D(myRow, myCol + 1, P_cols) : -1;

    // Voisins diagonaux (-1 si bord du domaine)
    int topLeftNeigh = (myRow > 0 && myCol > 0) ? rankOf2D(myRow - 1, myCol - 1, P_cols) : -1;
    int topRightNeigh = (myRow > 0 && myCol < P_cols - 1) ? rankOf2D(myRow - 1, myCol + 1, P_cols) : -1;
    int botLeftNeigh = (myRow < P_rows - 1 && myCol > 0) ? rankOf2D(myRow + 1, myCol - 1, P_cols) : -1;
    int botRightNeigh = (myRow < P_rows - 1 && myCol < P_cols - 1) ? rankOf2D(myRow + 1, myCol + 1, P_cols) : -1;

    int localStride = myBlockW + 2;
    int localSize = (myBlockH + 2) * localStride;

    float *localData = (float *)calloc(localSize, sizeof(float));
    float *localFlux = (float *)calloc(localSize, sizeof(float));

    float *globalData = nullptr;
    float *globalInitial = nullptr;

    if (rank == 0)
    {
        std::unique_ptr<Terrain> terrain;
        generateTerrain(terrain, GW, GH, terrainType);
        globalData = (float *)malloc(GH * GW * sizeof(float));
        memcpy(globalData, terrain->getData()->data(), GH * GW * sizeof(float));

        globalInitial = (float *)malloc(GH * GW * sizeof(float));
        memcpy(globalInitial, globalData, GH * GW * sizeof(float));

        savePngHeightmap("MPI2D_before.png", globalData, GW, GH);
    }

    double t0 = MPI_Wtime();

    scatter2D(globalData, localData, GH, GW, baseBlockH, baseBlockW, P_rows, P_cols, myBlockH, myBlockW, rank);

    for (int step = 0; step < steps; ++step)
    {
        // 1. Ghost cells de hauteur (arêtes + coins)
        exchangeGhosts2D(localData, myBlockH, myBlockW, topNeigh, botNeigh, leftNeigh, rightNeigh, topLeftNeigh,
                         topRightNeigh, botLeftNeigh, botRightNeigh);

        // 2. Calcul du flux (sans modifier data)
        stepMpi2DCheckboardInplaceBlocked(localData, localFlux, myBlockH, myBlockW);

        // 3. Échange des flux qui ont débordé dans les ghost cells (arêtes + coins)
        exchangeFluxBorders2D(localFlux, myBlockH, myBlockW, topNeigh, botNeigh, leftNeigh, rightNeigh, topLeftNeigh,
                              topRightNeigh, botLeftNeigh, botRightNeigh);

        // 4. Application du flux
        applyFlux2D(localData, localFlux, myBlockH, myBlockW);
    }

    gather2D(globalData, localData, GH, GW, baseBlockH, baseBlockW, P_rows, P_cols, myBlockH, myBlockW, rank);

    double elapsed = MPI_Wtime() - t0;

    if (rank == 0)
    {
        savePngHeightmap("MPI2D_after.png", globalData, GW, GH);
        printf("-------------- RESULT (MPI 2D %dx%d) --------------\n", P_rows, P_cols);
        printf("Relative error : %f\n", testConservation(globalInitial, globalData, GH * GW));
        printf("Elapsed : %.6f s\n", elapsed);

        free(globalData);
        free(globalInitial);
    }

    free(localData);
    free(localFlux);

    MPI_Finalize();
    return 0;
}