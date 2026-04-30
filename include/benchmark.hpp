#ifndef BENCHMARK_HPP
#define BENCHMARK_HPP

#include "ThermalErosion.hpp"
#include "Terrain.hpp"
#include <string>
#include <vector>
#include <memory>

struct BenchmarkStats {
    double mean;
    double median;
    double stddev;
    double min;
    double max;
    double q1;
    double q3;
    double iqr;
    double ci95_low;
    double ci95_high;
    int n;
};

struct BenchmarkResult {
    std::string methodName;
    int methodId;
    int terrainSize;
    int steps;
    int threads;
    std::vector<double> timesMs;
    std::vector<int> cellsModified;
    std::vector<double> massErrors;
    
    BenchmarkStats timeStats;
    BenchmarkStats cellsStats;
    BenchmarkStats errorStats;
    double speedup;
    double efficiency;
};

class Benchmark {
public:
    static void runAll(int size, int steps);
    static void runWeakScaling(int baseSize, int steps);
    static void saveToCSV(const std::vector<BenchmarkResult>& results, const std::string& filename);
    
private:
    static constexpr int WARMUP_RUNS = 3;
    static constexpr int MEASURED_RUNS = 5;
    
    static std::unique_ptr<Terrain> createPerlinTerrain(int size);
    static BenchmarkStats computeStats(const std::vector<double>& values);
    static double computeMassError(const std::vector<float>& initial, const std::vector<float>& final);
    
    static BenchmarkResult runMethod(int methodId, int terrainSize, int steps, int threads = 0);
    static void printResults(const std::vector<BenchmarkResult>& results);
};

#endif