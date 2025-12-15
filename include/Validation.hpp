#ifndef VALIDATION_TEST_HPP
#define VALIDATION_TEST_HPP

#include "ThermalErosion.hpp"
#include "Terrain.hpp"
#include <vector>
#include <string>
#include <fstream>
#include <cmath>
#include <iostream>

/**
 * @class ValidationTest
 * @brief Tests essentiels de validation de l'érosion thermique
 */
class ValidationTest {
public:
    /**
     * @brief Test 1: Vérifie que la masse totale est conservée
     * @param erosion Référence à l'érosion thermique
     * @param steps Nombre d'étapes à tester
     * @param tolerance Tolérance en % (défaut: 1%)
     * @return true si la masse est conservée
     */
    static bool test_mass_conservation(ThermalErosion& erosion, int steps = 50, float tolerance = 0.01f);
    
    /**
     * @brief Test 2: Vérifie que les pentes diminuent
     * @param erosion Référence à l'érosion thermique
     * @param steps Nombre d'étapes à tester
     * @param min_reduction Réduction minimale attendue (défaut: 30%)
     * @return true si les pentes diminuent suffisamment
     */
    static bool test_slope_reduction(ThermalErosion& erosion, int steps = 100, float min_reduction = 0.3f);
    
    /**
     * @brief Test 3: Vérifie qu'il n'y a pas de valeurs aberrantes
     * @param erosion Référence à l'érosion thermique
     * @param steps Nombre d'étapes à tester
     * @return true si pas de valeurs négatives ou NaN
     */
    static bool test_height_limits(ThermalErosion& erosion, int steps = 200);
    
    /**
     * @brief Exécute tous les tests et affiche un "compte rendu"
     * @param terrain Terrain à tester
     */
    static void run_all_tests(Terrain& terrain);
    
private:
    static float calculate_total_mass(const std::vector<float>& data);
    static float calculate_max_slope(const std::vector<float>& data, int width, int height);
};

#endif