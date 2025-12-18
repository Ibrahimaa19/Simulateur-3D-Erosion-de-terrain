#define STB_IMAGE_IMPLEMENTATION

#include "TerrainApp.hpp"
#include "ValidationTest.hpp"
#include "ThermalErosion.hpp"
#include <map>
#include <string>
#include <memory>
#include <chrono>
#include <fstream>
#include <vector>
#include <iostream>
#include <cmath>

enum class State{
    Render,
    Test,
};

enum class Heightmap{
    LoadHeightmap, 
    FaultFormation, 
    MidpointDisplacement, 
    PerlinNoise
};

// Dictionnaires pour récupérer les enums à partir de chaînes
std::map<std::string, State> dicState{{"render", State::Render}, {"test", State::Test}};
std::map<std::string, Heightmap> dicHeightmap{
    {"loadHeightmap", Heightmap::LoadHeightmap}, 
    {"faultFormation", Heightmap::FaultFormation},
    {"midpointDisplacement", Heightmap::MidpointDisplacement}, 
    {"perlinNoise", Heightmap::PerlinNoise} 
};

int main(int argc, char const *argv[])
{
    if(argc == 1 || (argc >= 2 && dicState.count(argv[1]) && dicState[argv[1]] == State::Render)){
        TerrainApp app;
        app.Init();
        app.Run();
    }
    else if(argc >= 4 && std::string(argv[1]) == "test") {

        std::string terrainType = argv[2];
        int steps = std::atoi(argv[3]);

        if(steps <= 0){
            std::cerr << "Erreur: steps doit être strictement positif\n";
            return 1;
        }

        std::unique_ptr<Terrain> terrain;

        switch(dicHeightmap[terrainType]){
            case Heightmap::LoadHeightmap:
                terrain = std::make_unique<Terrain>();
                terrain->load_terrain("../src/heightmap/iceland_heightmap.png", 1.0f, 100.0f);
                break;
            case Heightmap::FaultFormation: {
                auto generator = std::make_unique<FaultFormationTerrain>();
                generator->CreateFaultFormation(1024, 1024, 1000, 0, 255, 1);
                terrain = std::move(generator);
                break;
            }
            case Heightmap::MidpointDisplacement: {
                auto generator = std::make_unique<MidpointDisplacement>();
                generator->CreateMidpointDisplacement(std::pow(2, 10)+1, 0, 255, 1, 0.5);
                terrain = std::move(generator);
                break;
            }
            case Heightmap::PerlinNoise: {
                auto generator = std::make_unique<PerlinNoiseTerrain>();
                generator->CreatePerlinNoise(1024, 1024, 0, 255, 1, 0.005);
                terrain = std::move(generator);
                break;
            }
            default:
                std::cerr << "Heightmap non prise en charge\n";
                return 1;
        }

        ValidationTest::run_all_tests(terrain, terrainType, steps);
    }

    // ---------------- Benchmark performance ----------------
    else if(argc == 2 && std::string(argv[1]) == "bench") {

        std::vector<std::string> terrains = {"faultFormation", "midpointDisplacement", "perlinNoise"};
        std::vector<int> sizes = {512, 1024, 2048};
        std::vector<int> stepsList = {100, 200, 500, 1000, 1500, 2000, 2500, 3000, 4000, 5000};

        for(const auto& terrainType : terrains){

            std::string filename = "../performance_" + terrainType + ".csv";
            std::ofstream csvFile(filename);
            csvFile << "Size,Steps,Duration_ms\n";

            for(const auto& size : sizes){
                std::unique_ptr<Terrain> terrain;
                if(terrainType == "faultFormation"){
                    auto generator = std::make_unique<FaultFormationTerrain>();
                    generator->CreateFaultFormation(size, size, 1000, 0, 255, 1);
                    terrain = std::move(generator);
                }
                else if(terrainType == "midpointDisplacement"){
                    auto generator = std::make_unique<MidpointDisplacement>();
                    generator->CreateMidpointDisplacement(size+1, 0, 255, 1, 0.5);
                    terrain = std::move(generator);
                }
                else if(terrainType == "perlinNoise"){
                    auto generator = std::make_unique<PerlinNoiseTerrain>();
                    generator->CreatePerlinNoise(size, size, 0, 255, 1, 0.005);
                    terrain = std::move(generator);
                }

                ThermalErosion erosion;
                erosion.loadTerrainInfo(terrain);

                for(const auto& steps : stepsList){
                    auto start = std::chrono::high_resolution_clock::now();
                    for(int s=0; s<steps; ++s) erosion.step();
                    auto end = std::chrono::high_resolution_clock::now();

                    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();

                    std::cout << "Terrain: " << terrainType << ", Size: " << size << ", Steps: " << steps << ", Duration: " << duration_ms << " ms" << std::endl;

                    csvFile << size << "," << steps << "," << duration_ms << "\n";
                }
            }
            csvFile.close();
            std::cout << "Résultats enregistrés dans " << filename << std::endl;
        }
    }
    // ---------------- Usage ----------------
    else{
        std::cout << "Usage:\n"
                  << "./erosion              : run simulation\n"
                  << "./erosion test <typeTerrain> <steps> : run unit tests\n"
                  << "./erosion bench        : run performance benchmark\n"
                  << "<typeTerrain>: faultFormation | midpointDisplacement | perlinNoise\n";
    }

    return 0;
}
