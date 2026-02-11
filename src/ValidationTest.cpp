#include "ValidationTest.hpp"
#include <algorithm>
#include <iomanip>
#include <filesystem>
#include <fstream>

std::vector<float> ValidationTest::initialData;

float ValidationTest::test_mass_conservation(std::vector<float>& finalData) {
    float mass_before = calculate_total_mass(initialData);
    float mass_after = calculate_total_mass(finalData);

    float error = std::abs(mass_after - mass_before) / mass_before;    
    
    return error;
}

bool ValidationTest::test_height_limits(std::vector<float>& finalData) {

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

void ValidationTest::run_all_tests(std::unique_ptr<Terrain>& terrain, const std::string& terrainType, int steps){
    namespace fs = std::filesystem;

    fs::path baseDir = "./resultat/" + terrainType;
    fs::create_directories(baseDir);

    ThermalErosion erosion;
    erosion.loadTerrainInfo(terrain);
    erosion.setTalusAngle(25.f);
    erosion.setTransferRate(0.1f);

    initialData = *(terrain->get_data());
    
    int cellsModified[steps];

    cellsModified[0] = erosion.step();

    float error = test_mass_conservation(*terrain->get_data());

    std::string fichier_name = "./resultat/resultats_error_" +terrainType +".csv";
    std::ofstream fichier(fichier_name, std::ios::app);
    if (!fichier.is_open()) {
        std::cerr << "Erreur d'ouverture du fichier" << std::endl;
        exit(1);
    }
    
    for(int i = 1; i < steps; ++i){
        cellsModified[i] = erosion.step();
        if (i % 100 == 0) {
            fichier << i << "," << test_mass_conservation(*terrain->get_data()) << "\n";
            fichier.flush();
        }
    }

    fs::path cellFile = baseDir / ("cellModified" + std::to_string(steps) + ".csv");

    std::ofstream cellOut(cellFile);
    cellOut << "step,cells_modified\n";


    for (int i = 0; i < steps; ++i) {
        cellOut << i << "," << cellsModified[i] << "\n";

        
    }

    cellOut.close();

    float errorFromTimesteps = test_mass_conservation(*terrain->get_data());
    fs::path errorFile = baseDir / ("errorTimeStep" + std::to_string(steps) + ".csv");

    std::ofstream errorOut(errorFile);
    errorOut << errorFromTimesteps << std::endl;

    errorOut.close();

    int passed = 0;
    int total = 2;

    if(error < TOLERANCE)
        ++passed;

    if (test_height_limits(*terrain->get_data()))
        ++passed;

    std::cout << "RÉSULTATS: " << std::endl; 
    std::cout << "Steps : "<< steps << std::endl;
    std::cout << "Test " << total << " : Passed " << passed << std::endl;
    std::cout << "Conservation error pour 1 step: " << error << std::endl;
    std::cout << "Conservation error pour "<< steps << " steps: " << errorFromTimesteps << std::endl;
    std::cout << "Nombre de cellule modifie step 1: " << cellsModified[0] << std::endl ;
    std::cout << "Nombre de cellule modifie step " << steps << ": " << cellsModified[steps-1] << std::endl;
}

float ValidationTest::calculate_total_mass(const std::vector<float>& data) {
    float total = 0.0f;
    for (float h : data) {
        total += h;
    }
    return total;
}
