#include "ValidationTest.hpp"
#include <algorithm>
#include <iomanip>

// TEST 1: CONSERVATION DE LA MASSE

bool ValidationTest::test_mass_conservation(ThermalErosion& erosion, int steps, float tolerance) {
    std::cout << "\n[TEST 1] Conservation de la masse...\n";
    
    // Récupérer les données initiales
    const auto* data_ptr = erosion.get_data();
    if (!data_ptr) {
        std::cerr << "Données non disponibles\n";
        return false;
    }
    
    std::vector<float> initial_data = *data_ptr;
    float mass_before = calculate_total_mass(initial_data);
    
    std::cout << "  Masse initiale: " << mass_before << "\n";
    std::cout << "  Application de " << steps << " steps...\n";
    
    // Appliquer l'érosion
    for (int i = 0; i < steps; i++) {
        erosion.step();
        
        // Affichage de progression
        if ((i+1) % (steps/100) == 0) {
            std::cout << "    " << (i+1) << "/" << steps << " steps\n";
        }
    }
    
    // Récupérer les données finales
    const auto* final_data_ptr = erosion.get_data();
    float mass_after = calculate_total_mass(*final_data_ptr);
    float error = std::abs(mass_after - mass_before) / mass_before;
    
    std::cout << "  Masse finale: " << mass_after << "\n";
    std::cout << "  Erreur relative: " << (error * 100) << "%\n";
    
    bool passed = error < tolerance;
    
    if (passed) {
        std::cout << " TEST RÉUSSI: Masse conservée (erreur < " << (tolerance * 100) << "%)\n";
    } else {
        std::cout << " TEST ÉCHOUÉ: Perte de masse trop importante\n";
    }
    
    return passed;
}

// TEST 2: RÉDUCTION DES PENTES

bool ValidationTest::test_slope_reduction(ThermalErosion& erosion, int steps, float min_reduction) {
    std::cout << "\n[TEST 2] Réduction des pentes...\n";
    
    const auto* data_ptr = erosion.get_data();
    if (!data_ptr) {
        std::cerr << "  Données non disponibles\n";
        return false;
    }
    
    int width = erosion.get_terrain_width();   
    int height = erosion.get_terrain_height(); 
    
    float slope_before = calculate_max_slope(*data_ptr, width, height);
    std::cout << "  Pente max initiale: " << slope_before << "\n";
    
    // Appliquer l'érosion
    for (int i = 0; i < steps; i++) {
        erosion.step();
    }
    
    float slope_after = calculate_max_slope(*data_ptr, width, height);
    float reduction = (slope_before - slope_after) / slope_before;
    
    std::cout << "  Pente max finale: " << slope_after << "\n";
    std::cout << "  Réduction: " << (reduction * 100) << "%\n";
    
    bool passed = reduction >= min_reduction;
    
    if (passed) {
        std::cout << " TEST RÉUSSI: Pentes réduites d'au moins " << (min_reduction * 100) << "%\n";
    } else {
        std::cout << " TEST ÉCHOUÉ: Réduction insuffisante\n";
    }
    
    return passed;
}

// TEST 3: LIMITES DE HAUTEUR

bool ValidationTest::test_height_limits(ThermalErosion& erosion, int steps) {
    std::cout << "\n[TEST 3] Limites de hauteur...\n";
    
    bool has_negative = false;
    bool has_infinite = false;
        
    for (int i = 0; i < steps; i++) {
        erosion.step();
        
        // Vérifier périodiquement
        if (i % 100 == 0) {
            const auto* data_ptr = erosion.get_data();
            if (!data_ptr) continue;
            
            for (float h : *data_ptr) {
                if (h < 0.0f) {
                    has_negative = true;
                    std::cout << " Hauteur négative détectée à step " << i << "\n";
                }
                if (std::isinf(h)) {
                    has_infinite = true;
                    std::cout << " Infini détecté à step " << i << "\n";
                }
            }
        }
    }
    
    bool passed = !(has_negative || has_infinite);
    
    if (passed) {
        std::cout << " TEST RÉUSSI: Aucune valeur aberrante\n";
    } else {
        std::cout << " TEST ÉCHOUÉ: Valeurs aberrantes détectées:\n";
        if (has_negative) std::cout << "    - Hauteurs négatives\n";
        if (has_infinite) std::cout << "    - Valeurs infinies\n";
    }
    
    return passed;
}

// EXÉCUTION DE TOUS LES TESTS

void ValidationTest::run_all_tests(Terrain& terrain) {
    std::cout << "  VALIDATION DE L'ÉROSION THERMIQUE\n";
    
    ThermalErosion erosion;  // 17°, 20% transfert
    erosion.loadTerrainInfo(&terrain);
    
    // Sauvegarder l'état initial
    terrain.export_terrain_to_csv("validation/initial.csv");
    
    int passed = 0;
    int total = 3;
    
    // Test 1: Conservation de la masse
    if (test_mass_conservation(erosion, 50, 0.01f)) passed++;
    
    // Test 2: Réduction des pentes
    erosion.loadTerrainInfo(&terrain);
    if (test_slope_reduction(erosion, 100, 0.3f)) passed++;
    
    // Test 3: Limites de hauteur
    erosion.loadTerrainInfo(&terrain);
    if (test_height_limits(erosion, 200)) passed++;
    
    // Sauvegarder l'état final
    terrain.export_terrain_to_csv("validation/final.csv");
    
    // Rapport
    std::cout << "\n" << std::string(50, '=') << "\n";
    std::cout << "RÉSULTATS: " << passed << "/" << total << " tests réussis\n";
    
    if (passed == total) {
        std::cout << "ALGORITHME VALIDÉ\n";
    } else {
        std::cout << "ALGORITHME NON VALIDÉ\n";
    }
    std::cout << std::string(50, '=') << "\n";
}

// FONCTIONS UTILITAIRES

float ValidationTest::calculate_total_mass(const std::vector<float>& data) {
    float total = 0.0f;
    for (float h : data) {
        total += h;
    }
    return total;
}

float ValidationTest::calculate_max_slope(const std::vector<float>& data,
                                         int width,
                                         int height)
{
    float max_slope = 0.0f;
    for (int i = 1; i < height - 1; i++) {
        for (int j = 1; j < width - 1; j++) {

            float h = data[i * width + j];

            float slopes[8] = {
                std::abs(h - data[(i - 1) * width + j]), 
                std::abs(h - data[(i + 1) * width + j]), 
                std::abs(h - data[i * width + (j - 1)]),
                std::abs(h - data[i * width + (j + 1)])
                std::abs(h - data[(i - 1) * width + (j - 1)]);
                std::abs(h - data[(i - 1) * width + (j + 1)]);
                std::abs(h - data[(i + 1) * width + (j - 1)]);
                std::abs(h - data[(i + 1) * width + (j + 1)]);
            };

            // Garder la pente maximale
            for (float s : slopes) {
                max_slope = std::max(max_slope, s);
            }
        }
    }

    return max_slope;
}