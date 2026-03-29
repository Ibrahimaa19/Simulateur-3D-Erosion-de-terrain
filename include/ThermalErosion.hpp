#pragma once

#include "Terrain.hpp"
#include <memory>
#include <cmath>
#include <vector>
#include <iostream>
#include <algorithm>

/**
 * @class ThermalErosion
 * @brief Implémente un algorithme d’érosion thermique sur un terrain.
 */
class ThermalErosion
{
public:
    void loadTerrainInfo(std::unique_ptr<Terrain>& terrain) {
        m_data   = terrain->getData();
        m_height = terrain->getTerrainHeight();
        m_width  = terrain->getTerrainWidth();

        mNbPatchX = (m_width + PATCH_SIZE - 1) / PATCH_SIZE;
        mNbPatchZ = (m_height + PATCH_SIZE - 1) / PATCH_SIZE;

        mPatchMarked.resize(mNbPatchX * mNbPatchZ, false);

        resetProgress();
    }

    void setTalusAngle(float angle) {
        const float PI = 3.14159265f;
        talusAngle = std::tan(angle * PI / 180.0f);
    }

    void setTransferRate(float c) { transferRate = c; }

    int step();
    int stepChunk(int maxCells);

    void resetProgress();

    bool isIterationFinished() const { return mIterationFinished; }
    bool needsVisualUpdate() const;

    void commitWorkingData();

    const std::vector<int>& getDirtyPatchIndices() const { return mDirtyPatchIndices; }

    void clearDirtyPatchIndices()
    {
        for (int idx : mDirtyPatchIndices)
            mPatchMarked[idx] = false;

        mDirtyPatchIndices.clear();
    }

private:
    std::vector<float>* m_data = nullptr;

    int m_height = 0;
    int m_width = 0;

    float talusAngle = 0.f;
    float transferRate = 0.f;

    std::vector<float> m_workingData; /**< Buffer temporaire utilisé pendant le traitement incrémental */
    int mCurrentIndex = 0;            /**< Indice courant dans la grille interne */
    bool mIterationFinished = false;  /**< Indique si l'itération complète est terminée */

    int mCellsProcessedSinceLastCommit = 0; /**< Nombre de cellules traitées depuis le dernier commit visuel */
    int mCommitThreshold = 20000;           /**< Seuil de cellules avant mise à jour visuelle intermédiaire */
    bool mNeedsVisualUpdate = false;        /**< Indique si une mise à jour visuelle intermédiaire est requise */

    std::vector<int> mDirtyPatchIndices; /**< Liste des patches modifiés pendant l'érosion */
    std::vector<bool> mPatchMarked;      /**< Marqueurs évitant les doublons dans la liste des patches sales */

    int mNbPatchX = 0; /**< Nombre de patches en X */
    int mNbPatchZ = 0; /**< Nombre de patches en Z */

private:
    float get_height(int i, int j) const {
        return (*m_data)[i * m_width + j];
    }

    float get_talus() const {
        return talusAngle;
    }

    inline int toIndex(int i, int j) const;
    inline void localIndexToCoords(int localIndex, int& i, int& j) const;

    inline int patchIndexFromCell(int i, int j) const;
    void markPatchDirtyFromCell(int i, int j);

    void addMaterialToNeighbor(float* dst,
                               int neighborIndex,
                               float moveAmount,
                               int neighborI,
                               int neighborJ);

    bool erodeCell(int i, int j, const float* src, float* dst);
};