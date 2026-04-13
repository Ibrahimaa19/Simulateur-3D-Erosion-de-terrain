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
#include "Mpi.hpp"

enum class State{
    Render,
    Test,
    MPI
};

enum class Heightmap{
    LoadHeightmap, 
    FaultFormation, 
    MidpointDisplacement, 
    PerlinNoise
};

std::map<std::string, State> dicState{{"render", State::Render}, {"test", State::Test},{"MPI", State::MPI}};
std::map<std::string, Heightmap> dicHeightmap{{"loadHeightmap", Heightmap::LoadHeightmap}, {"faultFormation", Heightmap::FaultFormation}
                                                ,{"midpointDisplacement", Heightmap::MidpointDisplacement}, {"perlinNoise", Heightmap::PerlinNoise} };

bool isMPI() {
    const char* size = std::getenv("OMPI_COMM_WORLD_SIZE");
    return size != nullptr;
}

int main(int argc, char *argv[])
{

    if(argc == 1 || State::Render == dicState[argv[1]]){
        if(!isMPI()){
            printf("[!] Vous lancez le mode graphique avec mpirun ! \n");
            exit(1);
        }

        TerrainApp app;
        app.Init();
        app.Run();
        
        
    }
    else if (State::Test == dicState[argv[1]]) {

        if(!isMPI()){
            printf("[!] Vous lancez le mode test avec mpirun ! \n");
            exit(1);
        }

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
            generator->CreatePerlinNoise(5000, 5000, 0, 255, 1, 0.005);
            terrain = std::move(generator);
            break;
        }

        default:
            std::cerr << "Heightmap non prise en charge" << std::endl;
            return 1;
        }

        ValidationTest::run_all_tests(terrain, terrainType, steps);
    }else if(State::MPI == dicState[argv[1]]){

        if (argc < 6)
        {
            printf("usage : erosion MPI <terrain> <width> <height> <step>\n");
            return 1;
        }   

        lauchSequential(argc,argv);
    }

    else{
        fprintf(stdout, "Usage: %s render\n", argv[0]);
        fprintf(stdout, "Usage: %s test <typeTerrain>\n", argv[0]);
        fprintf(stdout, "Usage: %s MPI <typeTerrain>\n", argv[0]);
        fprintf(stdout, "<typeTerrain> : loadHeightmap | faultFormation | midpointDisplacement | perlinNoise");
    }

    return 0;
}
