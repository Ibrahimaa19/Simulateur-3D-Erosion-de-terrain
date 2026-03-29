#include "ThermalErosion.hpp"

ThermalErosion::ThermalErosion()
{
    useEightNeighbors();
}
const ThermalErosion::NeighborOffset ThermalErosion::kNeighbors8[8] = {
    {-1,  0}, // up
    { 1,  0}, // down
    { 0, -1}, // left
    { 0,  1}, // right
    {-1, -1}, // upLeft
    {-1,  1}, // upRight
    { 1, -1}, // downLeft
    { 1,  1}  // downRight
};

const ThermalErosion::NeighborOffset ThermalErosion::kNeighbors4[4] = {
    {-1,  0}, // up
    { 1,  0}, // down
    { 0, -1}, // left
    { 0,  1}  // right
};

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

    if (mActiveNeighbors == nullptr || mNeighborCount == 0) {
        useEightNeighbors();
    }

    const int totalInnerCells = (H - 2) * (W - 2);

    if (m_workingData.empty() || static_cast<int>(m_workingData.size()) != W * H) {
        m_workingData = *m_data;
        mCurrentIndex = 0;
    }

    const float* src = m_data->data();
    float* dst = m_workingData.data();

    mIterationFinished = false;
    int changes = 0;

    const int startIndex = mCurrentIndex;
    const int endIndex = std::min(mCurrentIndex + maxCells, totalInnerCells);

    mCellsProcessedSinceLastCommit += (endIndex - startIndex);
    if (mCellsProcessedSinceLastCommit >= mCommitThreshold) {
        mNeedsVisualUpdate = true;
    }

    int processed = 0;

    const int innerWidth  = W - 2;
    const int innerHeight = H - 2;

    const int totalCells = innerWidth * innerHeight;

    int currentLinear = startIndex;

    while (currentLinear < endIndex)
    {
        // Convertit en coordonnée globale
        int i, j;
        localIndexToCoords(currentLinear, i, j);

        // Début du bloc
        const int blockStartI = ((i - 1) / BLOCK_SIZE) * BLOCK_SIZE + 1;
        const int blockStartJ = ((j - 1) / BLOCK_SIZE) * BLOCK_SIZE + 1;

        const int blockEndI = std::min(blockStartI + BLOCK_SIZE, H - 1);
        const int blockEndJ = std::min(blockStartJ + BLOCK_SIZE, W - 1);

        // Parcours du bloc
        for (int bi = blockStartI; bi < blockEndI; ++bi)
        {
            for (int bj = blockStartJ; bj < blockEndJ; ++bj)
            {
                // reconstruire localIndex équivalent
                int localIdx = (bi - 1) * innerWidth + (bj - 1);

                if (localIdx < startIndex || localIdx >= endIndex)
                    continue;

                if (erodeCell(bi, bj, src, dst)) {
                    ++changes;
                }

                ++processed;
            }
        }

        // passer au bloc suivant
        currentLinear = (blockEndI - 1) * innerWidth + (blockEndJ - 1);
    }

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