#include "Terrain.hpp"
#include <random>
#include <vector>

/**
 * @class HydraulicErosion
 * @brief Simule l'érosion hydraulique par gouttes d'eau (modèle particle-based)
 *
 * Architecture par batching (prête à la parallélisation) :
 * - Chaque batch : toutes les gouttes lisent le même snapshot du terrain
 * - Les contributions (érosion/dépôt) sont accumulées dans un buffer delta
 * - À la fin du batch : terrain += delta
 *
 * Algorithme de référence :
 * - Chaque goutte tombe aléatoirement, descend vers le voisin le plus bas
 * - Érosion : retire de la matière proportionnelle à (pente × eau)
 * - Dépôt : à l'arrêt (cul-de-sac), dépose les sédiments transportés
 * - Évaporation : réduit le volume d'eau à chaque déplacement
 *
 * Exécution : apply() = tout d'un coup par batches, applyBatch() = incrémental (Lancer/Arrêter)
 */
class HydraulicErosion
{
public:
    HydraulicErosion(int iterations = 20000,
                     float rain = 1.0f,
                     float erosionRate = 0.05f,
                     float depositRate = 1.0f,
                     float evaporation = 0.1f);

    void setSeed(unsigned int seed);

    void setParams(int iterations, float rain, float erosionRate, float depositRate, float evaporation);

    /** Définit la taille des batches (gouttes par batch). Par défaut 2000 pour terrain 1024×1024. */
    void setBatchSize(int batchSize) { m_batchSize = std::max(1, batchSize); }
    int getBatchSize() const { return m_batchSize; }

    /** Applique toutes les gouttes par batches. */
    void apply(Terrain& terrain);

    /** Applique un lot de gouttes (pour mode incrémental). Retourne le nombre traité. */
    int applyBatch(Terrain& terrain, int batchSize);

private:
    int iterations;
    float rain;
    float erosionRate;
    float depositRate;
    float evaporation;
    int m_batchSize = 2000;
    std::mt19937 m_rng;

    /** Buffer d'accumulation des deltas (érosion/dépôt) par cellule. Réutilisé à chaque batch. */
    std::vector<float> m_delta;

    /** Traite une goutte : lit le terrain (inchangé), écrit dans m_delta. Retourne 1. */
    int processOneDroplet(const Terrain& terrain, std::vector<float>& delta);
};
