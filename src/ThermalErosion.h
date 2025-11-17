#pragma once
#include "HeightField.h"

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
     * @param c Fraction de matériau transférée par itération
     */
    ThermalErosion(float talusAngle = 0.6f, float c = 0.1f);

    /**
     * @brief Applique une itération d'érosion thermique sur le terrain
     * @param terrain Terrain sur lequel appliquer l'érosion
     */
    void step(HeightField& terrain);

private:
    float talusAngle;
    float c;      
};
