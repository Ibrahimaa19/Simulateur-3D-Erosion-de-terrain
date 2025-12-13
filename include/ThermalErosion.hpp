#include "Terrain.hpp"
#include <vector>

class ThermalErosion
{
public:

    void loadTerrainInfo(Terrain* terrain) {
        m_data  = terrain->get_data(); 
        m_height = terrain->get_terrain_height();
        m_width  = terrain->get_terrain_width();
    }
    
    void setTalusAngle(float talus) { talusAngle = talus; }
    void setTransferRate(float c)   { transferRate = c; }

    void step();

private:
    std::vector<float>* m_data = nullptr;
    int m_height = 0;
    int m_width = 0;
    float talusAngle = 45.f; //en degré
    float transferRate = 0.1f;

    float get_height(int i, int j) const {
        return (*m_data)[i * m_width + j];
    }

    float get_talus(){ return talusAngle;}
};

