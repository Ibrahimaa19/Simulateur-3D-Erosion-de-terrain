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
        Lod lod[3];
        int lodSteps[3] = {1,2,4};
        GLuint vbo;  
        GLuint ebo;
        GLuint vao;
    
    public:

        void set_patch(unsigned int x,unsigned int z,float xz){
            this->patch_x = x;
            this->patch_z = z;
            this->xzfactor = xz;
        }

        unsigned int get_patch_x(){
            return this->patch_x;
        }

        unsigned int get_patch_z(){
            return this->patch_z;
        }


        void creerBuffersGL() {
            for (int lod = 0; lod < 1; lod++) {

                glGenVertexArrays(1, &vao);
                glBindVertexArray(vao);

                // Créer VBO
                glGenBuffers(1, &vbo);
                glBindBuffer(GL_ARRAY_BUFFER, vbo);
                glBufferData(GL_ARRAY_BUFFER,this->lod[lod].vertices.size() * sizeof(float),this->lod[lod].vertices.data(),GL_DYNAMIC_DRAW);
                
                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
                glEnableVertexAttribArray(0);

                // Créer EBO
                glGenBuffers(1, &ebo);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                this->lod[lod].indices.size() * sizeof(unsigned int),
                this->lod[lod].indices.data(),
                GL_DYNAMIC_DRAW);

                glBindVertexArray(0);
            }
        }

        void generate_lod_vertices(std::vector<float>& heights,unsigned int width){

            for (int k=0;k<1;++k){
                int step = lodSteps[k];
                int resolution = (32 / step) + 1;  // 33, 17, ou 9

                //lod[k].vertices.resize(resolution*resolution*3);

                //std::cout << "resolution : " << resolution << std::endl;
                
                for (int localY = 0; localY < resolution; localY++) {
                    for (int localX = 0; localX < resolution; localX++) {
                        // Position globale
                        int worldX = patch_x * 32 + localX;
                        int worldZ = patch_z * 32 + localY;
                        
                        //std::cout << "patch[" << patch_x << patch_z << "] wordlz:" << worldZ << " width:" << width << " worldx:"  << worldX << " =" << worldZ * width + worldX << std::endl;
                        float h = heights[worldZ * width + worldX];
                        
                        lod[k].vertices.push_back((float)worldX/xzfactor);
                        lod[k].vertices.push_back(h);
                        lod[k].vertices.push_back((float)worldZ/xzfactor);
                    }
                }


                for (int y = 0; y < 32; y++) {
                    for (int x = 0; x < 32; x++) {
                        int topLeft = y * resolution + x;
                        int topRight = y * resolution + x + 1;
                        int bottomLeft = (y+1) * resolution + x;
                        int bottomRight = (y+1) * resolution + x + 1;
                        
                        // Triangle 1
                        lod[k].indices.push_back(topLeft);
                        lod[k].indices.push_back(bottomLeft);
                        lod[k].indices.push_back(topRight);
                        
                        // Triangle 2
                        lod[k].indices.push_back(topRight);
                        lod[k].indices.push_back(bottomLeft);
                        lod[k].indices.push_back(bottomRight);
                    }
                }
            }

  
            
        }



        void render(){
            glBindVertexArray(vao);

            std::cout << "draw patch" << get_patch_x() << "," << get_patch_z() << " , vbo:" << vbo << " vao:" << vao << " ebo:"<< ebo << std::endl;
            glDrawElements(GL_TRIANGLES, lod[0].indices.size(), GL_UNSIGNED_INT, 0);
        }

};

#endif