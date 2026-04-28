#include "RendererManager.hpp"

#include <algorithm>
#include <iostream>
#include <omp.h>

RendererManager::RendererManager(Terrain* terrain)
{
    setTerrain(terrain);
}

void RendererManager::setTerrain(Terrain* terrain)
{
    mTerrain = terrain;
    mPatches.clear();
}

void RendererManager::createPatches()
{
    if (!mTerrain)
        return;

    const int width = mTerrain->getTerrainWidth();
    const int height = mTerrain->getTerrainHeight();
    const int nbPatchX = (width + kTerrainPatchSize - 1) / kTerrainPatchSize;
    const int nbPatchZ = (height + kTerrainPatchSize - 1) / kTerrainPatchSize;

    std::cout << "nb_patch_x : " << nbPatchX << " ,nb_patch_z : " << nbPatchZ << std::endl;

    mPatches.clear();
    mPatches.reserve(nbPatchX * nbPatchZ);

    for (int i = 0; i < nbPatchX; ++i)
    {
        for (int j = 0; j < nbPatchZ; ++j)
        {
            auto patch = std::make_unique<Patch>();
            patch->setPatch(i, j, mTerrain->getXzFactor(), nbPatchX, nbPatchZ, mTexture.get());
            mPatches.push_back(std::move(patch));
        }
    }

    connectPatchNeighbors();
}

void RendererManager::connectPatchNeighbors()
{
    if (!mTerrain)
    {
        return;
    }

    const int nbPatchX = (mTerrain->getTerrainWidth() + kTerrainPatchSize - 1) / kTerrainPatchSize;
    const int nbPatchZ = (mTerrain->getTerrainHeight() + kTerrainPatchSize - 1) / kTerrainPatchSize;

    auto patchAt = [this, nbPatchZ](int patchX, int patchZ) -> Patch*
    { return mPatches[patchX * nbPatchZ + patchZ].get(); };

    for (int patchX = 0; patchX < nbPatchX; ++patchX)
    {
        for (int patchZ = 0; patchZ < nbPatchZ; ++patchZ)
        {
            Patch* patch = patchAt(patchX, patchZ);

            if (patchX > 0)
            {
                patch->addNeighbor(patchAt(patchX - 1, patchZ));
            }
            if (patchX + 1 < nbPatchX)
            {
                patch->addNeighbor(patchAt(patchX + 1, patchZ));
            }
            if (patchZ > 0)
            {
                patch->addNeighbor(patchAt(patchX, patchZ - 1));
            }
            if (patchZ + 1 < nbPatchZ)
            {
                patch->addNeighbor(patchAt(patchX, patchZ + 1));
            }
        }
    }
}

void RendererManager::loadVerticesLod()
{
    if (!mTerrain)
        return;

    std::vector<float>* data = mTerrain->getData();
    for (auto& patch : mPatches)
    {
        patch->generateLodVertices(*data, mTerrain->getTerrainWidth(), mTerrain->getTerrainHeight());
    }
}

void RendererManager::loadIndicesLod()
{
    if (!mTerrain)
        return;

    std::vector<float>* data = mTerrain->getData();
    for (auto& patch : mPatches)
    {
        patch->generateLodIndices(*data, mTerrain->getTerrainWidth(), mTerrain->getTerrainHeight());
    }
}

void RendererManager::initTexture()
{
    if (!mTerrain)
        return;

    mTexture = std::make_unique<Texture>();
    mTexture->generateRegion(mTerrain->getMinHeight(), mTerrain->getMaxHeight());

    mTexture->loadTile("../src/texture/IMGP5525_seamless.jpg", 0);
    mTexture->loadTile("../src/texture/IMGP5487_seamless.jpg", 1);
    mTexture->loadTile("../src/texture/grass.jpg", 2);
    mTexture->loadTile("../src/texture/water.jpg", 3);
}

void RendererManager::setupTerrainLod(unsigned int& vao, unsigned int& vbo, unsigned int& ebo)
{
    (void)vao;
    (void)vbo;
    (void)ebo;

    if (!mTerrain)
        return;

    if (!mTexture)
        initTexture();

    createPatches();
    loadIndicesLod();
    loadVerticesLod();

    for (auto& patch : mPatches)
    {
        patch->createBuffersGL();
    }
}

void RendererManager::renderLod(const glm::vec3& cameraPos, glm::mat4& projection, glm::mat4& view)
{
    if (!mTerrain)
    {
        std::cerr << "Erreur: RendererManager::mTerrain is null!" << std::endl;
        return;
    }

    mFrustrum.updateFrustum(projection, view);

    if (mLodIsOn)
    {
        for (auto& patch : mPatches)
        {
            patch->setLodLevel(patch->chooseLod(cameraPos, &mFrustrum));
        }

        correctLod();

        for (auto& patch : mPatches)
        {
            if (patch->getLodLevel() != -1)
            {
                patch->render();
            }
        }
    }
    else
    {
        for (auto& patch : mPatches)
        {
            patch->setLodLevel(0);
            patch->render();
        }
    }
}

void RendererManager::correctLod()
{
    bool changed;
    int temp = 0;

    do
    {
        changed = false;

        for (auto& patch : mPatches)
        {
            if (patch->getLodLevel() == -1)
            {
                continue;
            }

            for (int j = 0; j < static_cast<int>(patch->getNeighbors().size()); ++j)
            {
                Patch* neighbor = patch->getNeighbors()[j];

                if (neighbor->getLodLevel() == -1)
                {
                    continue;
                }

                if (patch->getLodLevel() > neighbor->getLodLevel() + 1)
                {
                    temp = neighbor->getLodLevel() + 1;
                    patch->setLodLevel(temp);
                    changed = true;
                }

                if (neighbor->getLodLevel() > patch->getLodLevel() + 1)
                {
                    temp = patch->getLodLevel() + 1;
                    neighbor->setLodLevel(temp);
                    changed = true;
                }
            }
        }

    } while (changed);
}

void RendererManager::activateLod()
{
    mLodIsOn = !mLodIsOn;
}

Texture* RendererManager::getTexture()
{
    return mTexture.get();
}

std::vector<std::unique_ptr<Patch>>& RendererManager::getPatches()
{
    return mPatches;
}

void RendererManager::updateVerticesGpuLod(const std::vector<int>& dirtyPatchIndices)
{
    if (!mTerrain)
        return;

    std::vector<int> sortedDirty = dirtyPatchIndices;
    std::sort(sortedDirty.begin(), sortedDirty.end());

    std::vector<float>* data = mTerrain->getData();
    const int count = static_cast<int>(sortedDirty.size());

#pragma omp parallel for schedule(static)
    for (int k = 0; k < count; ++k)
    {
        const int idx = sortedDirty[k];
        if (idx >= 0 && idx < static_cast<int>(mPatches.size()))
        {
            mPatches[idx]->generateLodVertices(*data, mTerrain->getTerrainWidth(), mTerrain->getTerrainHeight());
        }
    }

    for (int k = 0; k < count; ++k)
    {
        const int idx = sortedDirty[k];
        if (idx >= 0 && idx < static_cast<int>(mPatches.size()))
        {
            mPatches[idx]->uploadLodToGpu();
        }
    }
}

void RendererManager::updateVerticesGpuLod()
{
    if (!mTerrain)
        return;

    std::vector<float>* data = mTerrain->getData();
    const int count = static_cast<int>(mPatches.size());

#pragma omp parallel for schedule(static)
    for (int i = 0; i < count; ++i)
    {
        mPatches[i]->generateLodVertices(*data, mTerrain->getTerrainWidth(), mTerrain->getTerrainHeight());
    }

    for (auto& patch : mPatches)
    {
        patch->uploadLodToGpu();
    }
}
