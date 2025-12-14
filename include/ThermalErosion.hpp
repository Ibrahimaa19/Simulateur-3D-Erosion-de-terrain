#include "Terrain.hpp"
#include <cmath>
#include <vector>

/**
 * @class ThermalErosion
 * @brief Implémente un algorithme d’érosion thermique sur un terrain.
**/
class ThermalErosion
{
public:

    /**
     * @brief Charge les informations nécessaires depuis un terrain.
     *
     * Récupère :
     * - le pointeur vers les données de hauteur
     * - les dimensions du terrain
     *
     * @param terrain Pointeur vers le terrain à éroder
     *
     */
    void loadTerrainInfo(Terrain* terrain) {
        m_data   = terrain->get_data();
        m_height = terrain->get_terrain_height();
        m_width  = terrain->get_terrain_width();
    }
    
    /**
     * @brief Définit l’angle de talus critique.
     *
     * @param talus Angle de talus en degrés
     */
    void setTalusAngle(float angle) { 
        float talus = std::tan(angle/180);
        talusAngle = talus; 
    }

    /**
     * @brief Définit le taux de transfert de matière.
     *
     * @param c Coefficient de transfert (généralement entre 0 et 1)
     */
    void setTransferRate(float c) { transferRate = c; }

    /**
     * @brief Exécute une étape de l’algorithme d’érosion thermique.
     *
     */
    void step();

private:
    /** Pointeur vers les données de hauteur du terrain */
    std::vector<float>* m_data = nullptr;

    /** hauteur du terrain */
    int m_height = 0;

    /** largeur du terrain */
    int m_width = 0;

    /** Angle de talus critique (en degrés) */
    float talusAngle = 0.f;

    /** Taux de transfert de matière */
    float transferRate = 0.f;

    /**
     * @brief Retourne la hauteur au point (i, j).
     *
     * @param i Indice de ligne
     * @param j Indice de colonne
     * @return Hauteur à la position (i, j)
     */
    float get_height(int i, int j) const {
        return (*m_data)[i * m_width + j];
    }

    /**
     * @brief Retourne l’angle de talus courant.
     *
     * @return Angle de talus en degrés
     */
    float get_talus() { return talusAngle; }
};
