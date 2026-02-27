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
        Lod lod[5];
        int lodSteps[5] = {1,4,8,16,32};
        GLuint vbo[5];  
        GLuint ebo[5];
        GLuint vao[5];
    
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

            for (int k=0;k<5;++k){
                lod[k].vertices.clear();
                int step = lodSteps[k];
                int resolution = (32 / step) + 1;  // 33, 17, ou 9

                lod[k].vertices.reserve(resolution*resolution*3);
                
                for (int localY = 0; localY < resolution; localY++) {
                    for (int localX = 0; localX < resolution; localX++) {
                        // Position globale
                        int worldX = patch_x * 32 + localX*step;
                        int worldZ = patch_z * 32 + localY*step;

                        lod[k].vertices.push_back((float)worldX/xzfactor);

                        if (worldX >= 0 && worldX < width && worldZ >= 0 && worldZ < height){
                            float h = heights[worldZ * width + worldX];
                            lod[k].vertices.push_back(h);
                        }else{
                            lod[k].vertices.push_back(0.f);
                        }
                        
                        lod[k].vertices.push_back(((float)worldZ/xzfactor));
                    }
                }
            }
        }

        void generate_lod_indices(std::vector<float>& heights,unsigned int width,unsigned int height){

            bool stitchRight = false;
            bool stitchLeft = false;
            bool stitchBot = false;
            bool stitchTop = false;
            for (int k=0;k<5;++k){
                lod[k].indices.clear();

                int step = lodSteps[k];
                int resolution = (32 / step) + 1;  // 33, 17, ou 9
                int cellsPerRow = resolution - 1;

                lod[k].indices.reserve(resolution*resolution*6);

                
                for (int y = 0; y < cellsPerRow; y++) {
                    for (int x = 0; x < cellsPerRow; x++) {
                        
                        if (stitchRight && x == cellsPerRow-1){
                            if (y % 2 == 0)
                            {
                                int topLeft     = y * resolution + x;
                                int bottomLeft  = (y+2) * resolution + x;
                                int midLeft     = (y+1) * resolution + x;

                                int topRight    = y * resolution + x + 1;
                                int bottomRight = (y+2) * resolution + x + 1;

                                // Grand triangle 1
                                lod[k].indices.push_back(topLeft);
                                lod[k].indices.push_back(midLeft);
                                lod[k].indices.push_back(topRight);

                                // Grand triangle 2
                                lod[k].indices.push_back(midLeft);
                                lod[k].indices.push_back(bottomRight);
                                lod[k].indices.push_back(topRight);

                                // Grand triangle 3
                                lod[k].indices.push_back(midLeft);
                                lod[k].indices.push_back(bottomLeft);
                                lod[k].indices.push_back(bottomRight);
                            }
                            continue;
                        }
                        
                        if(stitchLeft && x ==0){
                            if (y % 2 == 0 && y + 2 <= cellsPerRow)
                            {
                                int topRight     = y * resolution + x + 1;
                                int midRight     = (y+1) * resolution + x + 1;
                                int bottomRight  = (y+2) * resolution + x + 1;

                                int topLeft      = y * resolution + x;
                                int bottomLeft   = (y+2) * resolution + x;

                                lod[k].indices.push_back(topLeft);
                                lod[k].indices.push_back(midRight);
                                lod[k].indices.push_back(topRight);

                                lod[k].indices.push_back(topLeft);
                                lod[k].indices.push_back(bottomLeft);
                                lod[k].indices.push_back(midRight);

                                lod[k].indices.push_back(midRight);
                                lod[k].indices.push_back(bottomLeft);
                                lod[k].indices.push_back(bottomRight);
                            }
                            continue;
                        }

                        if (stitchTop && y == cellsPerRow - 1){
                            if (x % 2 == 0 && x + 2 <= cellsPerRow)
                            {
                                int leftTop     = y * resolution + x;
                                int midTop      = y * resolution + (x+1);
                                int rightTop    = y * resolution + (x+2);

                                int leftBottom  = (y+1) * resolution + x;
                                int rightBottom = (y+1) * resolution + (x+2);

                                lod[k].indices.push_back(leftTop);
                                lod[k].indices.push_back(leftBottom);
                                lod[k].indices.push_back(midTop);

                                lod[k].indices.push_back(midTop);
                                lod[k].indices.push_back(leftBottom);
                                lod[k].indices.push_back(rightBottom);

                                lod[k].indices.push_back(midTop);
                                lod[k].indices.push_back(rightBottom);
                                lod[k].indices.push_back(rightTop);
                            }
                            continue;
                        }

                        if (stitchBot && y==0){
                            if (x % 2 == 0 && x + 2 <= cellsPerRow)
                            {
                                int leftBottom   = y * resolution + x;
                                int midBottom    = y * resolution + (x+1);
                                int rightBottom  = y * resolution + (x+2);

                                int leftTop      = (y+1) * resolution + x;
                                int rightTop     = (y+1) * resolution + (x+2);

                                lod[k].indices.push_back(leftBottom);
                                lod[k].indices.push_back(midBottom);
                                lod[k].indices.push_back(leftTop);

                                lod[k].indices.push_back(midBottom);
                                lod[k].indices.push_back(rightTop);
                                lod[k].indices.push_back(leftTop);

                                lod[k].indices.push_back(midBottom);
                                lod[k].indices.push_back(rightBottom);
                                lod[k].indices.push_back(rightTop);
                            }
                        }
                        
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



        void render(float cameraX,float cameraZ){
            int l = chooseLod(cameraX,cameraZ);
            glBindVertexArray(vao[l]);
            glBindBuffer(GL_ARRAY_BUFFER, vbo[l]);
            glBufferSubData(GL_ARRAY_BUFFER,0,lod[l].vertices.size() * sizeof(float),lod[l].vertices.data());

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo[l]);            
            glDrawElements(GL_TRIANGLES, lod[l].indices.size(), GL_UNSIGNED_INT, 0);
        }

        int chooseLod(float cameraPosX,float cameraPosZ){

            int centerX = patch_x * 32 + 16;
            int centerZ = patch_z * 32 + 16;

            float dx = cameraPosX - centerX;
            float dz = cameraPosZ - centerZ;

            float dist2 = dx*dx + dz*dz;

            if (dist2 < 50.0f * 50.0f){
                return 0;
            }else if (dist2 < 150.0f * 150.0f){
                return 1;
            }else{
                return 2;
            }
        }

};

#endif