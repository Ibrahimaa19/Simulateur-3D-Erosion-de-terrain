#include "ThermalErosion.hpp"
#include <omp.h>

const ThermalErosion::NeighborOffset ThermalErosion::kNeighbors8[8] = {
    {-1,  0},
    { 1,  0},
    { 0, -1},
    { 0,  1},
    {-1, -1},
    {-1,  1},
    { 1, -1},
    { 1,  1}
};

const ThermalErosion::NeighborOffset ThermalErosion::kNeighbors4[4] = {
    {-1,  0},
    { 1,  0},
    { 0, -1},
    { 0,  1}
};

ThermalErosion::ThermalErosion()
{
    useEightNeighbors();
}

void ThermalErosion::useEightNeighbors()
{
    mActiveNeighbors = kNeighbors8;
    mNeighborCount = 8;
}

void ThermalErosion::useFourNeighbors()
{
    mActiveNeighbors = kNeighbors4;
    mNeighborCount = 4;
}

int ThermalErosion::toIndex(int i, int j) const
{
    return i * m_width + j;
}

void ThermalErosion::localIndexToCoords(int localIndex, int& i, int& j) const
{
    i = 1 + localIndex / (m_width - 2);
    j = 1 + localIndex % (m_width - 2);
}

int ThermalErosion::patchIndexFromCell(int i, int j) const
{
    const int patchX = j / PATCH_SIZE;
    const int patchZ = i / PATCH_SIZE;
    return patchX * mNbPatchZ + patchZ;
}

void ThermalErosion::markPatchDirtyFromCell(int i, int j)
{
    const int patchIndex = patchIndexFromCell(i, j);

    if (!mPatchMarked[patchIndex]) {
        mPatchMarked[patchIndex] = true;
        mDirtyPatchIndices.push_back(patchIndex);
    }
}

void ThermalErosion::addMaterialToNeighbor(float* dst,
                                           int neighborIndex,
                                           float moveAmount,
                                           int neighborI,
                                           int neighborJ)
{
    dst[neighborIndex] += moveAmount;
    markPatchDirtyFromCell(neighborI, neighborJ);
}

bool ThermalErosion::erodeCell(int i, int j, const float* src, float* dst)
{
    const int center = toIndex(i, j);
    const float currentHeight = src[center];

    float totalDiff = 0.0f;
    int validNeighbors = 0;

    float diffs[8] = {0.0f};
    int neighborIndices[8] = {0};
    int neighborI[8] = {0};
    int neighborJ[8] = {0};

    for (int k = 0; k < mNeighborCount; ++k)
    {
        const int ni = i + mActiveNeighbors[k].di;
        const int nj = j + mActiveNeighbors[k].dj;
        const int nIndex = toIndex(ni, nj);

        const float diff = currentHeight - src[nIndex];

        diffs[k] = diff;
        neighborIndices[k] = nIndex;
        neighborI[k] = ni;
        neighborJ[k] = nj;

        if (diff > talusAngle) {
            totalDiff += diff;
            ++validNeighbors;
        }
    }

    if (totalDiff <= 0.0f || validNeighbors <= 0) {
        return false;
    }

    markPatchDirtyFromCell(i, j);

    float materialToMove = transferRate * (totalDiff / validNeighbors);
    materialToMove = std::min(materialToMove, currentHeight * transferRate);

    dst[center] -= materialToMove;

    const float invTotalDiff = 1.0f / totalDiff;

    for (int k = 0; k < mNeighborCount; ++k)
    {
        if (diffs[k] > talusAngle) {
            const float moveAmount = materialToMove * (diffs[k] * invTotalDiff);
            addMaterialToNeighbor(dst,
                                  neighborIndices[k],
                                  moveAmount,
                                  neighborI[k],
                                  neighborJ[k]);
        }
    }

    return true;
}
bool ThermalErosion::erodeCellToDeltaSerial(int i,
                                            int j,
                                            const float* src,
                                            float* delta)
{
    const int center = toIndex(i, j);
    const float currentHeight = src[center];

    float totalDiff = 0.0f;
    int validNeighbors = 0;

    float diffs[8] = {0.0f};
    int neighborIndices[8] = {0};
    int neighborI[8] = {0};
    int neighborJ[8] = {0};

    for (int k = 0; k < mNeighborCount; ++k)
    {
        const int ni = i + mActiveNeighbors[k].di;
        const int nj = j + mActiveNeighbors[k].dj;
        const int nIndex = toIndex(ni, nj);

        const float diff = currentHeight - src[nIndex];

        diffs[k] = diff;
        neighborIndices[k] = nIndex;
        neighborI[k] = ni;
        neighborJ[k] = nj;

        if (diff > talusAngle) {
            totalDiff += diff;
            ++validNeighbors;
        }
    }

    if (totalDiff <= 0.0f || validNeighbors <= 0) {
        return false;
    }

    float materialToMove = transferRate * (totalDiff / validNeighbors);
    materialToMove = std::min(materialToMove, currentHeight * transferRate);

    delta[center] -= materialToMove;

    const float invTotalDiff = 1.0f / totalDiff;

    for (int k = 0; k < mNeighborCount; ++k)
    {
        if (diffs[k] > talusAngle) {
            const float moveAmount = materialToMove * (diffs[k] * invTotalDiff);
            delta[neighborIndices[k]] += moveAmount;
        }
    }

    markPatchDirtyFromCell(i, j);
    for (int k = 0; k < mNeighborCount; ++k)
    {
        if (diffs[k] > talusAngle) {
            markPatchDirtyFromCell(neighborI[k], neighborJ[k]);
        }
    }

    return true;
}

int ThermalErosion::applyErosionRange(const float* src,
                                      float* dst,
                                      int startIndex,
                                      int endIndex)
{
    int changes = 0;

    for (int localIndex = startIndex; localIndex < endIndex; ++localIndex)
    {
        int i, j;
        localIndexToCoords(localIndex, i, j);

        if (erodeCell(i, j, src, dst)) {
            ++changes;
        }
    }

    return changes;
}

int ThermalErosion::applyBlockedErosionRange(const float* src,
                                             float* dst,
                                             int startIndex,
                                             int endIndex)
{
    const int W = m_width;
    const int H = m_height;
    const int innerWidth = W - 2;
    const int innerHeight = H - 2;

    int changes = 0;

    for (int blockI = 0; blockI < innerHeight; blockI += BLOCK_SIZE)
    {
        const int blockHeight = std::min(BLOCK_SIZE, innerHeight - blockI);

        for (int blockJ = 0; blockJ < innerWidth; blockJ += BLOCK_SIZE)
        {
            const int blockWidth = std::min(BLOCK_SIZE, innerWidth - blockJ);

            for (int di = 0; di < blockHeight; ++di)
            {
                const int innerI = blockI + di;

                for (int dj = 0; dj < blockWidth; ++dj)
                {
                    const int innerJ = blockJ + dj;
                    const int localIndex = innerI * innerWidth + innerJ;

                    if (localIndex < startIndex || localIndex >= endIndex) {
                        continue;
                    }

                    const int i = innerI + 1;
                    const int j = innerJ + 1;

                    if (erodeCell(i, j, src, dst)) {
                        ++changes;
                    }
                }
            }
        }
    }

    return changes;
}
int ThermalErosion::applyBlockedParallelErosionToDelta(const float* src,
                                                       float* delta)
{
    const int W = m_width;
    const int H = m_height;
    const int innerWidth = W - 2;
    const int innerHeight = H - 2;

    int changes = 0;

    #pragma omp parallel for collapse(2) schedule(static) reduction(+:changes)
    for (int blockI = 0; blockI < innerHeight; blockI += BLOCK_SIZE)
    {
        for (int blockJ = 0; blockJ < innerWidth; blockJ += BLOCK_SIZE)
        {
            const int blockHeight = std::min(BLOCK_SIZE, innerHeight - blockI);
            const int blockWidth  = std::min(BLOCK_SIZE, innerWidth - blockJ);

            for (int di = 0; di < blockHeight; ++di)
            {
                const int innerI = blockI + di;
                const int i = innerI + 1;

                for (int dj = 0; dj < blockWidth; ++dj)
                {
                    const int innerJ = blockJ + dj;
                    const int j = innerJ + 1;

                    const int center = toIndex(i, j);
                    const float currentHeight = src[center];

                    float totalDiff = 0.0f;
                    int validNeighbors = 0;

                    float diffs[8] = {0.0f};
                    int neighborIndices[8] = {0};
                    int neighborI[8] = {0};
                    int neighborJ[8] = {0};

                    for (int k = 0; k < mNeighborCount; ++k)
                    {
                        const int ni = i + mActiveNeighbors[k].di;
                        const int nj = j + mActiveNeighbors[k].dj;
                        const int nIndex = toIndex(ni, nj);

                        const float diff = currentHeight - src[nIndex];

                        diffs[k] = diff;
                        neighborIndices[k] = nIndex;
                        neighborI[k] = ni;
                        neighborJ[k] = nj;

                        if (diff > talusAngle) {
                            totalDiff += diff;
                            ++validNeighbors;
                        }
                    }

                    if (totalDiff <= 0.0f || validNeighbors <= 0) {
                        continue;
                    }

                    float materialToMove = transferRate * (totalDiff / validNeighbors);
                    materialToMove = std::min(materialToMove, currentHeight * transferRate);

                    #pragma omp atomic update
                    delta[center] -= materialToMove;

                    const float invTotalDiff = 1.0f / totalDiff;

                    for (int k = 0; k < mNeighborCount; ++k)
                    {
                        if (diffs[k] > talusAngle) {
                            const float moveAmount = materialToMove * (diffs[k] * invTotalDiff);

                            #pragma omp atomic update
                            delta[neighborIndices[k]] += moveAmount;
                        }
                    }

                    #pragma omp critical
                    {
                        markPatchDirtyFromCell(i, j);
                        for (int k = 0; k < mNeighborCount; ++k)
                        {
                            if (diffs[k] > talusAngle) {
                                markPatchDirtyFromCell(neighborI[k], neighborJ[k]);
                            }
                        }
                    }

                    ++changes;
                }
            }
        }
    }

    return changes;
}
void ThermalErosion::resetProgress()
{
    m_workingData.clear();
    mCurrentIndex = 0;
    mIterationFinished = false;
    mCellsProcessedSinceLastCommit = 0;
    mNeedsVisualUpdate = false;
    mDirtyPatchIndices.clear();
}

void ThermalErosion::commitWorkingData()
{
    if (!m_data || m_workingData.empty())
        return;

    *m_data = m_workingData;
    mCellsProcessedSinceLastCommit = 0;
    mNeedsVisualUpdate = false;
}

bool ThermalErosion::needsVisualUpdate() const
{
    return mNeedsVisualUpdate;
}

int ThermalErosion::step()
{
    if (!m_data) {
        std::cerr << "Error: Terrain data not loaded in ThermalErosion.\n";
        return 0;
    }

    const int totalInnerCells = (m_height - 2) * (m_width - 2);
    if (totalInnerCells <= 0) {
        return 0;
    }

    resetProgress();
    return stepChunk(totalInnerCells);
}

int ThermalErosion::stepChunk(int maxCells)
{
    const int W = m_width;
    const int H = m_height;

    if (!m_data) {
        std::cerr << "Error: Terrain data not loaded in ThermalErosion.\n";
        return 0;
    }

    if (W < 3 || H < 3 || maxCells <= 0) {
        return 0;
    }

    const int totalInnerCells = (H - 2) * (W - 2);

    if (m_workingData.empty() || static_cast<int>(m_workingData.size()) != W * H) {
        m_workingData = *m_data;
        mCurrentIndex = 0;
    }

    const float* src = m_data->data();
    float* dst = m_workingData.data();

    mIterationFinished = false;

    const int startIndex = mCurrentIndex;
    const int endIndex = std::min(mCurrentIndex + maxCells, totalInnerCells);

    mCellsProcessedSinceLastCommit += (endIndex - startIndex);
    if (mCellsProcessedSinceLastCommit >= mCommitThreshold) {
        mNeedsVisualUpdate = true;
    }

    const int changes = applyBlockedErosionRange(src, dst, startIndex, endIndex);

    mCurrentIndex = endIndex;

    if (mCurrentIndex >= totalInnerCells) {
        *m_data = m_workingData;
        m_workingData.clear();
        mCurrentIndex = 0;
        mIterationFinished = true;
        mCellsProcessedSinceLastCommit = 0;
        mNeedsVisualUpdate = false;
    }

    return changes;
}

int ThermalErosion::stepPureTwoPhase()
{
    if (!m_data) {
        std::cerr << "Error: Terrain data not loaded in ThermalErosion.\n";
        return 0;
    }

    if (m_width < 3 || m_height < 3) {
        return 0;
    }

    clearDirtyPatchIndices();

    const int totalInnerCells = (m_height - 2) * (m_width - 2);

    std::vector<float> srcSnapshot = *m_data;
    std::vector<float> dst = srcSnapshot;

    const int changes = applyErosionRange(srcSnapshot.data(),
                                          dst.data(),
                                          0,
                                          totalInnerCells);

    *m_data = std::move(dst);

    mIterationFinished = true;
    mNeedsVisualUpdate = false;
    mCellsProcessedSinceLastCommit = 0;
    mCurrentIndex = 0;

    return changes;
}

int ThermalErosion::stepBlockedPureTwoPhase()
{
    if (!m_data) {
        std::cerr << "Error: Terrain data not loaded in ThermalErosion.\n";
        return 0;
    }

    if (m_width < 3 || m_height < 3) {
        return 0;
    }

    clearDirtyPatchIndices();

    const int totalInnerCells = (m_height - 2) * (m_width - 2);

    std::vector<float> srcSnapshot = *m_data;
    std::vector<float> dst = srcSnapshot;

    const int changes = applyBlockedErosionRange(srcSnapshot.data(),
                                                 dst.data(),
                                                 0,
                                                 totalInnerCells);

    *m_data = std::move(dst);

    mIterationFinished = true;
    mNeedsVisualUpdate = false;
    mCellsProcessedSinceLastCommit = 0;
    mCurrentIndex = 0;

    return changes;
}
int ThermalErosion::stepBlockedParallelPureTwoPhase()
{
    if (!m_data) {
        std::cerr << "Error: Terrain data not loaded in ThermalErosion.\n";
        return 0;
    }

    if (m_width < 3 || m_height < 3) {
        return 0;
    }

    clearDirtyPatchIndices();

    std::vector<float> srcSnapshot = *m_data;
    std::vector<float> delta(srcSnapshot.size(), 0.0f);
    std::vector<float> dst = srcSnapshot;

    const int changes = applyBlockedParallelErosionToDelta(srcSnapshot.data(),
                                                           delta.data());

    for (std::size_t idx = 0; idx < dst.size(); ++idx) {
        dst[idx] += delta[idx];
    }

    *m_data = std::move(dst);

    mIterationFinished = true;
    mNeedsVisualUpdate = false;
    mCellsProcessedSinceLastCommit = 0;
    mCurrentIndex = 0;

    return changes;
}