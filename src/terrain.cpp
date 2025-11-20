#include "terrain.hpp"


void Terrain::load_vectices(){
    vertices.clear();
    for (int z = 0; z < height; z++) {
        for (int x = 0; x < width; x++) {
            bool isBorder = (x < borderSize || x >= width - borderSize || z < borderSize || z >= height - borderSize); // Si on est dans le bordure, alors isBorder devient true
            int index = z * width + x;
            float y = data[index];
            
            
            vertices.push_back((float)x/100.f);  
            if(isBorder){
                vertices.push_back(0.0f); // en bordure on aplatit
            }else{
                vertices.push_back(y);
            }
                        
            vertices.push_back((float)z/100.f);        
        }
    }
}

void Terrain::load_incides(){
    indices.clear();
    for (int z = 0; z < height - 1; z++) {
        for (int x = 0; x < width - 1; x++) {
            
            int hautG = z * width + x;
            int hautD = hautG + 1;
            int basG = (z + 1) * width + x;
            int basD = basG + 1;
            
            // Premier triangle
            indices.push_back(hautG);
            indices.push_back(basG);
            indices.push_back(hautD);
            
            // Deuxieme triangle
            indices.push_back(hautD);
            indices.push_back(basG);
            indices.push_back(basD);
        }
    }
}

void Terrain::setup_terrain(GLuint &VAO, GLuint &VBO, GLuint &EBO){
    // Créer un VAO
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    

    // Remplis de le buffers VBO de données
	glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, this->vertices.size() * sizeof(float), 
                this->vertices.data(), GL_DYNAMIC_DRAW);
    

    // Setup de VAO, comment lire le VBO
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    

    // Remplis de buffer d'indices
	glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, this->indices.size() * sizeof(unsigned int),
                 this->indices.data(), GL_STATIC_DRAW);
    
    // Lie le VAO au contexte courant, on va maintenant utiliser ce VAO
    glBindVertexArray(0);
    
}

int Terrain::getHeight(int i, int j) const{
    return data[i * width + j];
}

void Terrain::setHeight(int i, int j, float value){
    data[i * width + j] += value;
}

bool Terrain::inside(int i, int j) const {
    return (i >= 0 && i < height && j >= 0 && j < width);
}