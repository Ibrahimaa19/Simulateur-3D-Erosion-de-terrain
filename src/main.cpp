#include "FaultFormationTerrain.hpp"
#include "MidpointDisplacement.hpp"
#include "PerlinNoiseTerrain.hpp"
#include "ThermalErosion.hpp"
#include "ValidationTest.hpp"

#if EROSION_ENABLE_RENDERING
#include "TerrainApp.hpp"
#endif

#if EROSION_ENABLE_MPI
#include "Mpi.hpp"
#endif

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <map>
#include <memory>
#include <string>

enum class State
{
    Render,
    Test,
    MPI,
    Thermal
};

enum class Heightmap
{
    LoadHeightmap,
    FaultFormation,
    MidpointDisplacement,
    PerlinNoise
};

namespace
{
const std::map<std::string, State> kStates{
    {"render", State::Render},
    {"test", State::Test},
    {"MPI", State::MPI},
    {"mpi", State::MPI},
    {"thermal", State::Thermal},
};

const std::map<std::string, Heightmap> kHeightmaps{
    {"loadHeightmap", Heightmap::LoadHeightmap},
    {"faultFormation", Heightmap::FaultFormation},
    {"midpointDisplacement", Heightmap::MidpointDisplacement},
    {"perlinNoise", Heightmap::PerlinNoise},
};

void printUsage(const char* program)
{
    std::cerr << "Usage: " << program << " render\n";
    std::cerr << "Usage: " << program << " test <typeTerrain> <steps>\n";
    std::cerr << "Usage: " << program << " MPI <typeTerrain> <width> <height> <steps>\n";
    std::cerr << "<typeTerrain> : loadHeightmap | faultFormation | midpointDisplacement | perlinNoise\n";
}

std::unique_ptr<Terrain> buildTerrain(const std::string& terrainType)
{
    auto it = kHeightmaps.find(terrainType);
    if (it == kHeightmaps.end())
    {
        return nullptr;
    }

    switch (it->second)
    {
    case Heightmap::LoadHeightmap:
    {
        auto terrain = std::make_unique<Terrain>();
        terrain->loadTerrain("../src/heightmap/heightmap.png", 1.0f, 100.0f);
        return terrain;
    }

    case Heightmap::FaultFormation:
    {
        auto generator = std::make_unique<FaultFormationTerrain>();
        generator->CreateFaultFormation(2048, 2048, 1000, 0, 255, 1);
        return generator;
    }

    case Heightmap::MidpointDisplacement:
    {
        auto generator = std::make_unique<MidpointDisplacement>();
        generator->CreateMidpointDisplacement(std::pow(2, 11) + 1, 0, 255, 1, 0.5);
        return generator;
    }

    case Heightmap::PerlinNoise:
    {
        auto generator = std::make_unique<PerlinNoiseTerrain>();
        generator->CreatePerlinNoise(2048, 2048, 0, 255, 1, 0.005);
        return generator;
    }
    }

    return nullptr;
}
} // namespace

int main(int argc, char* argv[])
{
    const std::string mode = argc > 1 ? argv[1] : "render";
    const auto stateIt = kStates.find(mode);

    if (stateIt == kStates.end())
    {
        printUsage(argv[0]);
        return 1;
    }

    if (stateIt->second == State::Render)
    {
#if EROSION_ENABLE_RENDERING
        TerrainApp app;
        if (!app.Init())
        {
            std::cerr << "Erreur: impossible d'initialiser le rendu OpenGL.\n";
            return 1;
        }
        app.Run();
        return 0;
#else
        std::cerr << "Le mode rendu est désactivé dans ce build.\n";
        std::cerr << "Reconfigurez avec -DEROSION_ENABLE_RENDERING=ON si OpenGL/GLFW/GLM sont disponibles.\n";
        return 1;
#endif
    }

    if (stateIt->second == State::Test)
    {
        if (argc < 4)
        {
            printUsage(argv[0]);
            return 1;
        }

        const std::string terrainType = argv[2];
        const int steps = std::atoi(argv[3]);

        if (steps <= 0)
        {
            std::cerr << "Erreur: steps doit être strictement positif\n";
            return 1;
        }

        auto terrain = buildTerrain(terrainType);
        if (!terrain)
        {
            std::cerr << "Heightmap non prise en charge: " << terrainType << "\n";
            printUsage(argv[0]);
            return 1;
        }

        ValidationTest::run_all_tests(terrain, terrainType, steps);
        return 0;
    }

    if (stateIt->second == State::MPI)
    {
        if (argc < 6)
        {
            printUsage(argv[0]);
            return 1;
        }

#if EROSION_ENABLE_MPI
        return lauchMPI(argc, argv);
#else
        std::cerr << "Le mode MPI est désactivé dans ce build.\n";
        std::cerr << "Reconfigurez avec -DEROSION_ENABLE_MPI=ON si MPI est disponible.\n";
        return 1;
#endif
    }

    if (stateIt->second == State::Thermal)
    {
        if (argc < 4)
        {
            std::cerr << "Usage: " << argv[0] << " thermal <typeTerrain> <steps> [method]\n";
            std::cerr << "method: 0=Pure, 1=Blocked, 2=BlockedParallel, 3=Checkerboard,\n";
            std::cerr << "        4=BlockedCheckerboard, 5=InPlace, 6=InPlaceParallel\n";
            return 1;
        }

        const std::string terrainType = argv[2];
        const int steps = std::atoi(argv[3]);
        const int method = (argc >= 5) ? std::atoi(argv[4]) : 0;

        if (steps <= 0)
        {
            std::cerr << "Erreur: steps doit être strictement positif\n";
            return 1;
        }

        auto terrain = buildTerrain(terrainType);
        if (!terrain)
        {
            std::cerr << "Heightmap non prise en charge: " << terrainType << "\n";
            return 1;
        }

        const char* methodNames[] = {
            "Pure two-phase",
            "Blocked pure two-phase",
            "Blocked parallel pure two-phase",
            "Checkerboard pure two-phase",
            "Blocked checkerboard pure two-phase",
            "Checkerboard in-place",
            "Checkerboard in-place parallel"
        };

        ThermalErosion erosion;
        erosion.loadTerrainInfo(*terrain);  // ← CORRECTION ICI
        erosion.setTalusAngle(30.0f);
        erosion.setTransferRate(0.1f);
        erosion.useEightNeighbors();
        erosion.resetProgress();

        auto start = std::chrono::high_resolution_clock::now();
        
        int totalChanges = 0;
        for (int i = 0; i < steps; i++) {
            switch (method) {
                case 0: totalChanges += erosion.stepPureTwoPhase(); break;
                case 1: totalChanges += erosion.stepBlockedPureTwoPhase(); break;
                case 2: totalChanges += erosion.stepBlockedParallelPureTwoPhase(); break;
                case 3: totalChanges += erosion.stepCheckerboardPureTwoPhase(); break;
                case 4: totalChanges += erosion.stepBlockedCheckerboardPureTwoPhase(); break;
                case 5: totalChanges += erosion.stepCheckerboardInPlace(); break;
                case 6: totalChanges += erosion.stepCheckerboardInPlaceParallel(); break;
                default: std::cerr << "Méthode invalide" << std::endl; return 1;
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "\n========================================" << std::endl;
        std::cout << "Méthode: " << methodNames[method] << std::endl;
        std::cout << "Terrain: " << terrainType << std::endl;
        std::cout << "Taille: " << terrain->getTerrainWidth() << "x" << terrain->getTerrainHeight() << std::endl;
        std::cout << "Étapes: " << steps << std::endl;
        std::cout << "Temps total: " << duration.count() << " ms" << std::endl;
        std::cout << "Temps par étape: " << (duration.count() / steps) << " ms" << std::endl;
        std::cout << "Cellules modifiées: " << totalChanges << std::endl;
        std::cout << "========================================" << std::endl;
        
        return 0;
    }

    printUsage(argv[0]);
    return 1;
}
