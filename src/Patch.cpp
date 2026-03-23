#include "Patch.hpp"

void Patch::setPatch(unsigned int x, unsigned int z, float xzFactor, unsigned int nbPatchX, unsigned int nbPatchZ,Texture* texture)
{
    this->mPatchX = x;
    this->mPatchZ = z;
    this->mXzFactor = xzFactor;
    this->mNbPatchX = nbPatchX;
    this->mNbPatchZ = nbPatchZ;
    this->mPatchTexture = texture;
}

void Patch::addNeighbor(Patch *neighbor)
{
    mNeighbors.push_back(neighbor);
}

std::vector<Patch *> Patch::getNeighbors()
{
    return this->mNeighbors;
}

int Patch::getNeighborLodLevel(int index)
{
    return mNeighbors[index]->mLodLevel;
}

unsigned int Patch::getPatchX()
{
    return mPatchX;
}

unsigned int Patch::getPatchZ()
{
    return mPatchZ;
}

GLuint Patch::getVbo(int lodLevel)
{
    return mVbo[lodLevel];
}

void Patch::createBuffersGL()
{
    unsigned int idBufPos = 0;
    unsigned int idBufTex = 1;

    for (int lod = 0; lod < 5; lod++)
    {
        glGenVertexArrays(1, &mVao[lod]);
        glBindVertexArray(mVao[lod]);

        // Créer VBO
        glGenBuffers(1, &mVbo[lod]);
        glBindBuffer(GL_ARRAY_BUFFER, mVbo[lod]);

        glEnableVertexAttribArray(idBufPos);
        glVertexAttribPointer(idBufPos, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0);

        glBufferData(GL_ARRAY_BUFFER, this->mLod[lod].vertices.size() * sizeof(Vertex), this->mLod[lod].vertices.data(),GL_DYNAMIC_DRAW);
        
        unsigned numFloat =3;
        
        glEnableVertexAttribArray(idBufTex);
        glVertexAttribPointer(idBufTex, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)(numFloat*sizeof(float)));
        

        // Créer EBO
        glGenBuffers(1, &mEbo[lod]);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mEbo[lod]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, this->mLod[lod].indices.size() * sizeof(unsigned int),
                     this->mLod[lod].indices.data(), GL_DYNAMIC_DRAW);

        glBindVertexArray(0);
    }
}

void Patch::generateLodVertices(std::vector<float> &heights, unsigned int width, unsigned int height)
{
    const float skirtDepth = 0.01f;
    const float textureScale = 20.0f;

    const int basePatchX = static_cast<int>(mPatchX) * PATCH_SIZE;
    const int basePatchZ = static_cast<int>(mPatchZ) * PATCH_SIZE;

    const float invXzFactor = 1.0f / mXzFactor;
    const float invTexWidth = textureScale / (static_cast<float>(width) * mXzFactor);
    const float invTexHeight = textureScale / (static_cast<float>(height) * mXzFactor);

    for (int k = 0; k < 5; ++k)
    {
        const int step = mLodSteps[k];
        const int innerResolution = (PATCH_SIZE / step) + 1;
        const int resolution = innerResolution + 2;
        const int totalVertices = resolution * resolution;

        auto &vertices = mLod[k].vertices;
        vertices.resize(totalVertices);

        int outIndex = 0;

        for (int localY = 0; localY < resolution; ++localY)
        {
            const int innerY = localY - 1;
            const int clampedY = std::clamp(innerY, 0, innerResolution - 1);

            const int worldZ = basePatchZ + innerY * step;
            int sampleZ = basePatchZ + clampedY * step;
            sampleZ = std::clamp(sampleZ, 0, static_cast<int>(height) - 1);

            const bool borderY = (localY == 0 || localY == resolution - 1);

            for (int localX = 0; localX < resolution; ++localX, ++outIndex)
            {
                const int innerX = localX - 1;
                const int clampedX = std::clamp(innerX, 0, innerResolution - 1);

                const int worldX = basePatchX + innerX * step;
                int sampleX = basePatchX + clampedX * step;
                sampleX = std::clamp(sampleX, 0, static_cast<int>(width) - 1);

                float heightValue = heights[sampleZ * static_cast<int>(width) + sampleX];

                if (borderY || localX == 0 || localX == resolution - 1)
                {
                    heightValue -= skirtDepth;
                }

                vertices[outIndex].position = glm::vec3(
                    static_cast<float>(worldX) * invXzFactor,
                    heightValue,
                    static_cast<float>(worldZ) * invXzFactor
                );

                vertices[outIndex].texture = glm::vec2(
                    static_cast<float>(worldX) * invTexWidth,
                    static_cast<float>(worldZ) * invTexHeight
                );
            }
        }
    }
}

void Patch::generateLodIndices(std::vector<float> &heights, unsigned int width, unsigned int height)
{
    for (int k = 0; k < 5; ++k)
    {
        mLod[k].indices.clear();

        int step = mLodSteps[k];

        int innerResolution = (PATCH_SIZE / step) + 1;
        int resolution = innerResolution + 2;

        int cellsPerRow = resolution - 1;

        mLod[k].indices.reserve(resolution * resolution * 6);

        for (int y = 0; y < cellsPerRow; y++)
        {
            for (int x = 0; x < cellsPerRow; x++)
            {
                int topLeft = y * resolution + x;
                int topRight = y * resolution + x + 1;
                int bottomLeft = (y + 1) * resolution + x;
                int bottomRight = (y + 1) * resolution + x + 1;

                // Triangle 1
                mLod[k].indices.push_back(topLeft);
                mLod[k].indices.push_back(bottomLeft);
                mLod[k].indices.push_back(topRight);

                // Triangle 2
                mLod[k].indices.push_back(topRight);
                mLod[k].indices.push_back(bottomLeft);
                mLod[k].indices.push_back(bottomRight);
            }
        }
    }
}

void Patch::render()
{
    int lodLevel = this->mLodLevel;
    glBindVertexArray(mVao[lodLevel]);
    glDrawElements(GL_TRIANGLES, mLod[lodLevel].indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

int Patch::chooseLod(glm::vec3 cameraPos, Frustrum *frustrum)
{
    float centreX = (mPatchX * PATCH_SIZE + PATCH_SIZE * 0.5f) / mXzFactor;
    float centreZ = (mPatchZ * PATCH_SIZE + PATCH_SIZE * 0.5f) / mXzFactor;
    float hauteurMoyenne = 0.5f;

    glm::vec3 centre(centreX, hauteurMoyenne, centreZ);
    bool inFrustrum = frustrum->isPatchInFrustum(centre, (PATCH_SIZE * (17.f)) / mXzFactor);

    if (!inFrustrum)
        return -1;

    float distance = glm::distance(cameraPos, centre);

    if (distance < 500.f)
    {
        return 0;
    }
    else if (distance < 700.f)
    {
        return 1;
    }
    else if (distance < 850.f)
    {
        return 2;
    }
    else if (distance < 950.f)
    {
        return 3;
    }
    else
    {
        return 4;
    }
}

int Patch::getLodLevel()
{
    return this->mLodLevel;
}

void Patch::setLodLevel(int level)
{
    this->mLodLevel = level;
}

int Patch::getNbPatchX()
{
    return this->mNbPatchX;
}

int Patch::getNbPatchZ()
{
    return this->mNbPatchZ;
}

void Patch::uploadSingleLodToGpu(int lodLevel)
{
    glBindVertexArray(mVao[lodLevel]);

    glBindBuffer(GL_ARRAY_BUFFER, mVbo[lodLevel]);
    glBufferSubData(
        GL_ARRAY_BUFFER,
        0,
        mLod[lodLevel].vertices.size() * sizeof(Vertex),
        mLod[lodLevel].vertices.data()
    );

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mEbo[lodLevel]);
    glBufferSubData(
        GL_ELEMENT_ARRAY_BUFFER,
        0,
        mLod[lodLevel].indices.size() * sizeof(unsigned int),
        mLod[lodLevel].indices.data()
    );

    glBindVertexArray(0);
}

void Patch::uploadLodToGpu()
{
    for (int lod = 0; lod < 5; ++lod)
    {
        uploadSingleLodToGpu(lod);
    }
}