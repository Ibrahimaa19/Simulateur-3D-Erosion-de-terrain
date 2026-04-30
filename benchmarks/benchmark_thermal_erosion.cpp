#include "ThermalErosionCore.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
using Clock = std::chrono::steady_clock;

enum class TerrainMode
{
    Indexed,
    Normalized
};

struct Options
{
    std::vector<int> sizes{512, 1024, 2048};
    int iterations = 100;
    int warmup = 5;
    ThermalNeighborMode neighbors = ThermalNeighborMode::Eight;
    TerrainMode terrainMode = TerrainMode::Indexed;
    float talusAngleDegrees = 30.0f;
    float transferRate = 0.5f;
    bool help = false;
    std::string output = "benchmarks/results/thermal_erosion_profile.csv";
};

struct SizeRunStats
{
    double sumIterationMs = 0.0;
    double sumMassError = 0.0;
    long long sumModifiedCells = 0;
    int massErrorCount = 0;
    int minModifiedCells = std::numeric_limits<int>::max();
    int maxModifiedCells = std::numeric_limits<int>::min();
};

double elapsedMs(const Clock::time_point& start, const Clock::time_point& end)
{
    return std::chrono::duration<double, std::milli>(end - start).count();
}

void printUsage(const char* exe)
{
    std::cout
        << "Usage: " << exe << " [options]\n"
        << "\n"
        << "Options:\n"
        << "  --sizes 512 1024 2048 4096        Tailles N des terrains N x N\n"
        << "  --iterations 100                  Iterations mesurees\n"
        << "  --warmup 5                        Iterations de chauffe non ecrites\n"
        << "  --neighbors 8                     Voisinage thermique (8 disponible, 4 reserve)\n"
        << "  --terrain indexed                 Terrain indexed ou normalized\n"
        << "  --talus-angle 30                  Angle de talus en degres\n"
        << "  --transfer-rate 0.5               Taux de transfert de matiere\n"
        << "  --output path.csv                 Fichier CSV brut\n"
        << "  --help                            Affiche cette aide\n";
}

int parsePositiveInt(const std::string& value, const std::string& optionName)
{
    int parsed = 0;
    try
    {
        parsed = std::stoi(value);
    }
    catch (const std::exception&)
    {
        throw std::runtime_error("Valeur invalide pour " + optionName + ": " + value);
    }

    if (parsed <= 0)
    {
        throw std::runtime_error(optionName + " doit etre strictement positif");
    }
    return parsed;
}

int parseNonNegativeInt(const std::string& value, const std::string& optionName)
{
    int parsed = 0;
    try
    {
        parsed = std::stoi(value);
    }
    catch (const std::exception&)
    {
        throw std::runtime_error("Valeur invalide pour " + optionName + ": " + value);
    }

    if (parsed < 0)
    {
        throw std::runtime_error(optionName + " doit etre positif ou nul");
    }
    return parsed;
}

float parseFloat(const std::string& value, const std::string& optionName)
{
    float parsed = 0.0f;
    try
    {
        parsed = std::stof(value);
    }
    catch (const std::exception&)
    {
        throw std::runtime_error("Valeur invalide pour " + optionName + ": " + value);
    }
    return parsed;
}

ThermalNeighborMode parseNeighborMode(const std::string& value)
{
    const int neighbors = parsePositiveInt(value, "--neighbors");
    if (neighbors == 8)
    {
        return ThermalNeighborMode::Eight;
    }
    if (neighbors == 4)
    {
        return ThermalNeighborMode::Four;
    }
    throw std::runtime_error("--neighbors accepte uniquement 4 ou 8");
}

TerrainMode parseTerrainMode(const std::string& value)
{
    if (value == "indexed")
    {
        return TerrainMode::Indexed;
    }
    if (value == "normalized")
    {
        return TerrainMode::Normalized;
    }
    throw std::runtime_error("--terrain accepte uniquement indexed ou normalized");
}

Options parseOptions(int argc, char** argv)
{
    Options options;

    for (int i = 1; i < argc; ++i)
    {
        const std::string arg = argv[i];

        if (arg == "--help" || arg == "-h")
        {
            options.help = true;
        }
        else if (arg == "--sizes")
        {
            options.sizes.clear();
            while (i + 1 < argc)
            {
                const std::string next = argv[i + 1];
                if (next.rfind("--", 0) == 0)
                {
                    break;
                }
                options.sizes.push_back(parsePositiveInt(next, "--sizes"));
                ++i;
            }
            if (options.sizes.empty())
            {
                throw std::runtime_error("--sizes attend au moins une taille");
            }
        }
        else if (arg == "--iterations")
        {
            if (i + 1 >= argc) throw std::runtime_error("--iterations attend une valeur");
            options.iterations = parsePositiveInt(argv[++i], "--iterations");
        }
        else if (arg == "--warmup")
        {
            if (i + 1 >= argc) throw std::runtime_error("--warmup attend une valeur");
            options.warmup = parseNonNegativeInt(argv[++i], "--warmup");
        }
        else if (arg == "--neighbors")
        {
            if (i + 1 >= argc) throw std::runtime_error("--neighbors attend une valeur");
            options.neighbors = parseNeighborMode(argv[++i]);
        }
        else if (arg == "--terrain")
        {
            if (i + 1 >= argc) throw std::runtime_error("--terrain attend une valeur");
            options.terrainMode = parseTerrainMode(argv[++i]);
        }
        else if (arg == "--talus-angle")
        {
            if (i + 1 >= argc) throw std::runtime_error("--talus-angle attend une valeur");
            options.talusAngleDegrees = parseFloat(argv[++i], "--talus-angle");
        }
        else if (arg == "--transfer-rate")
        {
            if (i + 1 >= argc) throw std::runtime_error("--transfer-rate attend une valeur");
            options.transferRate = parseFloat(argv[++i], "--transfer-rate");
        }
        else if (arg == "--output")
        {
            if (i + 1 >= argc) throw std::runtime_error("--output attend un chemin");
            options.output = argv[++i];
        }
        else
        {
            throw std::runtime_error("Option inconnue: " + arg);
        }
    }

    if (!isThermalNeighborModeAvailable(options.neighbors))
    {
        throw std::runtime_error("Le voisinage 4 n'est pas disponible dans cette branche");
    }
    if (options.transferRate < 0.0f || options.transferRate > 1.0f)
    {
        throw std::runtime_error("--transfer-rate doit etre compris entre 0 et 1");
    }

    return options;
}

int neighborModeToInt(ThermalNeighborMode mode)
{
    return mode == ThermalNeighborMode::Eight ? 8 : 4;
}

const char* terrainModeToString(TerrainMode mode)
{
    return mode == TerrainMode::Indexed ? "indexed" : "normalized";
}

std::vector<float> generateNormalizedTerrain(int size)
{
    std::vector<float> data(static_cast<std::size_t>(size) * static_cast<std::size_t>(size));

    // Terrain normalise historique : les frequences sont ramenees dans [0, 1].
    for (int z = 0; z < size; ++z)
    {
        for (int x = 0; x < size; ++x)
        {
            const double nx = size > 1 ? static_cast<double>(x) / static_cast<double>(size - 1) : 0.0;
            const double nz = size > 1 ? static_cast<double>(z) / static_cast<double>(size - 1) : 0.0;
            const double dx = nx - 0.5;
            const double dz = nz - 0.5;

            const double waves =
                28.0 * std::sin(20.0 * nx + 7.0 * nz) +
                17.0 * std::cos(11.0 * nz - 3.0 * nx) +
                12.0 * std::sin(31.0 * (nx + nz));
            const double mound = 38.0 * std::exp(-5.5 * (dx * dx + dz * dz));

            data[static_cast<std::size_t>(z) * static_cast<std::size_t>(size) + x] =
                static_cast<float>(100.0 + waves + mound);
        }
    }

    return data;
}

std::vector<float> generateIndexedTerrain(int size)
{
    std::vector<float> data(static_cast<std::size_t>(size) * static_cast<std::size_t>(size));

    // Terrain indexe : les variations locales restent comparables entre tailles.
    for (int z = 0; z < size; ++z)
    {
        for (int x = 0; x < size; ++x)
        {
            const double fx = static_cast<double>(x);
            const double fz = static_cast<double>(z);

            const double waves =
                8.0 * std::sin(0.050 * fx + 0.020 * fz) +
                6.0 * std::cos(0.030 * fz - 0.040 * fx) +
                4.0 * std::sin(0.070 * (fx + fz));

            const double dx = fx - 0.5 * static_cast<double>(size);
            const double dz = fz - 0.5 * static_cast<double>(size);
            const double sigma = 0.18 * static_cast<double>(size);
            const double mound = 25.0 * std::exp(-(dx * dx + dz * dz) / (2.0 * sigma * sigma));

            data[static_cast<std::size_t>(z) * static_cast<std::size_t>(size) + x] =
                static_cast<float>(100.0 + waves + mound);
        }
    }

    return data;
}

std::vector<float> generateTerrain(int size, TerrainMode mode)
{
    if (mode == TerrainMode::Indexed)
    {
        return generateIndexedTerrain(size);
    }
    return generateNormalizedTerrain(size);
}

double computeMass(const std::vector<float>& data)
{
    double mass = 0.0;
    for (const float value : data)
    {
        mass += static_cast<double>(value);
    }
    return mass;
}

double computeMassError(double massBefore, double massAfter)
{
    const double denominator = std::abs(massBefore);
    if (denominator <= std::numeric_limits<double>::epsilon())
    {
        return std::numeric_limits<double>::quiet_NaN();
    }
    return std::abs(massAfter - massBefore) / denominator;
}

void writeHeader(std::ofstream& csv)
{
    csv << "terrain_mode,terrain_size,neighbors,iteration,iteration_ms,total_elapsed_ms,total_cells,"
        << "modified_cells,mass_before,mass_after,mass_error\n";
}

void runBenchmarkForSize(const Options& options, int terrainSize, std::ofstream& csv)
{
    std::cout << "[benchmark_thermal_erosion] Generation du terrain "
              << terrainSize << " x " << terrainSize
              << " (" << terrainModeToString(options.terrainMode) << ")" << std::endl;

    std::vector<float> terrain = generateTerrain(terrainSize, options.terrainMode);
    const float talusThreshold = thermalTalusThresholdFromDegrees(options.talusAngleDegrees);
    const long long totalCells =
        static_cast<long long>(terrainSize) * static_cast<long long>(terrainSize);

    std::cout << "[benchmark_thermal_erosion] Warm-up: " << options.warmup
              << " iterations, mesure: " << options.iterations << " iterations"
              << std::endl;

    for (int i = 0; i < options.warmup; ++i)
    {
        runThermalErosionStep(terrain,
                              terrainSize,
                              terrainSize,
                              talusThreshold,
                              options.transferRate,
                              options.neighbors);
    }

    double totalElapsedMs = 0.0;
    SizeRunStats runStats;
    for (int iteration = 0; iteration < options.iterations; ++iteration)
    {
        const double massBefore = computeMass(terrain);

        const auto start = Clock::now();
        const ThermalErosionStepStats stats = runThermalErosionStep(terrain,
                                                                    terrainSize,
                                                                    terrainSize,
                                                                    talusThreshold,
                                                                    options.transferRate,
                                                                    options.neighbors);
        const auto end = Clock::now();

        const double iterationMs = elapsedMs(start, end);
        totalElapsedMs += iterationMs;

        const double massAfter = computeMass(terrain);
        const double massError = computeMassError(massBefore, massAfter);

        runStats.sumIterationMs += iterationMs;
        runStats.sumModifiedCells += stats.modifiedCells;
        runStats.minModifiedCells = std::min(runStats.minModifiedCells, stats.modifiedCells);
        runStats.maxModifiedCells = std::max(runStats.maxModifiedCells, stats.modifiedCells);
        if (std::isfinite(massError))
        {
            runStats.sumMassError += massError;
            runStats.massErrorCount++;
        }

        csv << terrainModeToString(options.terrainMode) << ','
            << terrainSize << ','
            << neighborModeToInt(options.neighbors) << ','
            << iteration << ','
            << iterationMs << ','
            << totalElapsedMs << ','
            << totalCells << ','
            << stats.modifiedCells << ','
            << massBefore << ','
            << massAfter << ','
            << massError << '\n';
    }

    const double measuredIterations = static_cast<double>(options.iterations);
    const double meanIterationMs = runStats.sumIterationMs / measuredIterations;
    const double meanModifiedCells =
        static_cast<double>(runStats.sumModifiedCells) / measuredIterations;
    const double meanMassError = runStats.massErrorCount > 0
        ? runStats.sumMassError / static_cast<double>(runStats.massErrorCount)
        : std::numeric_limits<double>::quiet_NaN();

    std::cout << "[benchmark_thermal_erosion] Controle taille " << terrainSize
              << " | terrain_mode=" << terrainModeToString(options.terrainMode)
              << " | mean iteration_ms=" << meanIterationMs
              << " | mean modified_cells=" << meanModifiedCells
              << " | min modified_cells=" << runStats.minModifiedCells
              << " | max modified_cells=" << runStats.maxModifiedCells
              << " | mean mass_error=" << meanMassError
              << std::endl;
}

} // namespace

int main(int argc, char** argv)
{
    try
    {
        const Options options = parseOptions(argc, argv);
        if (options.help)
        {
            printUsage(argv[0]);
            return 0;
        }

        const std::filesystem::path outputPath(options.output);
        const std::filesystem::path parentPath = outputPath.parent_path();
        if (!parentPath.empty())
        {
            std::filesystem::create_directories(parentPath);
        }

        std::ofstream csv(outputPath);
        if (!csv)
        {
            throw std::runtime_error("Impossible d'ouvrir le fichier de sortie: " + options.output);
        }
        csv << std::setprecision(10);
        writeHeader(csv);

        for (const int terrainSize : options.sizes)
        {
            runBenchmarkForSize(options, terrainSize, csv);
        }

        std::cout << "[benchmark_thermal_erosion] Resultats ecrits dans "
                  << options.output << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[benchmark_thermal_erosion] Erreur: " << e.what() << std::endl;
        printUsage(argv[0]);
        return 1;
    }

    return 0;
}
