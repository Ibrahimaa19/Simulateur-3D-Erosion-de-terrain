#include "RendererManager.hpp"

RendererManager::RendererManager(Terrain *terrain)
{
    this->mTerrain = terrain;
    this->mFrustrum = new Frustrum;
    this->mLodIsOn = true;
}

RendererManager::~RendererManager()
{
    delete mFrustrum;
}

void RendererManager::renderLod(const glm::vec3 &cameraPos, glm::mat4 &projection, glm::mat4 &view)
{

    if (!mTerrain)
    {
        std::cerr << "Erreur: RendererManager::mTerrain is null!" << std::endl;
        return;
    }

    if (!mFrustrum)
    {
        std::cerr << "Erreur: RendererManager::mFrustrum is null!" << std::endl;
        return;
    }

    std::vector<std::unique_ptr<Patch>> &patches = mTerrain->getPatches();
    mFrustrum->updateFrustum(projection, view);

    if (mLodIsOn)
    {
        int temp = 0;
        for (int i = 0; i < patches.size(); ++i)
        {
            temp = patches[i]->chooseLod(cameraPos, mFrustrum);
            patches[i]->setLodLevel(temp);
        }

        for (int i = 0; i < patches.size(); ++i)
        {
            if (patches[i]->getLodLevel() != -1)
                patches[i]->render();
        }
    }
    else
    {
        for (int i = 0; i < patches.size(); ++i)
        {
            patches[i]->setLodLevel(0);
            patches[i]->render();
        }
    }
}

void RendererManager::correctLod()
{
    bool changed;
    int temp = 0;
    std::vector<std::unique_ptr<Patch>> &patches = mTerrain->getPatches();

    do
    {
        changed = false;

        for (int i = 0; i < patches.size(); ++i)
        {

            for (int j = 0; j < patches[i]->getNeighbors().size(); ++j)
            {

                if (patches[i]->getLodLevel() > patches[i]->getNeighborLodLevel(j) + 1)
                {
                    temp = patches[i]->getNeighborLodLevel(j) + 1;
                    patches[i]->setLodLevel(temp);
                    changed = true;
                }

                if (patches[i]->getNeighborLodLevel(j) > patches[i]->getLodLevel() + 1)
                {
                    temp = patches[i]->getLodLevel() + 1;
                    patches[i]->getNeighbors()[j]->setLodLevel(temp);
                    changed = true;
                }
            }
        }

    } while (changed);
}

void RendererManager::activateLod()
{
    this->mLodIsOn = !this->mLodIsOn;
}

void RendererManager::setTerrain(Terrain *terrain)
{
    this->mTerrain = terrain;
}