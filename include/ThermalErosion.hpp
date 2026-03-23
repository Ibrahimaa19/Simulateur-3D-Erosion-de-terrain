#pragma once

#include "Terrain.hpp"
#include <memory>
#include <cmath>
#include <vector>
#include <iostream>

/**
 * @class ThermalErosion
 * @brief Implémente un algorithme d’érosion thermique sur un terrain.
 */
class ThermalErosion
{
public:
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
     * @brief Exécute une itération complète d'érosion thermique.
     *
     * Réinitialise la progression incrémentale et traite l'ensemble
     * des cellules internes du terrain.
     *
     * @return Nombre de cellules modifiées durant l'itération
     */
    int step();

    /**
     * @brief Exécute une portion de l'itération d'érosion thermique.
     *
     * Le traitement est découpé en morceaux pour éviter de bloquer
     * le rendu interactif de l'application.
     *
     * @param maxCells Nombre maximum de cellules à traiter pendant cet appel
     * @return Nombre de cellules modifiées pendant ce morceau
     */
    int stepChunk(int maxCells);

    /**
     * @brief Réinitialise l'état interne de progression de l'érosion.
     *
     * Vide les buffers temporaires, remet à zéro les indices de parcours
     * et réinitialise l'état des patches modifiés.
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

    float get_height(int i, int j) const {
        return (*m_data)[i * m_width + j];
    }

    float get_talus() const {
        return talusAngle;
    }
};