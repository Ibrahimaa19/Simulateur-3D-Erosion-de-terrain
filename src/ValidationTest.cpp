#include "ValidationTest.hpp"
#include <algorithm>
#include <iomanip>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <chrono>
#include <cmath>
#include <vector>

std::vector<float> ValidationTest::initialData;

float ValidationTest::test_mass_conservation(std::vector<float>& finalData)
{
    float mass_before = calculate_total_mass(initialData);
    float mass_after = calculate_total_mass(finalData);

    float error = std::abs(mass_after - mass_before) / mass_before;
    return error;
}

bool ValidationTest::test_height_limits(std::vector<float>& finalData)
{
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

float ValidationTest::calculate_total_mass(const std::vector<float>& data)
{
    float total = 0.0f;
    for (float h : data) {
        total += h;
    }
    return total;
}

std::string ValidationTest::variant_to_string(ThermalVariant variant)
{
    switch (variant) {
        case ThermalVariant::PureTwoPhase:
            return "pureTwoPhase";
        case ThermalVariant::BlockedPureTwoPhase:
            return "blockedPureTwoPhase";
        case ThermalVariant::BlockedParallelPureTwoPhase:
            return "blockedParallelPureTwoPhase";
    }
    return "unknown";
}

int ValidationTest::run_one_step(ThermalErosion& erosion, ThermalVariant variant)
{
    switch (variant) {
        case ThermalVariant::PureTwoPhase:
            return erosion.stepPureTwoPhase();

        case ThermalVariant::BlockedPureTwoPhase:
            return erosion.stepBlockedPureTwoPhase();

        case ThermalVariant::BlockedParallelPureTwoPhase:
            return erosion.stepBlockedParallelPureTwoPhase();
    }

    return 0;
}

void ValidationTest::run_variant_tests(std::unique_ptr<Terrain>& terrain,
                              const std::vector<float>& referenceData,
                              const std::string& terrainType,
                              int steps,
                              ThermalVariant variant,
                              NeighborhoodMode neighborhood)
{
    namespace fs = std::filesystem;

    const std::string variantName = variant_to_string(variant);
    fs::path baseDir = fs::path("./resultat")
                 / terrainType
                 / neighborhood_to_string(neighborhood)
                 / variantName;
    fs::create_directories(baseDir);

    *terrain->getData() = referenceData;
    initialData = *terrain->getData();

    ThermalErosion erosion;
    erosion.loadTerrainInfo(terrain);
    erosion.setTalusAngle(25.f);
    erosion.setTransferRate(0.1f);
    if (neighborhood == NeighborhoodMode::FourNeighbors) {
        erosion.useFourNeighbors();
    } else {
        erosion.useEightNeighbors();
    }
    std::vector<int> cellsModified(steps, 0);

    fs::path errorEvolutionFile = baseDir / "error_evolution.csv";
    std::ofstream errorEvolutionOut(errorEvolutionFile);
    errorEvolutionOut << "step,error\n";

    using clock = std::chrono::high_resolution_clock;
    auto t0 = clock::now();

    float errorStep1 = 0.0f;

    for (int i = 0; i < steps; ++i)
    {
        cellsModified[i] = run_one_step(erosion, variant);

        float currentError = test_mass_conservation(*terrain->getData());

        if (i == 0) {
            errorStep1 = currentError;
        }

        if (i % 100 == 0 || i == steps - 1) {
            errorEvolutionOut << (i + 1) << "," << currentError << "\n";
        }
    }

    auto t1 = clock::now();
    const double totalMs =
        std::chrono::duration<double, std::milli>(t1 - t0).count();

    errorEvolutionOut.close();

    fs::path cellFile = baseDir / ("cellModified_" + std::to_string(steps) + ".csv");
    std::ofstream cellOut(cellFile);
    cellOut << "step,cells_modified\n";

    for (int i = 0; i < steps; ++i) {
        cellOut << (i + 1) << "," << cellsModified[i] << "\n";
    }

    cellOut.close();

    float errorFinal = test_mass_conservation(*terrain->getData());

    fs::path errorFile = baseDir / ("errorTimeStep_" + std::to_string(steps) + ".csv");
    std::ofstream errorOut(errorFile);
    errorOut << "final_error\n";
    errorOut << errorFinal << "\n";
    errorOut.close();

    fs::path perfFile = baseDir / ("performance_" + std::to_string(steps) + ".csv");
    std::ofstream perfOut(perfFile);
    perfOut << "steps,total_time_ms,avg_time_per_step_ms\n";
    perfOut << steps << "," << totalMs << "," << (totalMs / steps) << "\n";
    perfOut.close();

    int passed = 0;
    const int total = 2;

    if (errorFinal < TOLERANCE)
        ++passed;

    if (test_height_limits(*terrain->getData()))
        ++passed;

    std::cout << "========================================\n";
    std::cout << "VARIANTE : " << variantName << "\n";
    std::cout << "Terrain  : " << terrainType << "\n";
    std::cout << "Voisinage: " << neighborhood_to_string(neighborhood) << "\n";
    std::cout << "Steps    : " << steps << "\n";
    std::cout << "Tests    : Passed " << passed << " / " << total << "\n";
    std::cout << "Temps total (ms)          : " << totalMs << "\n";
    std::cout << "Temps moyen / step (ms)   : " << (totalMs / steps) << "\n";
    std::cout << "Conservation error step 1 : " << errorStep1 << "\n";
    std::cout << "Conservation error final  : " << errorFinal << "\n";
    std::cout << "Cellules modifiees step 1 : " << cellsModified.front() << "\n";
    std::cout << "Cellules modifiees fin    : " << cellsModified.back() << "\n";
    std::cout << "Dossier sortie            : " << baseDir << "\n";
    std::cout << "========================================\n";
}

void ValidationTest::run_all_tests(std::unique_ptr<Terrain>& terrain,
                                   const std::string& terrainType,
                                   int steps)
{
    if (!terrain || !terrain->getData() || terrain->getData()->empty()) {
        std::cerr << "Erreur : terrain invalide dans run_all_tests.\n";
        return;
    }

    const std::vector<float> referenceData = *terrain->getData();

    const NeighborhoodMode neighborhoods[] = {
        NeighborhoodMode::EightNeighbors,
        NeighborhoodMode::FourNeighbors
    };

    const ThermalVariant variants[] = {
        ThermalVariant::PureTwoPhase,
        ThermalVariant::BlockedPureTwoPhase,
        ThermalVariant::BlockedParallelPureTwoPhase
    };

    for (NeighborhoodMode neighborhood : neighborhoods) {
        for (ThermalVariant variant : variants) {
            run_variant_tests(terrain,
                              referenceData,
                              terrainType,
                              steps,
                              variant,
                              neighborhood);
        }
    }
}
std::string ValidationTest::neighborhood_to_string(NeighborhoodMode mode)
{
    switch (mode) {
        case NeighborhoodMode::FourNeighbors:
            return "4neighbors";
        case NeighborhoodMode::EightNeighbors:
            return "8neighbors";
    }
    return "unknownNeighbors";
}