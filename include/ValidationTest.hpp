#pragma once

#include "Terrain.hpp"
#include "ThermalErosion.hpp"
#include <memory>
#include <string>
#include <vector>

class ValidationTest
{
public:
    enum class ThermalVariant {
        PureTwoPhase,
        BlockedPureTwoPhase,
        BlockedParallelPureTwoPhase,
        CheckerboardPureTwoPhase,
        BlockedCheckerboardPureTwoPhase,
        CheckerboardInPlace,
        CheckerboardInPlaceParallel
    };

    enum class NeighborhoodMode {
        FourNeighbors,
        EightNeighbors
    };

    struct RunMetrics
    {
        double totalTimeMs = 0.0;
        double avgTimePerStepMs = 0.0;
        float finalMassError = 0.0f;
        int lastCellsModified = 0;
    };

    struct SummaryStats
    {
        int n = 0;
        double mean = 0.0;
        double median = 0.0;
        double stddev = 0.0;
        double min = 0.0;
        double max = 0.0;
        double ci95Low = 0.0;
        double ci95High = 0.0;
    };

    static std::vector<float> initialData;
    static constexpr float TOLERANCE = 1e-4f;

    static float test_mass_conservation(std::vector<float>& finalData);
    static bool test_height_limits(std::vector<float>& finalData);
    static float calculate_total_mass(const std::vector<float>& data);

    static void run_all_tests(std::unique_ptr<Terrain>& terrain,
                              const std::string& terrainType,
                              int steps);

private:
    static int run_one_step(ThermalErosion& erosion, ThermalVariant variant);
    static std::string variant_to_string(ThermalVariant variant);
    static std::string neighborhood_to_string(NeighborhoodMode mode);

    static SummaryStats compute_summary_stats(const std::vector<double>& values);

    static void write_raw_runs_csv(const std::string& filepath,
                                   const std::vector<RunMetrics>& runs);

    static void write_summary_csv(const std::string& filepath,
                                  const SummaryStats& totalStats,
                                  const SummaryStats& avgStepStats,
                                  const SummaryStats& massErrorStats,
                                  const SummaryStats& lastCellsStats);

    static void run_variant_tests(std::unique_ptr<Terrain>& terrain,
                                  const std::vector<float>& referenceData,
                                  const std::string& terrainType,
                                  int steps,
                                  ThermalVariant variant,
                                  NeighborhoodMode neighborhood);
};