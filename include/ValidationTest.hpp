#pragma once

#include "ThermalErosion.hpp"
#include "Terrain.hpp"

#include <vector>
#include <string>
#include <fstream>
#include <cmath>
#include <iostream>
#include <memory>

#define TOLERANCE 1e-5

/**
 * @class ValidationTest
 * @brief Classe utilitaire pour la validation de l’érosion thermique.
 *
 * Cette classe regroupe un ensemble de tests numériques permettant
 * d’évaluer la validité physique et la stabilité de l’algorithme
 * d’érosion thermique appliqué à un terrain discret.
 *
 * Les tests implémentés portent notamment sur :
 *  - la conservation de la masse,
 *  - la validité des valeurs de hauteur après érosion.
 */
class ValidationTest {
public:
    /**
     * @brief Exécute les tests de validation de l’érosion thermique et sauvegarde les résultats.
     *
     * Cette fonction applique l’algorithme d’érosion thermique sur un terrain
     * pendant un nombre donné d’itérations, puis vérifie la conservation de la masse
     * et la validité des hauteurs obtenues.
     *
     * Les résultats sont enregistrés dans le dossier :
     * resultat/<typeDeTerrain>/
     * sous forme de fichiers CSV et TXT.
     *
     * @param terrain Terrain sur lequel l’érosion et les tests sont appliqués.
     * @param terrainType Nom du type de terrain, utilisé pour organiser les résultats.
     * @param steps Nombre total d’itérations d’érosion.
     */
    static void run_all_tests(std::unique_ptr<Terrain>& terrain, const std::string& terrainType, int steps);
private:
    /**
     * @brief Test de conservation de la masse du terrain.
     *
     * Ce test compare la masse totale du terrain avant et après
     * l’application de l’érosion thermique.
     *
     * La masse est définie comme la somme de toutes les hauteurs
     * de la heightmap. Le résultat retourné est l’erreur relative
     * entre l’état initial et l’état final.
     *
     * @param finalData Données de hauteur du terrain après érosion.
     * @return Erreur relative de conservation de la masse.
     */
    static float test_mass_conservation(std::vector<float>& finalData);

    /**
     * @brief Vérification des limites numériques des hauteurs.
     *
     * Ce test s’assure que les valeurs de hauteur du terrain après
     * érosion restent physiquement et numériquement valides :
     *  - aucune hauteur négative,
     *  - aucune valeur infinie.
     *
     * @param finalData Données de hauteur du terrain après érosion.
     * @return true si toutes les valeurs sont valides, false sinon.
     */
    static bool test_height_limits(std::vector<float>& finalData);

    /**
     * @brief Données de hauteur du terrain avant l’érosion.
     *
     * Ce vecteur stocke l’état initial du terrain et sert de référence
     * pour le calcul de l’erreur de conservation de la masse.
     */
    static std::vector<float> initialData;

    /**
     * @brief Calcule la masse totale d’un terrain.
     *
     * La masse totale est définie comme la somme de toutes les hauteurs
     * de la heightmap.
     *
     * @param data Données de hauteur du terrain.
     * @return Masse totale du terrain.
     */
    static float calculate_total_mass(const std::vector<float>& data);
};

