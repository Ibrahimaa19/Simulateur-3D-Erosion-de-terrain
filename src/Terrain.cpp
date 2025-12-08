#include "Terrain.hpp"
#include "ThermalErosion.hpp"


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
}

void Terrain::load_vectices(){
    
    // Génére positions des points en fonction du LOD
    for(int k =0;k<4;k++){
        vertices[k].clear();
        size_t step = lodSteps[k];

        for (int z = 0; z < height; z+=step) {
            for (int x = 0; x < width; x+=step) {
                bool isBorder = (x < borderSize || x >= width - borderSize || z < borderSize || z >= height - borderSize); // Si on est dans le bordure, alors isBorder devient true
                int index = z * width + x;
                float y = data[index]*yfactor;
                
                
                vertices[k].push_back((float)x/xzfactor);  
                if(isBorder){
                    vertices[k].push_back(0.0f); // en bordure on aplatit
                }else{
                    vertices[k].push_back(y);
                }
                            
                vertices[k].push_back((float)z/xzfactor);        
            }
        }

    }

}

void Terrain::load_incides(){
    
    for(int k =0;k<4;k++){
        indices[k].clear();
        size_t step = lodSteps[k];

        // On définie height et width en fonction du lod
        int gridWidth = (width - 1) / step + 1;
        int gridHeight = (height - 1) / step + 1;
        
        for (int z = 0; z < gridHeight - 1; z++) {
            for (int x = 0; x < gridWidth - 1; x++) {
                int hautG = z * width + x;
                int hautD = hautG + 1;
                int basG = (z + 1) * width + x;
                int basD = basG + 1;
                
                // Premier triangle
                indices[k].push_back(hautG);
                indices[k].push_back(basG);
                indices[k].push_back(hautD);
                
                // Deuxieme triangle
                indices[k].push_back(hautD);
                indices[k].push_back(basG);
                indices[k].push_back(basD);
            }
        }
    }
    
}

void Terrain::setup_terrain(GLuint *VAO, GLuint *VBO, GLuint *EBO){
    this->load_incides();
	this->load_vectices();


    glGenVertexArrays(4, VAO);
    glGenBuffers(4, VBO);
    glGenBuffers(4, EBO);

    for (int lod = 0; lod < 4; lod++) {
        // Créer un VAO
        glBindVertexArray(VAO[lod]);
        
        // Remplis de le buffers VBO de données
        glBindBuffer(GL_ARRAY_BUFFER, VBO[lod]);
        glBufferData(GL_ARRAY_BUFFER, 
            vertices[lod].size() * sizeof(float), 
            vertices[lod].data(), 
            GL_STATIC_DRAW);
        
        // Setup de VAO, comment lire le VBO
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
        // Remplis de buffer d'indices

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO[lod]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, 
            indices[lod].size() * sizeof(unsigned int), 
            indices[lod].data(), 
            GL_STATIC_DRAW);
        
        // Lie le VAO au contexte courant, on va maintenant utiliser ce VAO
        glBindVertexArray(0);
    }
    
}

void Terrain::renderer(){
    glDrawElements(GL_TRIANGLES, indices[0].size(), GL_UNSIGNED_INT, 0);
}

bool Terrain::inside(int i, int j) const {
    return (i >= 0 && i < height && j >= 0 && j < width);
}

void Terrain::set_data(int i, float value){
    this->data[i] += value;
}

std::vector<float> Terrain::get_data(){
    return this->data;
}

int Terrain::get_indices_size() const{
    return this->indices[0].size();
};

int Terrain::get_vertices_size() const{
    return this->vertices[0].size();
};

float Terrain::get_lod_distance(int i) const{
    return lodDistances[i];
}

float Terrain::get_xz() const{
    return this->xzfactor;
}