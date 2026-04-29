#ifndef RENDERER_MANAGER_H
#define RENDERER_MANAGER_H

#include "Frustrum.hpp"
#include "Patch.hpp"
#include "Terrain.hpp"
#include "Texture.hpp"

#include <glm/glm.hpp>
#include <memory>
#include <vector>

/**
 * @brief Gère toutes les ressources de rendu associées à un Terrain CPU.
 *
 * Terrain reste indépendant d'OpenGL/GLM. Cette classe possède les patches,
 * textures, buffers GPU et le frustum nécessaires au rendu LOD.
 */
class RendererManager
{
  private:
    Terrain* mTerrain = nullptr;
    Frustrum mFrustrum;
    bool mLodIsOn = true;

    std::vector<std::unique_ptr<Patch>> mPatches;
    std::unique_ptr<Texture> mTexture;

    void loadVerticesLod();
    void loadIndicesLod();
    void createPatches();
    void connectPatchNeighbors();

    /**
     * @brief Corrige les différences de LOD entre patches voisins.
     */
    void correctLod();

  public:
    explicit RendererManager(Terrain* terrain);
    ~RendererManager() = default;

    void renderLod(const glm::vec3& cameraPos, glm::mat4& projection, glm::mat4& view);

    void activateLod();
    void setTerrain(Terrain* terrain);

    void initTexture();
    void setupTerrainLod(unsigned int& vao, unsigned int& vbo, unsigned int& ebo);
    void updateVerticesGpuLod();
    void updateVerticesGpuLod(const std::vector<int>& dirtyPatchIndices);

    Texture* getTexture();
    std::vector<std::unique_ptr<Patch>>& getPatches();
};

#endif
