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
        BlockedParallelPureTwoPhase
    };

    enum class NeighborhoodMode {
        FourNeighbors,
        EightNeighbors
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

    static void run_variant_tests(std::unique_ptr<Terrain>& terrain,
                              const std::vector<float>& referenceData,
                              const std::string& terrainType,
                              int steps,
                              ThermalVariant variant,
                              NeighborhoodMode neighborhood);
};