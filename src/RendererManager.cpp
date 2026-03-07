#include "RendererManager.hpp"

RendererManager::RendererManager(Terrain* mTer)
{
    this->mTerrain = mTer;
    this->mFrustrum = new Frustrum;
    lodIsOn = true;
}

RendererManager::~RendererManager()
{
    delete mFrustrum;
}

void RendererManager::rendererLod(const glm::vec3& cameraPos,glm::mat4& mProjection,glm::mat4& mView){

    if (!mTerrain) {
        std::cerr << "ERROR: RendererManager::mTerrain is null!" << std::endl;
        return;
    }
    
    if (!mFrustrum) {
        std::cerr << "ERROR: RendererManager::mFrustrum is null!" << std::endl;
        return;
    }

    std::vector<std::unique_ptr<Patch>>& patches = mTerrain->getPatches();
    mFrustrum->updateFrustum(mProjection,mView);

    if (lodIsOn)
    {
        
        int temp = 0;
        for(int i =0;i<patches.size();++i){
            temp = patches[i]->chooseLod(cameraPos,mFrustrum);
            patches[i]->setLodLevel(temp);

        }
        
        for(int i =0;i<patches.size();++i){
            if(patches[i]->getLodLevel() != -1)
                patches[i]->render();
        }
    }else{
        for(int i =0;i<patches.size();++i){
            patches[i]->setLodLevel(0);
            patches[i]->render();
        }
    }

}


void RendererManager::correctLod(){
    bool changed;
    int temp = 0;
    std::vector<std::unique_ptr<Patch>>& patches = mTerrain->getPatches();

    do
    {
        changed = false;

        for (int i =0;i<patches.size();++i)
        {

            for (int j =0;j<patches[i]->getNeightbour().size();++j)
            {

                if (patches[i]->getLodLevel() > patches[i]->getNeightbourLodLevel(j) + 1)
                {
                    temp = patches[i]->getNeightbourLodLevel(j) + 1;
                    patches[i]->setLodLevel(temp);
                    changed = true;
                }

                if (patches[i]->getNeightbourLodLevel(j) > patches[i]->getLodLevel() + 1)
                {
                    temp = patches[i]->getLodLevel() + 1;
                    patches[i]->getNeightbour()[j]->setLodLevel(temp);
                    changed = true;
                }
            }
        }

    } while(changed);

}

void RendererManager::activateLod(){
    this->lodIsOn = !this->lodIsOn;
}

void RendererManager::changeTerrain(Terrain* mTer){
    this->mTerrain = mTer;
}