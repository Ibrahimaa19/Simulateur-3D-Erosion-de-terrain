#ifndef PATCH_H
#define PATCH_H

#include <GL/glew.h>
#include <algorithm>
#include <vector>
#include <iostream>
#include "Frustrum.hpp"

#define PATCH_SIZE 32

struct Lod{
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
};

class Patch{
    private:
        float xzfactor;
        unsigned int patch_size = PATCH_SIZE;
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

        void set_patch(unsigned int x,unsigned int z,float xz,unsigned int nb_p_x,unsigned int nb_p_z);

        void addNeightbour(Patch* p);

        std::vector<Patch*> getNeightbour();

        int getNeightbourLodLevel(int i);

        unsigned int get_patch_x();

        unsigned int get_patch_z();

        GLuint get_vbo(int lod);

        void creerBuffersGL();

        void generate_lod_vertices(std::vector<float>& heights,unsigned int width,unsigned int height);

        void generate_lod_indices(std::vector<float>& heights,unsigned int width,unsigned int height);

        void render();

        int chooseLod(glm::vec3 cameraPos,Frustrum* frustrum);

        int getLodLevel();

        void setLodLevel(int l);

        int getNbPatchX();
        int getNbPatchZ();

};

#endif