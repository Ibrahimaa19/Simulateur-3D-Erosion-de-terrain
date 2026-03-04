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
    int nb_patch_x = std::ceil(width/32);
    int nb_patch_z = std::ceil(height/32);

    std::cout << "nb_patch_x : "<< nb_patch_x << " ,nb_patch_z : " << nb_patch_z << std::endl;
    for(int i=0;i<nb_patch_x;++i){
        for(int j=0;j<nb_patch_z;++j){
            std::unique_ptr<Patch> p = std::make_unique<Patch>();
            p->set_patch(i,j,xzfactor,nb_patch_x,nb_patch_z);
            this->patches.push_back(std::move(p));
        }
    }

    int width = nb_patch_x*nb_patch_z;

    for(int i=0;i<nb_patch_x;++i){
        for(int j=0;j<nb_patch_z;++j){
            int top = (i+1)*nb_patch_x + j;
            int bot = (i-1)*nb_patch_x + j;
            int right = (i)*nb_patch_x + (j+1);
            int left = (i+1)*nb_patch_x + (j-1);
            int cur = (i)*nb_patch_x + j;

            if(top >=0 && top < width)
                this->patches[cur]->addNeightbour(patches[top].get());

            if(bot >=0 && bot < width)
                this->patches[cur]->addNeightbour(patches[bot].get());

            if(right >=0 && right < width)
                this->patches[cur]->addNeightbour(patches[right].get());

            if(left >=0 && left < width)
                this->patches[cur]->addNeightbour(patches[left].get());

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

    for(int i =0;i<patches.size();++i){
        patches[i]->generate_lod_vertices(data,width,height);
    }
}

void Terrain::load_incides_lod(){
    for(int i =0;i<patches.size();++i){
        patches[i]->generate_lod_indices(data,width,height);
    }
}

void Terrain::setup_terrain_lod(GLuint &VAO, GLuint &VBO, GLuint &EBO){
    this->load_incides_lod();
	this->load_vectices_lod();

    for(int i =0;i<patches.size();++i){
        patches[i]->creerBuffersGL();
    }    
}

/*
    Verifie que deux patches avec des lod différents, ont une un niveau de différence maximum, utile dans le cadres des skirts
*/
void Terrain::correct_lod(){
    bool changed;
    int temp = 0;

    do
    {
        changed = false;

        for (int i =0;i<patches.size();++i)
        {

            for (int j =0;j<patches[i]->getNeightbour().size();++j)
            {

                if (patches[i]->getLodLevel() > patches[i]->getNeightbourLodLevel(j) + 1)
                {
                    temp = patches[i]->getNeightbourLodLevel(j) + 1;
                    patches[i]->setLodLevel(temp);
                    changed = true;
                }

                if (patches[i]->getNeightbourLodLevel(j) > patches[i]->getLodLevel() + 1)
                {
                    temp = patches[i]->getLodLevel() + 1;
                    patches[i]->getNeightbour()[j]->setLodLevel(temp);
                    changed = true;
                }
            }
        }

    } while(changed);

}

/*
    On affiche le terrain en prenant en compte les différents niveau de lod pour chaque patches
*/
void Terrain::renderer_lod(const glm::vec3& cameraPos){
    int temp = 0;
    int index = 0;
    std::vector<int> toUpdate;

    for(int i =0;i<patches.size();++i){
        temp = patches[i]->chooseLod(cameraPos.x,cameraPos.z);
        patches[i]->setLodLevel(temp);
    }

    correct_lod();

    for(int i =0;i<patches.size();++i){
        patches[i]->render();
    }

}


void Terrain::update_vertices_gpu_lod()
{
    load_vectices_lod();
    
}