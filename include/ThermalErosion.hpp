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
    ThermalErosion();
    /**
     * @brief Représente un décalage vers un voisin.
     */
    struct NeighborOffset
    {
        int di;
        int dj;
    };

    /**
     * @brief Charge les informations nécessaires depuis un terrain.
     *
     * @param terrain Pointeur vers le terrain à éroder
     */
    void loadTerrainInfo(std::unique_ptr<Terrain>& terrain) {
        m_data   = terrain->getData();
        m_height = terrain->getTerrainHeight();
        m_width  = terrain->getTerrainWidth();

        mNbPatchX = (m_width + PATCH_SIZE - 1) / PATCH_SIZE;
        mNbPatchZ = (m_height + PATCH_SIZE - 1) / PATCH_SIZE;

        mPatchMarked.resize(mNbPatchX * mNbPatchZ, false);

        resetProgress();
    }

    /**
     * @brief Définit l’angle de talus critique.
     *
     * @param angle Angle en degrés
     */
    void setTalusAngle(float angle) {
        const float PI = 3.14159265f;
        talusAngle = std::tan(angle * PI / 180.0f);
    }

    /**
     * @brief Définit le taux de transfert de matière.
     *
     * @param c Coefficient de transfert
     */
    void setTransferRate(float c) { transferRate = c; }

    /**
     * @brief Active le voisinage à 8 voisins.
     */
    void useEightNeighbors();

    /**
     * @brief Active le voisinage à 4 voisins.
     */
    void useFourNeighbors();

    /**
     * @brief Exécute une itération complète d'érosion thermique.
     *
     * @return Nombre de cellules modifiées durant l'itération
     */
    int step();

    /**
     * @brief Exécute une portion de l'itération d'érosion thermique.
     *
     * @param maxCells Nombre maximum de cellules à traiter pendant cet appel
     * @return Nombre de cellules modifiées pendant ce morceau
     */
    int stepChunk(int maxCells);

    /**
     * @brief Réinitialise l'état interne de progression de l'érosion.
     */
    void resetProgress();

    /**
     * @brief Indique si une itération complète est terminée.
     */
    bool isIterationFinished() const { return mIterationFinished; }

    /**
     * @brief Indique si un rafraîchissement visuel intermédiaire est souhaité.
     */
    bool needsVisualUpdate() const;

    /**
     * @brief Copie le buffer de travail dans les données terrain visibles.
     */
    void commitWorkingData();

    /**
     * @brief Retourne la liste des patches modifiés.
     */
    const std::vector<int>& getDirtyPatchIndices() const { return mDirtyPatchIndices; }

    /**
     * @brief Vide l’ensemble des patches modifiés.
     */
    void clearDirtyPatchIndices()
    {
        for (int idx : mDirtyPatchIndices)
            mPatchMarked[idx] = false;

        mDirtyPatchIndices.clear();
    }

private:
    /** pointeur vers les hauteurs du terrain */
    std::vector<float>* m_data = nullptr;

    /** dimensions du terrain */
    int m_height = 0;
    int m_width = 0;

    /** paramètres d’érosion */
    float talusAngle = 0.f;
    float transferRate = 0.f;

    std::vector<float> m_workingData;
    int mCurrentIndex = 0;
    bool mIterationFinished = false;

    int mCellsProcessedSinceLastCommit = 0;
    int mCommitThreshold = 20000;
    bool mNeedsVisualUpdate = false;

    std::vector<int> mDirtyPatchIndices;
    std::vector<bool> mPatchMarked;

    int mNbPatchX = 0;
    int mNbPatchZ = 0;

    /** voisinage actif */
    const NeighborOffset* mActiveNeighbors = nullptr;
    int mNeighborCount = 0;

    /** voisinages disponibles */
    static const NeighborOffset kNeighbors8[8];
    static const NeighborOffset kNeighbors4[4];

    static constexpr int BLOCK_SIZE = 32;

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