#include "ThermalErosion.hpp"

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
    int changes = 0;

    const int startIndex = mCurrentIndex;
    const int endIndex = std::min(mCurrentIndex + maxCells, totalInnerCells);

    mCellsProcessedSinceLastCommit += (endIndex - startIndex);
    if (mCellsProcessedSinceLastCommit >= mCommitThreshold) {
        mNeedsVisualUpdate = true;
    }

    for (int localIndex = startIndex; localIndex < endIndex; ++localIndex)
    {
        int i = 1 + localIndex / (W - 2);
        int j = 1 + localIndex % (W - 2);

        const int center = i * W + j;
        const int up = center - W;
        const int down = center + W;
        const int left = center - 1;
        const int right = center + 1;
        const int upLeft = up - 1;
        const int upRight = up + 1;
        const int downLeft = down - 1;
        const int downRight = down + 1;

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

        if (totalDiff > 0.0f && validNeighbors > 0)
        {
            const int centerPatchX = j / PATCH_SIZE;
            const int centerPatchZ = i / PATCH_SIZE;
            const int centerPatchIndex = centerPatchX * mNbPatchZ + centerPatchZ;

            if (!mPatchMarked[centerPatchIndex]) {
                mPatchMarked[centerPatchIndex] = true;
                mDirtyPatchIndices.push_back(centerPatchIndex);
            }

            float materialToMove = transferRate * (totalDiff / validNeighbors);
            materialToMove = std::min(materialToMove, currentHeight * transferRate);

            dst[center] -= materialToMove;

            const float invTotalDiff = 1.0f / totalDiff;

            if (diffUp > talusAngle) {
                float moveAmount = materialToMove * (diffUp * invTotalDiff);
                dst[up] += moveAmount;

                int patchX = j / PATCH_SIZE;
                int patchZ = (i - 1) / PATCH_SIZE;
                int patchIndex = patchX * mNbPatchZ + patchZ;
                if (!mPatchMarked[patchIndex]) {
                    mPatchMarked[patchIndex] = true;
                    mDirtyPatchIndices.push_back(patchIndex);
                }
            }

            if (diffDown > talusAngle) {
                float moveAmount = materialToMove * (diffDown * invTotalDiff);
                dst[down] += moveAmount;

                int patchX = j / PATCH_SIZE;
                int patchZ = (i + 1) / PATCH_SIZE;
                int patchIndex = patchX * mNbPatchZ + patchZ;
                if (!mPatchMarked[patchIndex]) {
                    mPatchMarked[patchIndex] = true;
                    mDirtyPatchIndices.push_back(patchIndex);
                }
            }

            if (diffLeft > talusAngle) {
                float moveAmount = materialToMove * (diffLeft * invTotalDiff);
                dst[left] += moveAmount;

                int patchX = (j - 1) / PATCH_SIZE;
                int patchZ = i / PATCH_SIZE;
                int patchIndex = patchX * mNbPatchZ + patchZ;
                if (!mPatchMarked[patchIndex]) {
                    mPatchMarked[patchIndex] = true;
                    mDirtyPatchIndices.push_back(patchIndex);
                }
            }

            if (diffRight > talusAngle) {
                float moveAmount = materialToMove * (diffRight * invTotalDiff);
                dst[right] += moveAmount;

                int patchX = (j + 1) / PATCH_SIZE;
                int patchZ = i / PATCH_SIZE;
                int patchIndex = patchX * mNbPatchZ + patchZ;
                if (!mPatchMarked[patchIndex]) {
                    mPatchMarked[patchIndex] = true;
                    mDirtyPatchIndices.push_back(patchIndex);
                }
            }

            if (diffUpLeft > talusAngle) {
                float moveAmount = materialToMove * (diffUpLeft * invTotalDiff);
                dst[upLeft] += moveAmount;

                int patchX = (j - 1) / PATCH_SIZE;
                int patchZ = (i - 1) / PATCH_SIZE;
                int patchIndex = patchX * mNbPatchZ + patchZ;
                if (!mPatchMarked[patchIndex]) {
                    mPatchMarked[patchIndex] = true;
                    mDirtyPatchIndices.push_back(patchIndex);
                }
            }

            if (diffUpRight > talusAngle) {
                float moveAmount = materialToMove * (diffUpRight * invTotalDiff);
                dst[upRight] += moveAmount;

                int patchX = (j + 1) / PATCH_SIZE;
                int patchZ = (i - 1) / PATCH_SIZE;
                int patchIndex = patchX * mNbPatchZ + patchZ;
                if (!mPatchMarked[patchIndex]) {
                    mPatchMarked[patchIndex] = true;
                    mDirtyPatchIndices.push_back(patchIndex);
                }
            }

            if (diffDownLeft > talusAngle) {
                float moveAmount = materialToMove * (diffDownLeft * invTotalDiff);
                dst[downLeft] += moveAmount;

                int patchX = (j - 1) / PATCH_SIZE;
                int patchZ = (i + 1) / PATCH_SIZE;
                int patchIndex = patchX * mNbPatchZ + patchZ;
                if (!mPatchMarked[patchIndex]) {
                    mPatchMarked[patchIndex] = true;
                    mDirtyPatchIndices.push_back(patchIndex);
                }
            }

            if (diffDownRight > talusAngle) {
                float moveAmount = materialToMove * (diffDownRight * invTotalDiff);
                dst[downRight] += moveAmount;

                int patchX = (j + 1) / PATCH_SIZE;
                int patchZ = (i + 1) / PATCH_SIZE;
                int patchIndex = patchX * mNbPatchZ + patchZ;
                if (!mPatchMarked[patchIndex]) {
                    mPatchMarked[patchIndex] = true;
                    mDirtyPatchIndices.push_back(patchIndex);
                }
            }

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