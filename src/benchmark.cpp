#include "benchmark.hpp"
#include "PerlinNoiseTerrain.hpp"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <omp.h>
#include <chrono>

// ============================================================
// IMPLEMENTATION DES METHODES DE BENCHMARK
// ============================================================

std::unique_ptr<Terrain> Benchmark::createPerlinTerrain(int size)
{
    auto terrain = std::make_unique<PerlinNoiseTerrain>();
    terrain->CreatePerlinNoise(size, size, 0, 100, 1.0f, 0.01f);
    return terrain;
}

BenchmarkStats Benchmark::computeStats(const std::vector<double>& values)
{
    BenchmarkStats stats;
    stats.n = values.size();
    
    if (values.empty()) return stats;
    
    std::vector<double> sorted = values;
    std::sort(sorted.begin(), sorted.end());
    
    stats.min = sorted.front();
    stats.max = sorted.back();
    stats.mean = std::accumulate(sorted.begin(), sorted.end(), 0.0) / sorted.size();
    
    if (sorted.size() % 2 == 0) {
        stats.median = (sorted[sorted.size()/2 - 1] + sorted[sorted.size()/2]) / 2.0;
    } else {
        stats.median = sorted[sorted.size()/2];
    }
    
    int q1_idx = sorted.size() / 4;
    int q3_idx = 3 * sorted.size() / 4;
    stats.q1 = sorted[q1_idx];
    stats.q3 = sorted[q3_idx];
    stats.iqr = stats.q3 - stats.q1;
    
    double variance = 0.0;
    for (double x : sorted) {
        variance += (x - stats.mean) * (x - stats.mean);
    }
    variance /= (sorted.size() - 1);
    stats.stddev = std::sqrt(variance);
    
    double margin = 1.96 * stats.stddev / std::sqrt(stats.n);
    stats.ci95_low = stats.mean - margin;
    stats.ci95_high = stats.mean + margin;
    
    return stats;
}

double Benchmark::computeMassError(const std::vector<float>& initial,
                                   const std::vector<float>& final)
{
    double sumInitial = 0.0, sumFinal = 0.0;
    for (float v : initial) sumInitial += v;
    for (float v : final) sumFinal += v;
    return std::abs(sumFinal - sumInitial) / (sumInitial + 1e-10);
}

BenchmarkResult Benchmark::runMethod(int methodId, int terrainSize, int steps, int threads)
{
    const char* methodNames[] = {
        "Pure two-phase",
        "Blocked pure two-phase",
        "Blocked parallel pure two-phase",
        "Checkerboard pure two-phase",
        "Blocked checkerboard pure two-phase",
        "Checkerboard in-place",
        "Checkerboard in-place parallel"
    };
    
    BenchmarkResult result;
    result.methodName = methodNames[methodId];
    result.methodId = methodId;
    result.terrainSize = terrainSize;
    result.steps = steps;
    result.threads = (threads > 0) ? threads : omp_get_max_threads();
    
    int originalThreads = omp_get_max_threads();
    if (threads > 0) omp_set_num_threads(threads);
    
    std::cout << "  " << methodNames[methodId] << " (" << terrainSize << "x" << terrainSize 
              << ", " << steps << " steps, " << result.threads << " threads)..." << std::flush;
    
    for (int run = 0; run < WARMUP_RUNS; ++run) {
        auto terrain = createPerlinTerrain(terrainSize);
        ThermalErosion erosion;
        erosion.loadTerrainInfo(*terrain);
        erosion.setTalusAngle(30.0f);
        erosion.setTransferRate(0.1f);
        
        if (methodId >= 3) {
            erosion.useFourNeighbors();
        } else {
            erosion.useEightNeighbors();
        }
        erosion.resetProgress();
        
        for (int i = 0; i < steps; ++i) {
            switch (methodId) {
                case 0: erosion.stepPureTwoPhase(); break;
                case 1: erosion.stepBlockedPureTwoPhase(); break;
                case 2: erosion.stepBlockedParallelPureTwoPhase(); break;
                case 3: erosion.stepCheckerboardPureTwoPhase(); break;
                case 4: erosion.stepBlockedCheckerboardPureTwoPhase(); break;
                case 5: erosion.stepCheckerboardInPlace(); break;
                case 6: erosion.stepCheckerboardInPlaceParallel(); break;
            }
        }
    }
    
    for (int run = 0; run < MEASURED_RUNS; ++run) {
        auto terrain = createPerlinTerrain(terrainSize);
        std::vector<float> initialData = *terrain->getData();
        
        ThermalErosion erosion;
        erosion.loadTerrainInfo(*terrain);
        erosion.setTalusAngle(30.0f);
        erosion.setTransferRate(0.1f);
        
        if (methodId >= 3) {
            erosion.useFourNeighbors();
        } else {
            erosion.useEightNeighbors();
        }
        erosion.resetProgress();
        
        auto start = std::chrono::high_resolution_clock::now();
        
        int totalChanges = 0;
        for (int i = 0; i < steps; ++i) {
            switch (methodId) {
                case 0: totalChanges += erosion.stepPureTwoPhase(); break;
                case 1: totalChanges += erosion.stepBlockedPureTwoPhase(); break;
                case 2: totalChanges += erosion.stepBlockedParallelPureTwoPhase(); break;
                case 3: totalChanges += erosion.stepCheckerboardPureTwoPhase(); break;
                case 4: totalChanges += erosion.stepBlockedCheckerboardPureTwoPhase(); break;
                case 5: totalChanges += erosion.stepCheckerboardInPlace(); break;
                case 6: totalChanges += erosion.stepCheckerboardInPlaceParallel(); break;
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        double timeMs = std::chrono::duration<double, std::milli>(end - start).count();
        double massError = computeMassError(initialData, *terrain->getData());
        
        result.timesMs.push_back(timeMs);
        result.cellsModified.push_back(totalChanges);
        result.massErrors.push_back(massError);
    }
    
    result.timeStats = computeStats(result.timesMs);
    result.cellsStats = computeStats(std::vector<double>(result.cellsModified.begin(), result.cellsModified.end()));
    result.errorStats = computeStats(result.massErrors);
    
    std::cout << " " << std::fixed << std::setprecision(0) << result.timeStats.mean << " +- " 
              << result.timeStats.stddev << " ms" << std::endl;
    
    if (threads > 0) omp_set_num_threads(originalThreads);
    
    return result;
}

void Benchmark::saveToCSV(const std::vector<BenchmarkResult>& results, const std::string& filename)
{
    std::ofstream file(filename);
    file << "method,method_id,size,steps,threads,"
         << "time_mean_ms,time_median_ms,time_stddev_ms,time_min_ms,time_max_ms,"
         << "time_q1_ms,time_q3_ms,time_ci95_low,time_ci95_high,"
         << "cells_mean,cells_stddev,mass_error_mean,mass_error_stddev\n";
    
    for (const auto& r : results) {
        file << r.methodName << ","
             << r.methodId << ","
             << r.terrainSize << ","
             << r.steps << ","
             << r.threads << ","
             << r.timeStats.mean << ","
             << r.timeStats.median << ","
             << r.timeStats.stddev << ","
             << r.timeStats.min << ","
             << r.timeStats.max << ","
             << r.timeStats.q1 << ","
             << r.timeStats.q3 << ","
             << r.timeStats.ci95_low << ","
             << r.timeStats.ci95_high << ","
             << r.cellsStats.mean << ","
             << r.cellsStats.stddev << ","
             << r.errorStats.mean << ","
             << r.errorStats.stddev << "\n";
    }
    file.close();
    std::cout << "Saved: " << filename << std::endl;
}

void Benchmark::printResults(const std::vector<BenchmarkResult>& results)
{
    std::cout << "\n" << std::string(100, '-') << std::endl;
    std::cout << std::left << std::setw(32) << "Method"
              << std::right << std::setw(10) << "Mean"
              << std::setw(10) << "Median"
              << std::setw(10) << "StdDev"
              << std::setw(10) << "Min"
              << std::setw(10) << "Max"
              << std::setw(12) << "Q1"
              << std::setw(12) << "Q3"
              << std::setw(10) << "Speedup"
              << std::endl;
    std::cout << std::string(100, '-') << std::endl;
    
    double refTime = results[0].timeStats.mean;
    
    for (const auto& r : results) {
        double speedup = refTime / r.timeStats.mean;
        std::cout << std::left << std::setw(32) << r.methodName
                  << std::right << std::setw(10) << std::fixed << std::setprecision(0) << r.timeStats.mean
                  << std::setw(10) << r.timeStats.median
                  << std::setw(10) << r.timeStats.stddev
                  << std::setw(10) << r.timeStats.min
                  << std::setw(10) << r.timeStats.max
                  << std::setw(12) << r.timeStats.q1
                  << std::setw(12) << r.timeStats.q3
                  << std::setw(10) << std::setprecision(2) << speedup << "x"
                  << std::endl;
    }
    std::cout << std::string(100, '-') << std::endl;
}

void Benchmark::runWeakScaling(int baseSize, int steps)
{
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "WEAK SCALING - OPENMP" << std::endl;
    std::cout << std::string(80, '=') << std::endl;
    std::cout << "Base size (1 thread): " << baseSize << "x" << baseSize << std::endl;
    std::cout << "Steps: " << steps << std::endl;
    std::cout << std::string(60, '-') << std::endl;
    
    std::vector<int> threadCounts = {1, 2, 4, 8, 12, 16};
    std::vector<BenchmarkResult> weakResults;
    
    for (int threads : threadCounts) {
        int size = baseSize * (int)std::sqrt(threads);
        size = (size / 2) * 2;
        if (size < 64) size = 64;
        
        std::cout << "  Threads: " << threads << " | Size: " << size << "x" << size << std::flush;
        
        int originalThreads = omp_get_max_threads();
        omp_set_num_threads(threads);
        
        auto terrain = createPerlinTerrain(size);
        std::vector<float> initialData = *terrain->getData();
        
        ThermalErosion erosion;
        erosion.loadTerrainInfo(*terrain);
        erosion.setTalusAngle(30.0f);
        erosion.setTransferRate(0.1f);
        erosion.useEightNeighbors();
        erosion.resetProgress();
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < steps; ++i) {
            erosion.stepBlockedParallelPureTwoPhase();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        double timeMs = std::chrono::duration<double, std::milli>(end - start).count();
        double massError = computeMassError(initialData, *terrain->getData());
        
        BenchmarkResult result;
        result.methodName = "Weak scaling";
        result.methodId = 2;
        result.terrainSize = size;
        result.steps = steps;
        result.threads = threads;
        result.timeStats.mean = timeMs;
        result.timeStats.stddev = 0;
        result.massErrors.push_back(massError);
        
        weakResults.push_back(result);
        
        std::cout << " | Time: " << std::fixed << std::setprecision(0) << timeMs << " ms" << std::endl;
        
        omp_set_num_threads(originalThreads);
    }
    
    saveToCSV(weakResults, "weak_scaling_results.csv");
    
    double baseTime = weakResults[0].timeStats.mean;
    std::cout << "\n" << std::string(60, '-') << std::endl;
    std::cout << std::left << std::setw(10) << "Threads"
              << std::right << std::setw(12) << "Size"
              << std::setw(14) << "Time (ms)"
              << std::setw(14) << "Efficiency"
              << std::endl;
    std::cout << std::string(60, '-') << std::endl;
    
    for (const auto& r : weakResults) {
        double efficiency = (baseTime / r.timeStats.mean) * 100.0;
        std::cout << std::left << std::setw(10) << r.threads
                  << std::right << std::setw(8) << r.terrainSize << "x" << std::setw(4) << r.terrainSize
                  << std::setw(14) << std::fixed << std::setprecision(0) << r.timeStats.mean
                  << std::setw(13) << std::setprecision(1) << efficiency << "%"
                  << std::endl;
    }
    std::cout << std::string(60, '-') << std::endl;
}

void Benchmark::runAll(int size, int steps)
{
    std::vector<BenchmarkResult> results;
    
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "BENCHMARK THERMAL EROSION - PERLIN NOISE" << std::endl;
    std::cout << std::string(80, '=') << std::endl;
    std::cout << "Runs: " << MEASURED_RUNS << " measured, " << WARMUP_RUNS << " warmup" << std::endl;
    std::cout << "Terrain: " << size << "x" << size << ", " << steps << " steps" << std::endl;
    std::cout << std::string(80, '=') << "\n" << std::endl;
    
    // TEST A: 7 methods comparison
    
    std::vector<BenchmarkResult> testAResults;
    
    int bestMethod = 2;
    double bestSpeedup = 1.0;
    
    // TEST C: Different sizes
    
    std::vector<int> testSizes = {128, 256, 512, 1024, 2048, 4096};
    std::vector<BenchmarkResult> testCResults;
    
    for (int s : testSizes) {
        if (s <= size * 2) {
            testCResults.push_back(runMethod(bestMethod, s, steps, 2));
        }
    }
    
    saveToCSV(testCResults, "benchmark_testC_size_scalability.csv");
    
    std::cout << "\nSIZE SCALABILITY" << std::endl;
    std::cout << std::string(70, '-') << std::endl;
    std::cout << std::left << std::setw(12) << "Size"
              << std::right << std::setw(14) << "Time (ms)"
              << std::setw(14) << "Cells (M)"
              << std::setw(14) << "ms/Mcell"
              << std::setw(14) << "Speedup"
              << std::endl;
    std::cout << std::string(70, '-') << std::endl;
    
    double baseTimeSize = testCResults[0].timeStats.mean;
    for (const auto& r : testCResults) {
        double millionCells = (r.terrainSize * r.terrainSize) / 1e6;
        double msPerMcell = r.timeStats.mean / millionCells;
        double speedup = baseTimeSize / r.timeStats.mean;
        std::cout << std::left << std::setw(12) << (std::to_string(r.terrainSize) + "x" + std::to_string(r.terrainSize))
                  << std::right << std::setw(14) << std::fixed << std::setprecision(0) << r.timeStats.mean
                  << std::setw(14) << std::setprecision(2) << millionCells
                  << std::setw(14) << std::setprecision(2) << msPerMcell
                  << std::setw(14) << std::setprecision(2) << speedup << "x"
                  << std::endl;
    }
    std::cout << std::string(70, '-') << std::endl;
    
    // WEAK SCALING
    runWeakScaling(256, steps);
    
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "BENCHMARK COMPLETED" << std::endl;
    std::cout << "Best method: " << testAResults[bestMethod].methodName << std::endl;
    std::cout << "Best threads: " << 2 << std::endl;
    std::cout << "Max speedup: " << bestSpeedup << "x" << std::endl;
    std::cout << std::string(80, '=') << std::endl;
}

// ============================================================
// MAIN
// ============================================================

int main(int argc, char* argv[])
{
    int size = 2048;
    int steps = 500;
    
    if (argc > 1) size = std::atoi(argv[1]);
    if (argc > 2) steps = std::atoi(argv[2]);
    
    std::cout << "Benchmark with terrain " << size << "x" << size 
              << ", " << steps << " steps" << std::endl;
    
    Benchmark::runAll(size, steps);
    return 0;
}