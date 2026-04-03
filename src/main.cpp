#define STB_IMAGE_IMPLEMENTATION

#include "TerrainApp.hpp"
#include "ValidationTest.hpp"
#include "ThermalErosion.hpp"
#include <map>
#include <string>
#include <memory>
#include <iostream>
#include <cstdlib>
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

std::map<std::string, State> dicState{
    {"render", State::Render},
    {"test", State::Test}
};

std::map<std::string, Heightmap> dicHeightmap{
    {"loadHeightmap", Heightmap::LoadHeightmap},
    {"faultFormation", Heightmap::FaultFormation},
    {"midpointDisplacement", Heightmap::MidpointDisplacement},
    {"perlinNoise", Heightmap::PerlinNoise}
};

int main(int argc, char const *argv[])
{
    if (argc == 1 || State::Render == dicState[argv[1]]) {
        TerrainApp app;
        app.Init();
        app.Run();
    }
    else if (State::Test == dicState[argv[1]]) {

        std::unique_ptr<Terrain> terrain;

        if (argc < 4) {
            std::cerr << "Usage: " << argv[0]
                      << " test <typeTerrain> <steps>\n";
            std::cerr << "<typeTerrain> : loadHeightmap | faultFormation | midpointDisplacement | perlinNoise\n";
            return 1;
        }

        std::string terrainType = argv[2];
        int steps = std::atoi(argv[3]);

        if (steps <= 0) {
            std::cerr << "Erreur: steps doit être strictement positif\n";
            return 1;
        }

        switch (dicHeightmap[terrainType])
        {
        case Heightmap::LoadHeightmap:
            terrain = std::make_unique<Terrain>();
            terrain->loadTerrain("../src/heightmap/iceland_heightmap.png", 1.0f, 100.0f);
            break;

        case Heightmap::FaultFormation:
        {
            auto generator = std::make_unique<FaultFormationTerrain>();
            generator->CreateFaultFormation(2048, 2048, 1000, 0, 255, 1);
            terrain = std::move(generator);
            break;
        }

        case Heightmap::MidpointDisplacement:
        {
            auto generator = std::make_unique<MidpointDisplacement>();
            generator->CreateMidpointDisplacement(std::pow(2, 11) + 1, 0, 255, 1, 0.5);
            terrain = std::move(generator);
            break;
        }

        case Heightmap::PerlinNoise:
        {
            auto generator = std::make_unique<PerlinNoiseTerrain>();
            generator->CreatePerlinNoise(2048, 2048, 0, 255, 1, 0.005);
            terrain = std::move(generator);
            break;
        }

        default:
            std::cerr << "Heightmap non prise en charge" << std::endl;
            return 1;
        }

        ValidationTest::run_all_tests(terrain, terrainType, steps);
    }
    else {
        std::cout << "Usage: " << argv[0] << " render\n";
        std::cout << "Usage: " << argv[0] << " test <typeTerrain> <steps>\n";
        std::cout << "<typeTerrain> : loadHeightmap | faultFormation | midpointDisplacement | perlinNoise\n";
    }

    return 0;
}