#pragma once

#include "Terrain.hpp"
#include <memory>
#include <cmath>
#include <vector>
#include <iostream>
#include <algorithm>

class ThermalErosion
{
public:
    struct NeighborOffset
    {
        int di;
        int dj;
    };

    ThermalErosion();

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

    void useEightNeighbors();
    void useFourNeighbors();

    int step();
    int stepChunk(int maxCells);

    int stepPureTwoPhase();
    int stepBlockedPureTwoPhase();
    int stepBlockedParallelPureTwoPhase();

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
    static constexpr int BLOCK_SIZE = 32;

    std::vector<float>* m_data = nullptr;

    int m_height = 0;
    int m_width = 0;

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

    const NeighborOffset* mActiveNeighbors = nullptr;
    int mNeighborCount = 0;

    static const NeighborOffset kNeighbors8[8];
    static const NeighborOffset kNeighbors4[4];

private:
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

    int applyErosionRange(const float* src,
                          float* dst,
                          int startIndex,
                          int endIndex);

    int applyBlockedErosionRange(const float* src,
                                 float* dst,
                                 int startIndex,
                                 int endIndex);

    bool erodeCellToDeltaSerial(int i,
                                int j,
                                const float* src,
                                float* delta);

    int applyBlockedParallelErosionToDelta(const float* src,
                                           float* delta);
    int applyBlockedParallelErosionToThreadLocalBuffers(
        const float* src,
        std::vector<std::vector<float>>& threadDeltas,
        std::vector<std::vector<unsigned char>>& threadPatchMarked);
};