#include "Benchmark.hpp"
#include "FaultFormationTerrain.hpp"
#include "MidpointDisplacement.hpp"
#include "PerlinNoiseTerrain.hpp"
#include "ThermalErosion.hpp"
#include "ValidationTest.hpp"

#if EROSION_ENABLE_RENDERING
#include "TerrainApp.hpp"
#endif

#if EROSION_ENABLE_MPI
int lauchMPI(int argc, char* argv[]);
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
    Bench
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
    {"render", State::Render}, {"test", State::Test}, {"MPI", State::MPI}, {"mpi", State::MPI}, {"bench", State::Bench},
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
    std::cerr << "Usage: " << program << " bench <erosion|render|interaction|mpi> [options]\n";
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

    if (stateIt->second == State::Bench)
    {
        return Benchmark::run(argc, argv);
    }

    printUsage(argv[0]);
    return 1;
}
