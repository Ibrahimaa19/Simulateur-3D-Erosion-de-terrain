#include "Patch.hpp"

void Patch::setPatch(unsigned int x, unsigned int z, float xzFactor, unsigned int nbPatchX, unsigned int nbPatchZ)
{
    this->mPatchX = x;
    this->mPatchZ = z;
    this->mXzFactor = xzFactor;
    this->mNbPatchX = nbPatchX;
    this->mNbPatchZ = nbPatchZ;
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

        glBindVertexArray(idBufPos);
    }
}

void Patch::generateLodVertices(std::vector<float> &heights, unsigned int width, unsigned int height)
{
    float heightValue = 0.f;
    const float skirtDepth = 0.01f;
    

    for (int k = 0; k < 5; ++k)
    {

        mLod[k].vertices.clear();
        int step = mLodSteps[k];

        int innerResolution = (PATCH_SIZE / step) + 1;

        // résolution étendue (+2 pour skirt)
        int resolution = innerResolution + 2;

        mLod[k].vertices.reserve(resolution * resolution);

        for (int localY = 0; localY < resolution; localY++)
        {
            for (int localX = 0; localX < resolution; localX++)
            {
                // Coordonnées locales internes
                int innerX = localX - 1;
                int innerY = localY - 1;

                // Clamp pour rester sur le bord interne
                int clampedX = std::clamp(innerX, 0, innerResolution - 1);
                int clampedY = std::clamp(innerY, 0, innerResolution - 1);

                // Position monde correcte (patch de 32)
                int worldX = mPatchX * PATCH_SIZE + innerX * step;
                int worldZ = mPatchZ * PATCH_SIZE + innerY * step;

                // lecture dans le vecteur heightmap
                int sampleX = mPatchX * PATCH_SIZE + clampedX * step;
                int sampleZ = mPatchZ * PATCH_SIZE + clampedY * step;

                sampleX = std::clamp(sampleX, 0, (int)width - 1);
                sampleZ = std::clamp(sampleZ, 0, (int)height - 1);

                heightValue = heights[sampleZ * width + sampleX];

                if (localX == 0 || localX == resolution - 1 || localY == 0 || localY == resolution - 1)
                {
                    heightValue -= skirtDepth;
                }


                glm::vec3 vecPos(((float)worldX / mXzFactor),heightValue,((float)worldZ / mXzFactor));
                glm::vec2 vecTex(0.,0.);

                mLod[k].vertices.push_back(Vertex(vecPos,vecTex));
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
    glBindBuffer(GL_ARRAY_BUFFER, mVbo[lodLevel]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, mLod[lodLevel].vertices.size() * sizeof(Vertex), mLod[lodLevel].vertices.data());

    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mEbo[lodLevel]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mLod[lodLevel].indices.size() * sizeof(unsigned int),
                 mLod[lodLevel].indices.data(), GL_DYNAMIC_DRAW);

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