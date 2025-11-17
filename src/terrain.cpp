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

