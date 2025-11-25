#include "../include/terrain.hpp"

/**
 * @class ThermalErosion
 * @brief Classe pour simuler l'érosion thermique d'un terrain
 *
 * L'érosion thermique déplace la matière des pentes trop raides vers les voisins plus bas
 * afin de stabiliser le relief et lisser les pentes
 */
class ThermalErosion
{
public:
    /**
     * @brief Constructeur de la classe ThermalErosion
     * @param talusAngle Angle critique des pentes (tangent), au-dessus duquel la matière est déplacée
     * @param transferRate Coefficient de diffusion
     */
    ThermalErosion(float talusAngle = 12.0f, float tranferRate = 0.9f);

    /**
     * @brief Applique une itération d'érosion thermique sur le terrain
     * @param terrain Terrain sur lequel appliquer l'érosion
     */
    void step(Terrain& terrain);

private:
    float talusAngle;
    float transferRate;      
};
