#include "ThermalErosion.hpp"
#ifdef _OPENMP
#include <omp.h>
#endif

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

bool ThermalErosion::buildCellStencil(int i,
                                      int j,
                                      const float* src,
                                      CellStencil& stencil) const
{
    stencil = CellStencil{};

    stencil.center = toIndex(i, j);
    stencil.currentHeight = src[stencil.center];

    for (int k = 0; k < mNeighborCount; ++k)
    {
        const int ni = i + mActiveNeighbors[k].di;
        const int nj = j + mActiveNeighbors[k].dj;
        const int nIndex = toIndex(ni, nj);

        const float diff = stencil.currentHeight - src[nIndex];

        stencil.diffs[k] = diff;
        stencil.neighborIndices[k] = nIndex;
        stencil.neighborI[k] = ni;
        stencil.neighborJ[k] = nj;

        if (diff > talusAngle) {
            stencil.totalDiff += diff;
            ++stencil.validNeighbors;
            stencil.activeSlots[stencil.activeCount++] = k;
        }
    }

    return stencil.totalDiff > 0.0f && stencil.validNeighbors > 0;
}

float ThermalErosion::computeMaterialToMove(const CellStencil& stencil) const
{
    float materialToMove =
        transferRate * (stencil.totalDiff / stencil.validNeighbors);

    materialToMove = std::min(materialToMove,
                              stencil.currentHeight * transferRate);

    return materialToMove;
}

void ThermalErosion::applyTransfersToBuffer(const CellStencil& stencil,
                                            float materialToMove,
                                            float* dst)
{
    dst[stencil.center] -= materialToMove;

    const float invTotalDiff = 1.0f / stencil.totalDiff;

    for (int a = 0; a < stencil.activeCount; ++a)
    {
        const int k = stencil.activeSlots[a];
        const float moveAmount =
            materialToMove * (stencil.diffs[k] * invTotalDiff);

        dst[stencil.neighborIndices[k]] += moveAmount;
    }
}

void ThermalErosion::markDirtyFromStencil(int i,
                                          int j,
                                          const CellStencil& stencil)
{
    markPatchDirtyFromCell(i, j);

    for (int k = 0; k < mNeighborCount; ++k)
    {
        if (stencil.diffs[k] > talusAngle) {
            markPatchDirtyFromCell(stencil.neighborI[k],
                                   stencil.neighborJ[k]);
        }
    }
}

bool ThermalErosion::erodeCell(int i, int j, const float* src, float* dst)
{
    CellStencil stencil;

    if (!buildCellStencil(i, j, src, stencil)) {
        return false;
    }

    const float materialToMove = computeMaterialToMove(stencil);

    applyTransfersToBuffer(stencil, materialToMove, dst);
    markDirtyFromStencil(i, j, stencil);

    return true;
}
bool ThermalErosion::erodeCellInPlace(int i, int j, float* data)
{
    CellStencil stencil;

    if (!buildCellStencil(i, j, data, stencil)) {
        return false;
    }

    const float materialToMove = computeMaterialToMove(stencil);

    applyTransfersToBuffer(stencil, materialToMove, data);
    markDirtyFromStencil(i, j, stencil);

    return true;
}
bool ThermalErosion::erodeCellToDeltaSerial(int i,
                                            int j,
                                            const float* src,
                                            float* delta)
{
    CellStencil stencil;

    if (!buildCellStencil(i, j, src, stencil)) {
        return false;
    }

    const float materialToMove = computeMaterialToMove(stencil);

    applyTransfersToBuffer(stencil, materialToMove, delta);
    markDirtyFromStencil(i, j, stencil);

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
int ThermalErosion::applyCheckerboardErosionRange(const float* src,
                                                  float* dst,
                                                  int color)
{
    int changes = 0;

    for (int i = 1; i < m_height - 1; ++i)
    {
        for (int j = 1; j < m_width - 1; ++j)
        {
            if (((i + j) & 1) != color) {
                continue;
            }

            if (erodeCell(i, j, src, dst)) {
                ++changes;
            }
        }
    }

    return changes;
}
int ThermalErosion::applyBlockedCheckerboardErosionRange(const float* src,
                                                         float* dst,
                                                         int color)
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
                const int i = innerI + 1;

                for (int dj = 0; dj < blockWidth; ++dj)
                {
                    const int innerJ = blockJ + dj;
                    const int j = innerJ + 1;

                    if (((i + j) & 1) != color) {
                        continue;
                    }

                    if (erodeCell(i, j, src, dst)) {
                        ++changes;
                    }
                }
            }
        }
    }

    return changes;
}
int ThermalErosion::stepCheckerboardPureTwoPhase()
{
    if (!m_data) {
        std::cerr << "Error: Terrain data not loaded in ThermalErosion.\n";
        return 0;
    }

    if (m_width < 3 || m_height < 3) {
        return 0;
    }

    if (mNeighborCount != 4) {
        std::cerr << "Warning: checkerboard scheduling is intended for four-neighbor mode.\n";
    }

    clearDirtyPatchIndices();

    std::vector<float> srcSnapshot = *m_data;
    std::vector<float> dst = srcSnapshot;

    int changes = 0;
    changes += applyCheckerboardErosionRange(srcSnapshot.data(), dst.data(), 0);
    changes += applyCheckerboardErosionRange(srcSnapshot.data(), dst.data(), 1);

    *m_data = std::move(dst);

    mIterationFinished = true;
    mNeedsVisualUpdate = false;
    mCellsProcessedSinceLastCommit = 0;
    mCurrentIndex = 0;

    return changes;
}
int ThermalErosion::stepBlockedCheckerboardPureTwoPhase()
{
    if (!m_data) {
        std::cerr << "Error: Terrain data not loaded in ThermalErosion.\n";
        return 0;
    }

    if (m_width < 3 || m_height < 3) {
        return 0;
    }

    if (mNeighborCount != 4) {
        std::cerr << "Warning: checkerboard scheduling is intended for four-neighbor mode.\n";
    }

    clearDirtyPatchIndices();

    std::vector<float> srcSnapshot = *m_data;
    std::vector<float> dst = srcSnapshot;

    int changes = 0;
    changes += applyBlockedCheckerboardErosionRange(srcSnapshot.data(), dst.data(), 0);
    changes += applyBlockedCheckerboardErosionRange(srcSnapshot.data(), dst.data(), 1);

    *m_data = std::move(dst);

    mIterationFinished = true;
    mNeedsVisualUpdate = false;
    mCellsProcessedSinceLastCommit = 0;
    mCurrentIndex = 0;

    return changes;
}
int ThermalErosion::applyBlockedParallelErosionToThreadLocalBuffers(
    const float* src,
    std::vector<std::vector<float>>& threadDeltas,
    std::vector<std::vector<unsigned char>>& threadPatchMarked)
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
#ifdef _OPENMP
            const int tid = omp_get_thread_num();
#else
            const int tid = 0;
#endif

            std::vector<float>& localDelta = threadDeltas[tid];
            std::vector<unsigned char>& localPatchMask = threadPatchMarked[tid];

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

                    CellStencil stencil;

                    if (!buildCellStencil(i, j, src, stencil)) {
                        continue;
                    }

                    const float materialToMove = computeMaterialToMove(stencil);

                    applyTransfersToBuffer(stencil,
                                           materialToMove,
                                           localDelta.data());

                    markPatchMaskFromStencil(i, j, stencil, localPatchMask);

                    ++changes;
                }
            }
        }
    }

    return changes;
}
int ThermalErosion::applyCheckerboardInPlaceColor(float* data, int color)
{
    int changes = 0;

    for (int i = 1; i < m_height - 1; ++i)
    {
        for (int j = 1; j < m_width - 1; ++j)
        {
            if (((i + j) & 1) != color) {
                continue;
            }

            if (erodeCellInPlace(i, j, data)) {
                ++changes;
            }
        }
    }

    return changes;
}
int ThermalErosion::stepCheckerboardInPlace()
{
    if (!m_data) {
        std::cerr << "Error: Terrain data not loaded in ThermalErosion.\n";
        return 0;
    }

    if (m_width < 3 || m_height < 3) {
        return 0;
    }

    if (mNeighborCount != 4) {
        std::cerr << "Warning: in-place checkerboard is intended for four-neighbor mode.\n";
    }

    clearDirtyPatchIndices();

    float* data = m_data->data();

    int changes = 0;
    changes += applyCheckerboardInPlaceColor(data, 0);
    changes += applyCheckerboardInPlaceColor(data, 1);

    mIterationFinished = true;
    mNeedsVisualUpdate = false;
    mCellsProcessedSinceLastCommit = 0;
    mCurrentIndex = 0;

    return changes;
}
void ThermalErosion::resetProgress()
{
    m_workingData.clear();
    mCurrentIndex = 0;
    mIterationFinished = false;
    mCellsProcessedSinceLastCommit = 0;
    mNeedsVisualUpdate = false;
    std::fill(mPatchMarked.begin(), mPatchMarked.end(), false);
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
    if (mCurrentIndex == 0) {
        clearDirtyPatchIndices();
    }
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
    std::vector<float> dst = srcSnapshot;

#ifdef _OPENMP
    const int numThreads = omp_get_max_threads();
#else
    const int numThreads = 1;
#endif

    std::vector<std::vector<float>> threadDeltas(
        numThreads,
        std::vector<float>(srcSnapshot.size(), 0.0f)
    );

    std::vector<std::vector<unsigned char>> threadPatchMarked(
        numThreads,
        std::vector<unsigned char>(mNbPatchX * mNbPatchZ, 0)
    );

    const int changes = applyBlockedParallelErosionToThreadLocalBuffers(
        srcSnapshot.data(),
        threadDeltas,
        threadPatchMarked
    );

    #pragma omp parallel for schedule(static)
    for (std::ptrdiff_t idx = 0; idx < static_cast<std::ptrdiff_t>(dst.size()); ++idx)
    {
        float sum = 0.0f;
        for (int t = 0; t < numThreads; ++t) {
            sum += threadDeltas[t][idx];
        }
        dst[idx] += sum;
    }

    for (int patchIdx = 0; patchIdx < mNbPatchX * mNbPatchZ; ++patchIdx)
    {
        bool dirty = false;
        for (int t = 0; t < numThreads; ++t)
        {
            if (threadPatchMarked[t][patchIdx]) {
                dirty = true;
                break;
            }
        }

        if (dirty) {
            mPatchMarked[patchIdx] = true;
            mDirtyPatchIndices.push_back(patchIdx);
        }
    }

    *m_data = std::move(dst);

    mIterationFinished = true;
    mNeedsVisualUpdate = false;
    mCellsProcessedSinceLastCommit = 0;
    mCurrentIndex = 0;

    return changes;
}

int ThermalErosion::applyCheckerboardInPlaceColorParallelBuffered(float* data, int color)
{
    if (mNeighborCount != 4) {
        std::cerr << "Warning: checkerboard in-place parallel is intended for four-neighbor mode.\n";
    }

#ifdef _OPENMP
    const int numThreads = omp_get_max_threads();
#else
    const int numThreads = 1;
#endif

    const std::size_t dataSize =
        static_cast<std::size_t>(m_width) * static_cast<std::size_t>(m_height);
    const int numPatches = mNbPatchX * mNbPatchZ;

    std::vector<std::vector<float>> threadDeltas(
        numThreads,
        std::vector<float>(dataSize, 0.0f)
    );

    std::vector<std::vector<unsigned char>> threadPatchMasks(
        numThreads,
        std::vector<unsigned char>(numPatches, 0)
    );

    int changes = 0;

    #pragma omp parallel for reduction(+:changes) schedule(static)
    for (int i = 1; i < m_height - 1; ++i)
    {
#ifdef _OPENMP
        const int tid = omp_get_thread_num();
#else
        const int tid = 0;
#endif

        std::vector<float>& localDelta = threadDeltas[tid];
        std::vector<unsigned char>& localPatchMask = threadPatchMasks[tid];

        for (int j = 1; j < m_width - 1; ++j)
        {
            if (((i + j) & 1) != color) {
                continue;
            }

            CellStencil stencil;

            if (!buildCellStencil(i, j, data, stencil)) {
                continue;
            }

            const float materialToMove = computeMaterialToMove(stencil);

            applyTransfersToBuffer(stencil,
                                   materialToMove,
                                   localDelta.data());

            markPatchMaskFromStencil(i, j, stencil, localPatchMask);

            ++changes;
        }
    }

    #pragma omp parallel for schedule(static)
    for (std::ptrdiff_t idx = 0; idx < static_cast<std::ptrdiff_t>(dataSize); ++idx)
    {
        float sum = 0.0f;
        for (int t = 0; t < numThreads; ++t) {
            sum += threadDeltas[t][idx];
        }
        data[idx] += sum;
    }

    for (int patchIdx = 0; patchIdx < numPatches; ++patchIdx)
    {
        bool dirty = false;
        for (int t = 0; t < numThreads; ++t)
        {
            if (threadPatchMasks[t][patchIdx]) {
                dirty = true;
                break;
            }
        }

        if (dirty && !mPatchMarked[patchIdx]) {
            mPatchMarked[patchIdx] = true;
            mDirtyPatchIndices.push_back(patchIdx);
        }
    }

    return changes;
}
int ThermalErosion::stepCheckerboardInPlaceParallel()
{
    if (!m_data) {
        std::cerr << "Error: Terrain data not loaded in ThermalErosion.\n";
        return 0;
    }

    if (m_width < 3 || m_height < 3) {
        return 0;
    }

    if (mNeighborCount != 4) {
        std::cerr << "Warning: parallel in-place checkerboard is intended for four-neighbor mode.\n";
    }

    clearDirtyPatchIndices();

    float* data = m_data->data();

    int changes = 0;
    changes += applyCheckerboardInPlaceColorParallelBuffered(data, 0);
    changes += applyCheckerboardInPlaceColorParallelBuffered(data, 1);

    mIterationFinished = true;
    mNeedsVisualUpdate = false;
    mCellsProcessedSinceLastCommit = 0;
    mCurrentIndex = 0;

    return changes;
}

void ThermalErosion::markPatchMaskFromStencil(
    int i,
    int j,
    const CellStencil& stencil,
    std::vector<unsigned char>& patchMask) const
{
    patchMask[patchIndexFromCell(i, j)] = 1;

    for (int k = 0; k < mNeighborCount; ++k)
    {
        if (stencil.diffs[k] > talusAngle) {
            patchMask[patchIndexFromCell(stencil.neighborI[k],
                                         stencil.neighborJ[k])] = 1;
        }
    }
}