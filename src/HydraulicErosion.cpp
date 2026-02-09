#include "HydraulicErosion.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>

HydraulicErosion::HydraulicErosion() {}

void HydraulicErosion::loadTerrainInfo(Terrain* terrain) {
    mTerrain = terrain;
    if (!mTerrain) return;
    
    mWidth = terrain->get_terrain_width();
    mHeight = terrain->get_terrain_height();
    
    mWater.resize(mWidth * mHeight, 0.0f);
    mSediment.resize(mWidth * mHeight, 0.0f);
}

void HydraulicErosion::addRain() {
    for (float& w : mWater) {
        w += mRainRate;
    }
}

void HydraulicErosion::computeFlow() {
    std::vector<float> newWater = mWater;
    float* heightData = mTerrain->get_data()->data();
    
    for (int y = 1; y < mHeight - 1; y++) {
        for (int x = 1; x < mWidth - 1; x++) {
            int idx = y * mWidth + x;
            float currentWater = mWater[idx];
            if (currentWater < 0.001f) continue;
            
            float totalHeight = heightData[idx] + currentWater;
            
            // Voisins
            int up = idx - mWidth;
            int down = idx + mWidth;
            int left = idx - 1;
            int right = idx + 1;
            
            // Calculer où l'eau veut aller
            float flow[4] = {0, 0, 0, 0};
            float totalFlow = 0.0f;
            
            flow[0] = std::max(0.0f, totalHeight - (heightData[up] + mWater[up]));
            flow[1] = std::max(0.0f, totalHeight - (heightData[down] + mWater[down]));
            flow[2] = std::max(0.0f, totalHeight - (heightData[left] + mWater[left]));
            flow[3] = std::max(0.0f, totalHeight - (heightData[right] + mWater[right]));
            
            // Normaliser
            for (int i = 0; i < 4; i++) {
                if (flow[i] > mMinSlope) {
                    totalFlow += flow[i];
                } else {
                    flow[i] = 0.0f;
                }
            }
            
            if (totalFlow > 0.0f) {
                // Limiter à 80% de l'eau disponible
                float maxFlow = currentWater * 0.8f;
                if (totalFlow > maxFlow) {
                    float scale = maxFlow / totalFlow;
                    for (int i = 0; i < 4; i++) flow[i] *= scale;
                    totalFlow = maxFlow;
                }
                
                // Appliquer l'écoulement
                newWater[idx] -= totalFlow;
                newWater[up] += flow[0];
                newWater[down] += flow[1];
                newWater[left] += flow[2];
                newWater[right] += flow[3];
            }
        }
    }
    
    mWater = newWater;
}

void HydraulicErosion::transportSediment() {
    float* heightData = mTerrain->get_data()->data();
    
    for (int y = 1; y < mHeight - 1; y++) {
        for (int x = 1; x < mWidth - 1; x++) {
            int idx = y * mWidth + x;
            
            if (mWater[idx] < 0.001f) continue;
            
            // Estimer la pente
            float dx = (heightData[idx+1] + mWater[idx+1]) - 
                      (heightData[idx-1] + mWater[idx-1]);
            float dy = (heightData[idx+mWidth] + mWater[idx+mWidth]) - 
                      (heightData[idx-mWidth] + mWater[idx-mWidth]);
            float slope = std::sqrt(dx*dx + dy*dy);
            
            float capacity = mSedimentCapacity * mWater[idx] * slope;
            
            if (mSediment[idx] < capacity) {
                // Érosion légère
                float take = std::min(mSolubility * mWater[idx], 
                                     capacity - mSediment[idx]);
                take = std::min(take, heightData[idx] * 0.05f);
                
                heightData[idx] -= take;
                mSediment[idx] += take;
            } else {
                // Dépôt
                float deposit = (mSediment[idx] - capacity) * 0.3f;
                deposit = std::min(deposit, mSediment[idx]);
                
                heightData[idx] += deposit;
                mSediment[idx] -= deposit;
            }
        }
    }
}

void HydraulicErosion::evaporate() {
    for (int i = 0; i < mWater.size(); i++) {
        mWater[i] *= (1.0f - mEvaporationRate);
        mWater[i] = std::max(0.0f, mWater[i]);
        
        // Dépôt quand l'eau s'évapore
        if (mWater[i] < 0.001f && mSediment[i] > 0.0f) {
            float* heightData = mTerrain->get_data()->data();
            float deposit = mSediment[i] * 0.5f;
            heightData[i] += deposit;
            mSediment[i] -= deposit;
        }
    }
}

int HydraulicErosion::step() {
    if (!mTerrain) return 0;
    
    static int stepCount = 0;
    stepCount++;
    
    addRain();
    computeFlow();
    transportSediment();
    evaporate();
    
    // DEBUG simple
    if (stepCount % 10 == 0) {
        float totalWater = 0.0f;
        for (float w : mWater) totalWater += w;
        std::cout << "[Eau] Étape " << stepCount << " - Total: " << totalWater << std::endl;
    }
    
    return 1;
}