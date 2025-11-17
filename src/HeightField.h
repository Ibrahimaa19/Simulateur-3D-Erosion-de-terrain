#include <vector>
#include <algorithm>

struct HeightField {
    int size;                  
    float cellSpacing;         
    std::vector<float> heights;

    // Constructeur
    HeightField(int terrainSize, float spacing)
    {
        size = terrainSize;    
        cellSpacing = spacing;
        heights = std::vector<float>(size * size, 0.0f);
    }

    // Accès simple à une hauteur
    float GetHeight(int i, int j) const
    {
        return heights[i * size + j];
    }

    void SetHeight(int i, int j, float value)
    {
        heights[i * size + j] += value;
    }

    bool inside(int i, int j) const {
        return (i >= 0 && i < size && j >= 0 && j < size);
    }
};

