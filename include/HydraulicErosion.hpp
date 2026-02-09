#ifndef HYDRAULIC_EROSION_HPP
#define HYDRAULIC_EROSION_HPP

#include "Terrain.hpp"
#include <vector>

class HydraulicErosion {
private:
    Terrain* mTerrain = nullptr;
    std::vector<float> mWater;
    std::vector<float> mSediment;
    
    int mWidth = 0;
    int mHeight = 0;
    
    // Paramètres (valeurs douces)
    float mRainRate = 0.001f;
    float mEvaporationRate = 0.02f;
    float mSolubility = 0.05f;
    float mSedimentCapacity = 0.05f;
    float mMinSlope = 0.02f;
    
    // Méthodes internes
    void addRain();
    void computeFlow();
    void transportSediment();
    void evaporate();
    
public:
    HydraulicErosion();
    
    void loadTerrainInfo(Terrain* terrain);
    int step();  // Retourne 1 si exécuté
    
    // Setters
    void setRainRate(float rate) { mRainRate = rate; }
    void setEvaporationRate(float rate) { mEvaporationRate = rate; }
    void setSolubility(float s) { mSolubility = s; }
    void setSedimentCapacity(float c) { mSedimentCapacity = c; }
    void setMinSlope(float s) { mMinSlope = s; }
    
    // Pour la visualisation
    const std::vector<float>& getWaterData() const { return mWater; }
    float getWaterHeightAt(int x, int y) const {
        if (!mTerrain || x < 0 || x >= mWidth || y < 0 || y >= mHeight) 
            return 0.0f;
        int idx = y * mWidth + x;
        return mWater[idx];
    }
};

#endif