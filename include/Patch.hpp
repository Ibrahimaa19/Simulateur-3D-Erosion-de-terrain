#ifndef PATCH_H
#define PATCH_H

#include <vector>
#include <iostream>

struct Lod{
    std::vector<float> vertices;
    std::vector<float> indices;
};

class Patch{
    private:
        unsigned int patch_size = 32;
        unsigned int patch_x,patch_z;
        Lod lod[3];
        int lodSteps[3] = {1,2,4};
    
    public:

        void set_patch(unsigned int x,unsigned int z){
            this->patch_x = x;
            this->patch_z = z;
        }

        unsigned int get_patch_x(){
            return this->patch_x;
        }

        unsigned int get_patch_z(){
            return this->patch_z;
        }

        void generate_lod_vertices(std::vector<float>& heights,unsigned int width){

            for (int k=0;k<3;++k){
                int step = lodSteps[k];
                int resolution = (32 / step) + 1;  // 33, 17, ou 9

                lod[k].vertices.resize(resolution*resolution);

                
                for (int localY = 0; localY < resolution; localY++) {
                    for (int localX = 0; localX < resolution; localX++) {
                        // Position globale
                        int worldX = patch_x * 32 + localX;
                        int worldZ = patch_z * 32 + localY;
                        
                        std::cout << "patch" << patch_x << patch_z << " " << worldZ << " " << width << " "  << worldX << " =" << worldZ * width + worldX << std::endl;
                        float h = heights[0];
                        
                        lod[k].vertices.push_back((float)worldX);
                        lod[k].vertices.push_back(h);
                        lod[k].vertices.push_back((float)worldZ);
                    }
                }
            }
        }

        void render(){
            
        }

};

#endif