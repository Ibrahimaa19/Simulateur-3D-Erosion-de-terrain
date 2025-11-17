#include "terrain.hpp"


void Terrain::load_vectices(){
    vertices.clear();
    for (int z = 0; z < height; z++) {
        for (int x = 0; x < width; x++) {
            bool isBorder = (x < borderSize || x >= width - borderSize || z < borderSize || z >= height - borderSize);
            int index = z * width + x;
            float y = data[index] * 10.0f;
            
            
            vertices.push_back((float)x);  
            if(isBorder){
                vertices.push_back(0.0f); 
            }else{
                vertices.push_back(y);
            }
                        
            vertices.push_back((float)z);        
        }
    }
}

void Terrain::load_incides(){
    indices.clear();
    for (int z = 0; z < height - 1; z++) {
        for (int x = 0; x < width - 1; x++) {
            
            int topLeft = z * width + x;
            int topRight = topLeft + 1;
            int bottomLeft = (z + 1) * width + x;
            int bottomRight = bottomLeft + 1;
            
            
            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);
            
            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }
}

