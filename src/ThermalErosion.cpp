#include "ThermalErosion.hpp"

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
    const int W = m_width;

    const int center    = toIndex(i, j);
    const int up        = toIndex(i - 1, j);
    const int down      = toIndex(i + 1, j);
    const int left      = toIndex(i, j - 1);
    const int right     = toIndex(i, j + 1);
    const int upLeft    = toIndex(i - 1, j - 1);
    const int upRight   = toIndex(i - 1, j + 1);
    const int downLeft  = toIndex(i + 1, j - 1);
    const int downRight = toIndex(i + 1, j + 1);

    const float currentHeight = src[center];

    const float diffUp        = currentHeight - src[up];
    const float diffDown      = currentHeight - src[down];
    const float diffLeft      = currentHeight - src[left];
    const float diffRight     = currentHeight - src[right];
    const float diffUpLeft    = currentHeight - src[upLeft];
    const float diffUpRight   = currentHeight - src[upRight];
    const float diffDownLeft  = currentHeight - src[downLeft];
    const float diffDownRight = currentHeight - src[downRight];

    float totalDiff = 0.0f;
    int validNeighbors = 0;

    if (diffUp > talusAngle)        { totalDiff += diffUp;        ++validNeighbors; }
    if (diffDown > talusAngle)      { totalDiff += diffDown;      ++validNeighbors; }
    if (diffLeft > talusAngle)      { totalDiff += diffLeft;      ++validNeighbors; }
    if (diffRight > talusAngle)     { totalDiff += diffRight;     ++validNeighbors; }
    if (diffUpLeft > talusAngle)    { totalDiff += diffUpLeft;    ++validNeighbors; }
    if (diffUpRight > talusAngle)   { totalDiff += diffUpRight;   ++validNeighbors; }
    if (diffDownLeft > talusAngle)  { totalDiff += diffDownLeft;  ++validNeighbors; }
    if (diffDownRight > talusAngle) { totalDiff += diffDownRight; ++validNeighbors; }

    if (totalDiff <= 0.0f || validNeighbors <= 0) {
        return false;
    }

    markPatchDirtyFromCell(i, j);

    float materialToMove = transferRate * (totalDiff / validNeighbors);
    materialToMove = std::min(materialToMove, currentHeight * transferRate);

    dst[center] -= materialToMove;

    const float invTotalDiff = 1.0f / totalDiff;

    if (diffUp > talusAngle) {
        const float moveAmount = materialToMove * (diffUp * invTotalDiff);
        addMaterialToNeighbor(dst, up, moveAmount, i - 1, j);
    }

    if (diffDown > talusAngle) {
        const float moveAmount = materialToMove * (diffDown * invTotalDiff);
        addMaterialToNeighbor(dst, down, moveAmount, i + 1, j);
    }

    if (diffLeft > talusAngle) {
        const float moveAmount = materialToMove * (diffLeft * invTotalDiff);
        addMaterialToNeighbor(dst, left, moveAmount, i, j - 1);
    }

    if (diffRight > talusAngle) {
        const float moveAmount = materialToMove * (diffRight * invTotalDiff);
        addMaterialToNeighbor(dst, right, moveAmount, i, j + 1);
    }

    if (diffUpLeft > talusAngle) {
        const float moveAmount = materialToMove * (diffUpLeft * invTotalDiff);
        addMaterialToNeighbor(dst, upLeft, moveAmount, i - 1, j - 1);
    }

    if (diffUpRight > talusAngle) {
        const float moveAmount = materialToMove * (diffUpRight * invTotalDiff);
        addMaterialToNeighbor(dst, upRight, moveAmount, i - 1, j + 1);
    }

    if (diffDownLeft > talusAngle) {
        const float moveAmount = materialToMove * (diffDownLeft * invTotalDiff);
        addMaterialToNeighbor(dst, downLeft, moveAmount, i + 1, j - 1);
    }

    if (diffDownRight > talusAngle) {
        const float moveAmount = materialToMove * (diffDownRight * invTotalDiff);
        addMaterialToNeighbor(dst, downRight, moveAmount, i + 1, j + 1);
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

    for (int localIndex = startIndex; localIndex < endIndex; ++localIndex)
    {
        int i, j;
        localIndexToCoords(localIndex, i, j);

        if (erodeCell(i, j, src, dst)) {
            ++changes;
        }
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