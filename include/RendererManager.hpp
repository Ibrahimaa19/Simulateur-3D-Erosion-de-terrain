#ifndef RENDERER_MANAGER_H
#define RENDERER_MANAGER_H

#include "Terrain.hpp"
#include <glm/glm.hpp>

class RendererManager
{
private:
    Terrain* mTerrain;
    Frustrum* mFrustrum;
    bool lodIsOn = true;
    void correctLod();

public:
    RendererManager(Terrain* ter);
    ~RendererManager();
    void rendererLod(const glm::vec3& cameraPos,glm::mat4& mProjection,glm::mat4& mView);
    
    void activateLod();

    void changeTerrain(Terrain* ter);
};


#endif