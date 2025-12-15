#include "ValidationTest.hpp"
#include <algorithm>
#include <iomanip>

std::vector<float> ValidationTest::initialData;

float ValidationTest::test_mass_conservation(std::vector<float>& finalData) {
    float mass_before = calculate_total_mass(initialData);
    float mass_after = calculate_total_mass(finalData);

    float error = std::abs(mass_after - mass_before) / mass_before;    
    
    return error;
}

bool ValidationTest::test_height_limits(std::vector<float>& finalData) {
    std::cout << "\n[TEST 2] Limites de hauteur...\n";

    for (float h : finalData) {
        if (h < 0.0f) {
            return false;
        }
        if (std::isinf(h)) {
            return false;
        }
    }

    return true;
}

void ValidationTest::run_all_tests(std::unique_ptr<Terrain>& terrain, int steps){
    std::cout << "  VALIDATION DE L'ÉROSION THERMIQUE\n";
    
    ThermalErosion erosion;
    erosion.loadTerrainInfo(terrain);
    erosion.setTalusAngle(25.f);
    erosion.setTransferRate(0.1f);

    initialData = *(terrain->get_data());
    
    int cellsModified[steps];

    for(int i = 0; i < 100; ++i){
        cellsModified[i] = erosion.step();
    }

    float error = test_mass_conservation(*terrain->get_data());

    for(int i = 100; i < steps; ++i){
        cellsModified[i] = erosion.step();
    }

    float errorFromTimesteps = test_mass_conservation(*terrain->get_data());
    
    int passed = 0;
    int total = 2;

    if(error < TOLERANCE)
        ++passed;

    if (test_height_limits(*terrain->get_data()))
        ++passed;

    std::cout << "RÉSULTATS: " << std::endl; 
    std::cout << "Steps : "<< steps << std::endl;
    std::cout << "Test " << total << " : Passed " << passed << std::endl;
    std::cout << "Conservation error pour 100 steps: " << error << std::endl;
    std::cout << "Conservation error pour "<< steps << " steps: " << errorFromTimesteps << std::endl;
    std::cout << "Nombre de cellule modifie step 1: " << cellsModified[0] << std::endl ;
    std::cout << "Nombre de cellule modifie step " << steps << ": " << cellsModified[steps-1];
}

float ValidationTest::calculate_total_mass(const std::vector<float>& data) {
    float total = 0.0f;
    for (float h : data) {
        total += h;
    }
    return total;
}
