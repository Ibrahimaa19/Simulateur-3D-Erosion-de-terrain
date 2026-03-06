#include "Patch.hpp"

void Patch::set_patch(unsigned int x,unsigned int z,float xz,unsigned int nb_p_x,unsigned int nb_p_z,Frustrum* mFrus){
    this->patch_x = x;
    this->patch_z = z;
    this->xzfactor = xz;
    this->nb_patch_x = nb_p_x;
    this->nb_patch_z = nb_p_z;
    this->frustrum = mFrus;
}

void Patch::addNeightbour(Patch* p){
    neightbour.push_back(p);
}

std::vector<Patch*> Patch::getNeightbour(){
    return this->neightbour;
}


int Patch::getNeightbourLodLevel(int i){
    return neightbour[i]->lodLevel;
}

unsigned int Patch::get_patch_x(){
    return patch_x;
}

unsigned int Patch::get_patch_z(){
    return patch_z;
}

GLuint Patch::get_vbo(int lod){
    return vbo[lod];
}

void Patch::creerBuffersGL() {

    for (int lod = 0; lod < 5; lod++) {

        glGenVertexArrays(1, &vao[lod]);
        glBindVertexArray(vao[lod]);

        // Créer VBO
        glGenBuffers(1, &vbo[lod]);
        glBindBuffer(GL_ARRAY_BUFFER, vbo[lod]);
        glBufferData(GL_ARRAY_BUFFER,this->lod[lod].vertices.size() * sizeof(float),this->lod[lod].vertices.data(),GL_DYNAMIC_DRAW);
        
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // Créer EBO
        glGenBuffers(1, &ebo[lod]);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo[lod]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        this->lod[lod].indices.size() * sizeof(unsigned int),
        this->lod[lod].indices.data(),
        GL_DYNAMIC_DRAW);

        glBindVertexArray(0);
    }
}


void Patch::generate_lod_vertices(std::vector<float>& heights,unsigned int width,unsigned int height){
    float heightValue = 0.f;
    const float skirtDepth = 0.01f;

    for (int k=0;k<5;++k){
        lod[k].vertices.clear();
        int step = lodSteps[k];

        int innerResolution = (PATCH_SIZE / step) + 1;

        // résolution étendue (+2 pour skirt)
        int resolution = innerResolution + 2;

        lod[k].vertices.reserve(resolution*resolution*3);
        
        for (int localY = 0; localY < resolution; localY++) {
            for (int localX = 0; localX < resolution; localX++) {

                // Coordonnées locales internes
                int innerX = localX - 1;
                int innerY = localY - 1;

                // Clamp pour rester sur le bord interne
                int clampedX = std::clamp(innerX, 0, innerResolution - 1);
                int clampedY = std::clamp(innerY, 0, innerResolution - 1);

                // Position monde correcte (patch de 32)
                int worldX = patch_x * PATCH_SIZE + innerX * step;
                int worldZ = patch_z * PATCH_SIZE + innerY * step;

                // lecture dans le vecteur heightmap
                int sampleX = patch_x * PATCH_SIZE + clampedX * step;
                int sampleZ = patch_z * PATCH_SIZE + clampedY * step;

                sampleX = std::clamp(sampleX, 0, (int)width - 1);
                sampleZ = std::clamp(sampleZ, 0, (int)height - 1);

                heightValue = heights[sampleZ * width + sampleX];


                if (localX == 0 || localX == resolution - 1 || localY == 0 || localY == resolution - 1){

                    heightValue -= skirtDepth;
                }
                
        
                lod[k].vertices.push_back((float)worldX/xzfactor);
                lod[k].vertices.push_back(heightValue);
                lod[k].vertices.push_back(((float)worldZ/xzfactor));
            }
        }
    }
}

void Patch::generate_lod_indices(std::vector<float>& heights,unsigned int width,unsigned int height){

    for (int k=0;k<5;++k){
        lod[k].indices.clear();

        int step = lodSteps[k];
        

        int innerResolution = (PATCH_SIZE / step) + 1;
        int resolution = innerResolution + 2;

        int cellsPerRow = resolution - 1;

        lod[k].indices.reserve(resolution*resolution*6);

        for (int y = 0; y < cellsPerRow; y++) {
            for (int x = 0; x < cellsPerRow; x++) {

                int topLeft = y * resolution + x;
                int topRight = y * resolution + x + 1;
                int bottomLeft = (y+1) * resolution + x;
                int bottomRight = (y+1) * resolution + x + 1;
                
                // Triangle 1
                lod[k].indices.push_back((topLeft));
                lod[k].indices.push_back((bottomLeft));
                lod[k].indices.push_back((topRight));
                
                // Triangle 2
                lod[k].indices.push_back((topRight));
                lod[k].indices.push_back((bottomLeft));
                lod[k].indices.push_back((bottomRight));

            }
        }
    }
}

void Patch::render(){
    int l = this->lodLevel;
    glBindVertexArray(vao[l]);
    glBindBuffer(GL_ARRAY_BUFFER, vbo[l]);
    glBufferSubData(GL_ARRAY_BUFFER,0,lod[l].vertices.size() * sizeof(float),lod[l].vertices.data());


    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo[l]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,lod[l].indices.size() * sizeof(unsigned int),lod[l].indices.data(),GL_DYNAMIC_DRAW);

    glDrawElements(GL_TRIANGLES, lod[l].indices.size(), GL_UNSIGNED_INT, 0);

    glBindVertexArray(0);
}

int Patch::chooseLod(glm::vec3 cameraPos){

    float centreX = (patch_x * PATCH_SIZE + PATCH_SIZE*0.5f)/xzfactor;
    float centreZ = (patch_z * PATCH_SIZE + PATCH_SIZE*0.5f)/xzfactor;
    float hauteurMoyenne = 0.5f;

    glm::vec3 centre(centreX, hauteurMoyenne, centreZ);
    bool inFrustrum = frustrum->patchDansFrustum(centre,PATCH_SIZE*(13.f/xzfactor));

    if (!inFrustrum)
        return -1;

    float distance = glm::distance(cameraPos,centre);

    //std::cout << distance << std::endl;
    if (distance < 500.f){
        return 0;
    }else if (distance < 700.f){
        return 1;
    }else if (distance < 850.f){
        return 2;
    }else if (distance < 950.f){
        return 3;
    }else{
        return 4;
    }
        
}

int Patch::getLodLevel(){
    return this->lodLevel;
}

void Patch::setLodLevel(int l){
    this->lodLevel = l;
}

int Patch::getNbPatchX(){
    return this->nb_patch_x;
}
int Patch::getNbPatchZ(){
    return this->nb_patch_z;
}