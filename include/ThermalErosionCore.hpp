#pragma once

#include <vector>

enum class ThermalNeighborMode
{
    Four = 4,
    Eight = 8
};

struct ThermalErosionStepStats
{
    int modifiedCells = 0;
};

/**
 * @brief Convertit l'angle de talus en degres vers le seuil utilise par
 *        l'algorithme existant.
 */
float thermalTalusThresholdFromDegrees(float angleDegrees);

/**
 * @brief Indique si un voisinage est implemente dans cette branche.
 *
 * La base historique du projet contient uniquement la version 8 voisins.
 * Le mode 4 voisins est reserve pour une implementation ulterieure.
 */
bool isThermalNeighborModeAvailable(ThermalNeighborMode mode);

/**
 * @brief Execute une iteration CPU pure d'erosion thermique.
 *
 * Cette fonction ne depend pas de Terrain, OpenGL, GLFW, GLEW ou ImGui. Elle
 * permet de partager exactement le coeur de calcul entre l'application et les
 * benchmarks CPU.
 */
ThermalErosionStepStats runThermalErosionStep(std::vector<float>& data,
                                              int width,
                                              int height,
                                              float talusThreshold,
                                              float transferRate,
                                              ThermalNeighborMode mode);
