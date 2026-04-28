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

    struct CellStencil
    {
        int center = 0;
        float currentHeight = 0.0f;

        float totalDiff = 0.0f;
        int validNeighbors = 0;

        float diffs[8] = {0.0f};
        int neighborIndices[8] = {0};
        int neighborI[8] = {0};
        int neighborJ[8] = {0};
        int activeSlots[8] = {0};
        int activeCount = 0;
    };

    enum class ChunkVariant
    {
        None = 0,
        PureTwoPhase,
        BlockedPureTwoPhase,
        BlockedParallelPureTwoPhase,
        CheckerboardPureTwoPhase,
        BlockedCheckerboardPureTwoPhase,
        CheckerboardInPlace,
        CheckerboardInPlaceParallel
    };

    struct BlockCoord
    {
        int blockI = 0;
        int blockJ = 0;
    };

    struct ChunkState
    {
        ChunkVariant variant = ChunkVariant::None;
        bool active = false;

        int phase = 0;
        int nextLinearIndex = 0;

        int nextBlockI = 0;
        int nextBlockJ = 0;

        std::vector<float> srcSnapshot;
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
    int stepCheckerboardPureTwoPhase();
    int stepBlockedCheckerboardPureTwoPhase();
    int stepCheckerboardInPlace();
    int stepCheckerboardInPlaceParallel();

    int stepPureTwoPhaseChunk(int budgetCells);
    int stepBlockedPureTwoPhaseChunk(int budgetBlocks);
    int stepBlockedParallelPureTwoPhaseChunk(int budgetBlocks);
    int stepCheckerboardPureTwoPhaseChunk(int budgetCells);
    int stepBlockedCheckerboardPureTwoPhaseChunk(int budgetBlocks);
    int stepCheckerboardInPlaceChunk(int budgetCells);
    int stepCheckerboardInPlaceParallelChunk(int budgetBlocks);

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

    ChunkState mChunkState;

    static const NeighborOffset kNeighbors8[8];
    static const NeighborOffset kNeighbors4[4];

private:
    inline int toIndex(int i, int j) const;
    inline void localIndexToCoords(int localIndex, int& i, int& j) const;

    inline int patchIndexFromCell(int i, int j) const;
    void markPatchDirtyFromCell(int i, int j);

    bool erodeCell(int i, int j, const float* src, float* dst);
    bool erodeCellInPlace(int i, int j, float* data);
    bool erodeCellToDeltaSerial(int i, int j, const float* src, float* delta);

    int applyErosionRange(const float* src,
                          float* dst,
                          int startIndex,
                          int endIndex);

    int applyBlockedErosionRange(const float* src,
                                 float* dst,
                                 int startIndex,
                                 int endIndex);

    int applyCheckerboardErosionRange(const float* src,
                                      float* dst,
                                      int color);

    int applyBlockedCheckerboardErosionRange(const float* src,
                                             float* dst,
                                             int color);

    int applyCheckerboardInPlaceColor(float* data, int color);

    int applyBlockedParallelErosionToDelta(const float* src,
                                           float* delta);

    int applyBlockedParallelErosionToThreadLocalBuffers(
        const float* src,
        std::vector<std::vector<float>>& threadDeltas,
        std::vector<std::vector<unsigned char>>& threadPatchMarked);

    int applyCheckerboardInPlaceColorParallelBuffered(float* data, int color);

    bool buildCellStencil(int i, int j, const float* src, CellStencil& stencil) const;
    float computeMaterialToMove(const CellStencil& stencil) const;

    void applyTransfersToBuffer(const CellStencil& stencil,
                                float materialToMove,
                                float* dst);

    void markDirtyFromStencil(int i, int j, const CellStencil& stencil);

    void markPatchMaskFromStencil(int i,
                                  int j,
                                  const CellStencil& stencil,
                                  std::vector<unsigned char>& patchMask) const;

    void resetChunkState();
    void beginChunkIteration(ChunkVariant variant);
    bool advanceCheckerboardPhase();
    int collectNextBlocks(int budgetBlocks, std::vector<BlockCoord>& blocks);
    bool isBlockTraversalFinished() const;
    void finalizeChunkIteration();

    int applyCheckerboardErosionLinearRange(const float* src,
                                            float* dst,
                                            int color,
                                            int startIndex,
                                            int endIndex);

    int applyCheckerboardInPlaceLinearRange(float* data,
                                            int color,
                                            int startIndex,
                                            int endIndex);

    int applyBlockedPureOnBlocks(const float* src,
                                 float* dst,
                                 const std::vector<BlockCoord>& blocks,
                                 int& processedCells);

    int applyBlockedCheckerboardPureOnBlocks(const float* src,
                                             float* dst,
                                             int color,
                                             const std::vector<BlockCoord>& blocks,
                                             int& processedCells);

    int applyBlockedParallelOnBlocksToThreadLocalBuffers(
        const float* src,
        const std::vector<BlockCoord>& blocks,
        std::vector<std::vector<float>>& threadDeltas,
        std::vector<std::vector<unsigned char>>& threadPatchMarked,
        int& processedCells);

    int applyCheckerboardInPlaceParallelOnBlocksBuffered(
        float* data,
        int color,
        const std::vector<BlockCoord>& blocks,
        int& processedCells);
};