#include "ValidationTest.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
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
        case ThermalVariant::CheckerboardPureTwoPhase:
            return "checkerboardPureTwoPhase";
        case ThermalVariant::BlockedCheckerboardPureTwoPhase:
            return "blockedCheckerboardPureTwoPhase";
        case ThermalVariant::CheckerboardInPlace:
            return "checkerboardInPlace";
        case ThermalVariant::CheckerboardInPlaceParallel:
            return "checkerboardInPlaceParallel";
    }
    return "unknown";
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

int ValidationTest::run_one_step(ThermalErosion& erosion, ThermalVariant variant)
{
    switch (variant) {
        case ThermalVariant::PureTwoPhase:
            return erosion.stepPureTwoPhase();

        case ThermalVariant::BlockedPureTwoPhase:
            return erosion.stepBlockedPureTwoPhase();

        case ThermalVariant::BlockedParallelPureTwoPhase:
            return erosion.stepBlockedParallelPureTwoPhase();
        case ThermalVariant::CheckerboardPureTwoPhase:
            return erosion.stepCheckerboardPureTwoPhase();
        case ThermalVariant::BlockedCheckerboardPureTwoPhase:
            return erosion.stepBlockedCheckerboardPureTwoPhase();
        case ThermalVariant::CheckerboardInPlace:
            return erosion.stepCheckerboardInPlace();
        case ThermalVariant::CheckerboardInPlaceParallel:
            return erosion.stepCheckerboardInPlaceParallel();
    }

    return 0;
}

ValidationTest::SummaryStats
ValidationTest::compute_summary_stats(const std::vector<double>& values)
{
    SummaryStats stats;

    if (values.empty()) {
        return stats;
    }

    stats.n = static_cast<int>(values.size());

    std::vector<double> sorted = values;
    std::sort(sorted.begin(), sorted.end());

    stats.min = sorted.front();
    stats.max = sorted.back();

    const double sum = std::accumulate(sorted.begin(), sorted.end(), 0.0);
    stats.mean = sum / static_cast<double>(sorted.size());

    if (sorted.size() % 2 == 0) {
        const std::size_t mid = sorted.size() / 2;
        stats.median = 0.5 * (sorted[mid - 1] + sorted[mid]);
    } else {
        stats.median = sorted[sorted.size() / 2];
    }

    double variance = 0.0;
    if (sorted.size() > 1) {
        for (double x : sorted) {
            const double d = x - stats.mean;
            variance += d * d;
        }
        variance /= static_cast<double>(sorted.size() - 1);
        stats.stddev = std::sqrt(variance);
    } else {
        stats.stddev = 0.0;
    }

    const double margin = 1.96 * stats.stddev / std::sqrt(static_cast<double>(stats.n));
    stats.ci95Low = stats.mean - margin;
    stats.ci95High = stats.mean + margin;

    return stats;
}

void ValidationTest::write_raw_runs_csv(const std::string& filepath,
                                        const std::vector<RunMetrics>& runs)
{
    std::ofstream out(filepath);
    out << "run_id,total_time_ms,avg_time_per_step_ms,final_mass_error,last_cells_modified\n";

    for (std::size_t i = 0; i < runs.size(); ++i) {
        out << (i + 1) << ","
            << runs[i].totalTimeMs << ","
            << runs[i].avgTimePerStepMs << ","
            << runs[i].finalMassError << ","
            << runs[i].lastCellsModified << "\n";
    }
}

void ValidationTest::write_summary_csv(const std::string& filepath,
                                       const SummaryStats& totalStats,
                                       const SummaryStats& avgStepStats,
                                       const SummaryStats& massErrorStats,
                                       const SummaryStats& lastCellsStats)
{
    std::ofstream out(filepath);
    out << "metric,n,mean,median,stddev,min,max,ci95_low,ci95_high\n";

    out << "total_time_ms,"
        << totalStats.n << ","
        << totalStats.mean << ","
        << totalStats.median << ","
        << totalStats.stddev << ","
        << totalStats.min << ","
        << totalStats.max << ","
        << totalStats.ci95Low << ","
        << totalStats.ci95High << "\n";

    out << "avg_time_per_step_ms,"
        << avgStepStats.n << ","
        << avgStepStats.mean << ","
        << avgStepStats.median << ","
        << avgStepStats.stddev << ","
        << avgStepStats.min << ","
        << avgStepStats.max << ","
        << avgStepStats.ci95Low << ","
        << avgStepStats.ci95High << "\n";

    out << "final_mass_error,"
        << massErrorStats.n << ","
        << massErrorStats.mean << ","
        << massErrorStats.median << ","
        << massErrorStats.stddev << ","
        << massErrorStats.min << ","
        << massErrorStats.max << ","
        << massErrorStats.ci95Low << ","
        << massErrorStats.ci95High << "\n";

    out << "last_cells_modified,"
        << lastCellsStats.n << ","
        << lastCellsStats.mean << ","
        << lastCellsStats.median << ","
        << lastCellsStats.stddev << ","
        << lastCellsStats.min << ","
        << lastCellsStats.max << ","
        << lastCellsStats.ci95Low << ","
        << lastCellsStats.ci95High << "\n";
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
    const std::string neighborhoodName = neighborhood_to_string(neighborhood);

    fs::path baseDir = fs::path("./resultat")
                     / terrainType
                     / neighborhoodName
                     / variantName;
    fs::create_directories(baseDir);

    constexpr int warmupRuns = 3;
    constexpr int measuredRuns = 20;

    std::vector<RunMetrics> measured;
    measured.reserve(measuredRuns);

    float referenceErrorStep1 = 0.0f;
    int referenceCellsStep1 = 0;
    int validationPassed = 0;

    fs::path errorEvolutionFile = baseDir / "error_evolution_last_run.csv";

    for (int run = 0; run < warmupRuns + measuredRuns; ++run)
    {
        const bool isWarmup = (run < warmupRuns);

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
        std::ofstream errorEvolutionOut;

        if (!isWarmup && run == warmupRuns + measuredRuns - 1) {
            errorEvolutionOut.open(errorEvolutionFile);
            errorEvolutionOut << "step,error\n";
        }

        using clock = std::chrono::high_resolution_clock;
        auto t0 = clock::now();

        float errorStep1 = 0.0f;

        for (int i = 0; i < steps; ++i)
        {
            cellsModified[i] = run_one_step(erosion, variant);

            const float currentError = test_mass_conservation(*terrain->getData());

            if (i == 0) {
                errorStep1 = currentError;
            }

            if (errorEvolutionOut.is_open() && (i % 100 == 0 || i == steps - 1)) {
                errorEvolutionOut << (i + 1) << "," << currentError << "\n";
            }
        }

        auto t1 = clock::now();
        const double totalMs =
            std::chrono::duration<double, std::milli>(t1 - t0).count();

        const float finalError = test_mass_conservation(*terrain->getData());

        if (!isWarmup) {
            RunMetrics m;
            m.totalTimeMs = totalMs;
            m.avgTimePerStepMs = totalMs / static_cast<double>(steps);
            m.finalMassError = finalError;
            m.lastCellsModified = cellsModified.back();
            measured.push_back(m);

            if (measured.size() == 1) {
                referenceErrorStep1 = errorStep1;
                referenceCellsStep1 = cellsModified.front();

                validationPassed = 0;
                if (finalError < TOLERANCE)
                    ++validationPassed;
                if (test_height_limits(*terrain->getData()))
                    ++validationPassed;
            }
        }
    }

    std::vector<double> totalTimes;
    std::vector<double> avgStepTimes;
    std::vector<double> finalMassErrors;
    std::vector<double> lastCells;

    totalTimes.reserve(measured.size());
    avgStepTimes.reserve(measured.size());
    finalMassErrors.reserve(measured.size());
    lastCells.reserve(measured.size());

    for (const RunMetrics& m : measured) {
        totalTimes.push_back(m.totalTimeMs);
        avgStepTimes.push_back(m.avgTimePerStepMs);
        finalMassErrors.push_back(static_cast<double>(m.finalMassError));
        lastCells.push_back(static_cast<double>(m.lastCellsModified));
    }

    const SummaryStats totalStats = compute_summary_stats(totalTimes);
    const SummaryStats avgStepStats = compute_summary_stats(avgStepTimes);
    const SummaryStats massErrorStats = compute_summary_stats(finalMassErrors);
    const SummaryStats lastCellsStats = compute_summary_stats(lastCells);

    write_raw_runs_csv((baseDir / "raw_runs.csv").string(), measured);
    write_summary_csv((baseDir / "summary_stats.csv").string(),
                      totalStats,
                      avgStepStats,
                      massErrorStats,
                      lastCellsStats);

    std::cout << "========================================\n";
    std::cout << "VARIANTE : " << variantName << "\n";
    std::cout << "Terrain  : " << terrainType << "\n";
    std::cout << "Voisinage: " << neighborhoodName << "\n";
    std::cout << "Steps    : " << steps << "\n";
    std::cout << "Warm-up  : " << warmupRuns << "\n";
    std::cout << "Runs     : " << measuredRuns << "\n";
    std::cout << "Tests    : Passed " << validationPassed << " / 2\n";
    std::cout << "Mean total time (ms)       : " << totalStats.mean << "\n";
    std::cout << "Median total time (ms)     : " << totalStats.median << "\n";
    std::cout << "Stddev total time (ms)     : " << totalStats.stddev << "\n";
    std::cout << "Min total time (ms)        : " << totalStats.min << "\n";
    std::cout << "Max total time (ms)        : " << totalStats.max << "\n";
    std::cout << "95% CI total time (ms)     : [" << totalStats.ci95Low
              << ", " << totalStats.ci95High << "]\n";
    std::cout << "Mean time / step (ms)      : " << avgStepStats.mean << "\n";
    std::cout << "Mean final mass error      : " << massErrorStats.mean << "\n";
    std::cout << "Conservation error step 1  : " << referenceErrorStep1 << "\n";
    std::cout << "Cells modified step 1      : " << referenceCellsStep1 << "\n";
    std::cout << "Mean last cells modified   : " << lastCellsStats.mean << "\n";
    std::cout << "Dossier sortie             : " << baseDir << "\n";
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

    const ThermalVariant baseVariants[] = {
        ThermalVariant::PureTwoPhase,
        ThermalVariant::BlockedPureTwoPhase,
        ThermalVariant::BlockedParallelPureTwoPhase
    };

    for (ThermalVariant variant : baseVariants) {
        run_variant_tests(terrain, referenceData, terrainType, steps,
                          variant, NeighborhoodMode::EightNeighbors);
    }

    const ThermalVariant fourNeighborVariants[] = {
        ThermalVariant::PureTwoPhase,
        ThermalVariant::BlockedPureTwoPhase,
        ThermalVariant::BlockedParallelPureTwoPhase,
        ThermalVariant::CheckerboardPureTwoPhase,
        ThermalVariant::BlockedCheckerboardPureTwoPhase,
        ThermalVariant::CheckerboardInPlace,
        ThermalVariant::CheckerboardInPlaceParallel
    };

    for (ThermalVariant variant : fourNeighborVariants) {
        run_variant_tests(terrain, referenceData, terrainType, steps,
                          variant, NeighborhoodMode::FourNeighbors);
    }
}