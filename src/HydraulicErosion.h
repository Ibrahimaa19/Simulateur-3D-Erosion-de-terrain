#include "../include/terrain.hpp"

/**
 * @class HydraulicErosion
 * @brief Classe pour simuler l'érosion hydraulique d'un terrain
 *
 * Le modèle "goutte d'eau" simule la pluie, l'érosion, le transport et le dépôt de sédiments
 * Chaque goutte se déplace sur le terrain en suivant la pente et modifie les hauteurs locales
 */
class HydraulicErosion
{
public:
    /**
     * @brief Constructeur de la classe HydraulicErosion
     * @param iterations Nombre de gouttes à simuler
     * @param rain Quantité d'eau ajoutée par goutte
     * @param erosionRate Taux d'érosion par pente et par goutte
     * @param depositRate Taux de dépôt de sédiments
     * @param evaporation Fraction d'eau évaporée par étape
     */
    HydraulicErosion(int iterations = 20000,
                     float rain = 1.0f,
                     float erosionRate = 0.02f,
                     float depositRate = 0.05f,
                     float evaporation = 0.1f);

    /**
     * @brief Applique l'érosion hydraulique sur le terrain
     * @param terrain Terrain sur lequel appliquer l'érosion
     */
    void apply(Terrain& terrain);

private:
    int iterations;   
    float rain;          
    float erosionRate;   
    float depositRate;   
    float evaporation;   
};
