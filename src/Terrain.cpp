#include "Terrain.hpp"
#include "ThermalErosion.hpp"
#include <fstream>


void Terrain::load_terrain (const char* image_path,float yfactor,float xzfactor){
    int t_channels;

    unsigned char* image = stbi_load(image_path, &this->width, &this->height, &t_channels, 1);

    if (image){
        std::cout << "Loaded heightmap of size " << height << " x " << width << std::endl;
    }else{
        std::cout << "Failed to load texture" << std::endl;
    }

    this->data.resize(height*width);

    this->borderSize = 10;
    this->cellSpacing = 1;
    this->yfactor = yfactor;
    this->xzfactor = xzfactor;
    

    for (int i = 0; i < width * height; i++) {
        this->data[i] = image[i] / 255.0f;
    }

    this->max_height = *std::max_element(data.begin(),data.end());
    this->min_height = *std::min_element(data.begin(),data.end());

    stbi_image_free(image);

    create_patches();

}

void Terrain::create_patches(){
    for(int i=0;i<32;++i){
        for(int j=0;j<32;++j){
            this->patches[i][j].set_patch(i,j,xzfactor);
        }
    }
}

void Terrain::load_vectices(){
    vertices.clear();
    for (int z = 0; z < height; z++) {
        for (int x = 0; x < width; x++) {
            bool isBorder = (x < borderSize || x >= width - borderSize || z < borderSize || z >= height - borderSize); // Si on est dans le bordure, alors isBorder devient true
            int index = z * width + x;
            float y = data[index]*yfactor;
            
            
            vertices.push_back((float)x/xzfactor);  
            if(isBorder){
                vertices.push_back(0.0f); // en bordure on aplatit
            }else{
                vertices.push_back(y);
            }
                        
            vertices.push_back((float)z/xzfactor);        
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
    this->load_incides();
	this->load_vectices();

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

void Terrain::update_vertices() {
    load_vectices();
}

void Terrain::renderer(){
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
}

bool Terrain::inside(int i, int j) const {
    return (i >= 0 && i < height && j >= 0 && j < width);
}

void Terrain::set_data(int i, float value){
    this->data[i] += value;
}

std::vector<float>* Terrain::get_data(){
    return &(this->data);
}
/*
std::vector<float> Terrain::get_data(){
    return this->data;
}*/

int Terrain::get_indices_size() const{
    return this->indices.size();
};

int Terrain::get_vertices_size() const{
    return this->vertices.size();
}

void Terrain::update_vertices_gpu(GLuint VBO)
{
    load_vectices();

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferSubData(GL_ARRAY_BUFFER,
                    0,
                    vertices.size() * sizeof(float),
                    vertices.data());
}


void Terrain::load_vectices_lod(){

    for(int i =0;i<32;++i){
        for(int j =0;j<32;++j){
            patches[i][j].generate_lod_vertices(data,width);
        }
    }

}

void Terrain::load_incides_lod(){
    
    for (int y = 0; y < 32; y++) {
        for (int x = 0; x < 32; x++) {
            int topLeft = y * 33 + x;
            int topRight = y * 33 + x + 1;
            int bottomLeft = (y+1) * 33 + x;
            int bottomRight = (y+1) * 33 + x + 1;
            
            indices_lod[0].push_back(topLeft);
            indices_lod[0].push_back(bottomLeft);
            indices_lod[0].push_back(topRight);
            indices_lod[0].push_back(topRight);
            indices_lod[0].push_back(bottomLeft);
            indices_lod[0].push_back(bottomRight);
        }
    }
    
    // LOD1 (17×17)
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            int topLeft = y * 17 + x;
            int topRight = y * 17 + x + 1;
            int bottomLeft = (y+1) * 17 + x;
            int bottomRight = (y+1) * 17 + x + 1;
            
            indices_lod[1].push_back(topLeft);
            indices_lod[1].push_back(bottomLeft);
            indices_lod[1].push_back(topRight);
            indices_lod[1].push_back(topRight);
            indices_lod[1].push_back(bottomLeft);
            indices_lod[1].push_back(bottomRight);
        }
    }
    
    // LOD2 (9×9)
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int topLeft = y * 9 + x;
            int topRight = y * 9 + x + 1;
            int bottomLeft = (y+1) * 9 + x;
            int bottomRight = (y+1) * 9 + x + 1;
            
            indices_lod[2].push_back(topLeft);
            indices_lod[2].push_back(bottomLeft);
            indices_lod[2].push_back(topRight);
            indices_lod[2].push_back(topRight);
            indices_lod[2].push_back(bottomLeft);
            indices_lod[2].push_back(bottomRight);
        }
    }
    
}

void Terrain::setup_terrain_lod(GLuint &VAO, GLuint &VBO, GLuint &EBO){
    this->load_incides_lod();
	this->load_vectices_lod();
    
    for(int i=0;i<32;++i){
        for(int j=0;j<32;++j){
            this->patches[i][j].creerBuffersGL();
        }
    }
}

void Terrain::renderer_lod(){
    for(int i=0;i<32;++i){
        for(int j=0;j<32;++j){
            this->patches[i][j].render();
        }
    }
}
