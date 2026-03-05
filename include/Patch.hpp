#ifndef PATCH_H
#define PATCH_H

#include <vector>
#include <iostream>

struct Lod{
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
};

class Patch{
    private:
        float xzfactor;
        unsigned int patch_size = 32;
        unsigned int patch_x,patch_z;
        unsigned int nb_patch_x,nb_patch_z;
        Lod lod[5];
        int lodSteps[5] = {1,2,4,8,16};
        GLuint vbo[5];  
        GLuint ebo[5];
        GLuint vao[5];
        int lodLevel;

        std::vector<Patch*> neightbour;
    
    public:

        void set_patch(unsigned int x,unsigned int z,float xz,unsigned int nb_p_x,unsigned int nb_p_z){
            this->patch_x = x;
            this->patch_z = z;
            this->xzfactor = xz;
            this->nb_patch_x = nb_p_x;
            this->nb_patch_z = nb_p_z;
        }

        void addNeightbour(Patch* p){
            neightbour.push_back(p);
        }

        int getNeightbourNumber(){
            return neightbour.size();
        }

        std::vector<Patch*> getNeightbour(){
            return this->neightbour;
        }

        int getNeightbourLodLevel(int i){
            return this->neightbour[i]->lodLevel;
        }

        unsigned int get_patch_x(){
            return this->patch_x;
        }

        unsigned int get_patch_z(){
            return this->patch_z;
        }

        GLuint get_vbo(int lod){
            return this->vbo[lod];
        }


        void creerBuffersGL() {

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

        void generate_lod_vertices(std::vector<float>& heights,unsigned int width,unsigned int height){
            float heightValue = 0.f;
            const float skirtDepth = 0.01f;

            for (int k=0;k<5;++k){
                lod[k].vertices.clear();
                int step = lodSteps[k];

                int innerResolution = (32 / step) + 1;

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
                        int worldX = patch_x * 32 + innerX * step;
                        int worldZ = patch_z * 32 + innerY * step;

                        // lecture dans le vecteur heightmap
                        int sampleX = patch_x * 32 + clampedX * step;
                        int sampleZ = patch_z * 32 + clampedY * step;

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

        void generate_lod_indices(std::vector<float>& heights,unsigned int width,unsigned int height){

            for (int k=0;k<5;++k){
                lod[k].indices.clear();

                int step = lodSteps[k];
                

                int innerResolution = (32 / step) + 1;
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

        void render(){
            int l = this->lodLevel;
            glBindVertexArray(vao[l]);
            glBindBuffer(GL_ARRAY_BUFFER, vbo[l]);
            glBufferSubData(GL_ARRAY_BUFFER,0,lod[l].vertices.size() * sizeof(float),lod[l].vertices.data());


            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo[l]);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER,lod[l].indices.size() * sizeof(unsigned int),lod[l].indices.data(),GL_DYNAMIC_DRAW);

            glDrawElements(GL_TRIANGLES, lod[l].indices.size(), GL_UNSIGNED_INT, 0);

            glBindVertexArray(0);
        }

        int chooseLod(float cameraPosX,float cameraPosZ){

            int centerX = patch_x * 32 + 16;
            int centerZ = patch_z * 32 + 16;

            float dx = cameraPosX - centerX;
            float dz = cameraPosZ - centerZ;

            float dist2 = dx*dx + dz*dz;

            if (dist2 < 50.0f * 50.0f){
                return 0;
            }else if (dist2 < 100.0f * 100.0f){
                return 1;
            }else if (dist2 < 150.0f * 150.0f){
                return 2;
            }else if (dist2 < 200.0f * 200.0f){
                return 3;
            }else{
                return 4;
            }
        }

        int getLodLevel(){
            return this->lodLevel;
        }

        void setLodLevel(int l){
            this->lodLevel = l;
        }

        int getNbPatchX(){
            return this->nb_patch_x;
        }
        int getNbPatchZ(){
            return this->nb_patch_z;
        }

};

#endif