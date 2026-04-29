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
// IMPLÉMENTATION DES MÉTHODES DE BENCHMARK
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
    
    // Warm-up
    for (int run = 0; run < WARMUP_RUNS; ++run) {
        auto terrain = createPerlinTerrain(terrainSize);
        ThermalErosion erosion;
        erosion.loadTerrainInfo(*terrain);
        erosion.setTalusAngle(30.0f);
        erosion.setTransferRate(0.1f);
        
        if (methodId >= 3) {  // 3,4,5,6 sont des méthodes checkerboard
            erosion.useFourNeighbors();
            std::cout << " (4 voisins)" << std::flush;
        } else {
            erosion.useEightNeighbors();
            std::cout << " (8 voisins)" << std::flush;
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
    
    // Mesures
    for (int run = 0; run < MEASURED_RUNS; ++run) {
        auto terrain = createPerlinTerrain(terrainSize);
        std::vector<float> initialData = *terrain->getData();
        
        ThermalErosion erosion;
        erosion.loadTerrainInfo(*terrain);
        erosion.setTalusAngle(30.0f);
        erosion.setTransferRate(0.1f);
        
        if (methodId >= 3) {  // 3,4,5,6 sont des méthodes checkerboard
            erosion.useFourNeighbors();
            std::cout << " (4 voisins)" << std::flush;
        } else {
            erosion.useEightNeighbors();
            std::cout << " (8 voisins)" << std::flush;
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
    
    std::cout << " " << std::fixed << std::setprecision(0) << result.timeStats.mean << "±" 
              << result.timeStats.stddev << " ms" << std::endl;
    
    if (threads > 0) omp_set_num_threads(originalThreads);
    
    return result;
}

void Benchmark::printResults(const std::vector<BenchmarkResult>& results)
{
    std::cout << "\n" << std::string(100, '-') << std::endl;
    std::cout << std::left << std::setw(32) << "Méthode"
              << std::right << std::setw(10) << "Moyenne"
              << std::setw(10) << "Médiane"
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

void Benchmark::saveToCSV(const std::vector<BenchmarkResult>& results, const std::string& filename)
{
    std::ofstream file(filename);
    file << "method,method_id,size,steps,threads,"
         << "time_mean_ms,time_median_ms,time_stddev_ms,time_min_ms,time_max_ms,time_q1_ms,time_q3_ms,"
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
             << r.cellsStats.mean << ","
             << r.cellsStats.stddev << ","
             << r.errorStats.mean << ","
             << r.errorStats.stddev << "\n";
    }
    file.close();
    std::cout << "\n✅ Résultats sauvegardés dans " << filename << std::endl;
}

void Benchmark::runAll(int size, int steps)
{
    std::vector<BenchmarkResult> results;
    
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "🔬 BENCHMARK ÉROSION THERMIQUE - PERLIN NOISE" << std::endl;
    std::cout << std::string(80, '=') << std::endl;
    std::cout << "📊 " << MEASURED_RUNS << " runs mesurés, " << WARMUP_RUNS << " warmup" << std::endl;
    std::cout << "📊 Terrain: " << size << "x" << size << ", " << steps << " étapes" << std::endl;
    std::cout << std::string(80, '=') << "\n" << std::endl;
    
    // ============================================================
    // TEST A: Comparaison des 7 méthodes (12 threads)
    // ============================================================
    std::cout << "📊 TEST A: COMPARAISON DES 7 MÉTHODES" << std::endl;
    std::cout << "   (" << size << "x" << size << ", " << steps << " étapes, 12 threads)" << std::endl;
    std::cout << std::string(60, '-') << std::endl;
    
    for (int method = 0; method < 7; ++method) {
        results.push_back(runMethod(method, size, steps));
    }
    
    printResults(results);
    
    // Identifier la meilleure méthode (celle avec le meilleur speedup)
    double refTime = results[0].timeStats.mean;
    int bestMethod = 0;
    double bestSpeedup = 1.0;
    for (const auto& r : results) {
        double speedup = refTime / r.timeStats.mean;
        if (speedup > bestSpeedup) {
            bestSpeedup = speedup;
            bestMethod = r.methodId;
        }
    }
    
    std::cout << "\n🏆 Meilleure méthode identifiée: " << results[bestMethod].methodName 
              << " (speedup " << bestSpeedup << "x)" << std::endl;
    
    // ============================================================
    // TEST B: Scalabilité OpenMP - meilleure méthode
    // ============================================================
    std::cout << "\n📊 TEST B: SCALABILITÉ OPENMP" << std::endl;
    std::cout << "   (" << results[bestMethod].methodName << " - meilleure méthode, " 
              << size << "x" << size << ", " << steps << " étapes)" << std::endl;
    std::cout << std::string(60, '-') << std::endl;
    
    std::vector<BenchmarkResult> scalingResults;
    std::vector<int> threadCounts = {1, 2, 4, 8, 12};
    for (int threads : threadCounts) {
        scalingResults.push_back(runMethod(bestMethod, size, steps, threads));
    }
    
    std::cout << "\n📊 SCALABILITÉ OPENMP" << std::endl;
    std::cout << std::string(60, '-') << std::endl;
    std::cout << std::left << std::setw(12) << "Threads"
              << std::right << std::setw(14) << "Temps (ms)"
              << std::setw(12) << "Speedup"
              << std::setw(12) << "Idéal"
              << std::setw(14) << "Efficacité"
              << std::endl;
    std::cout << std::string(60, '-') << std::endl;
    
    double baseTime = scalingResults[0].timeStats.mean;
    int bestThreads = 1;
    double bestThreadSpeedup = 1.0;
    for (const auto& r : scalingResults) {
        double speedup = baseTime / r.timeStats.mean;
        if (speedup > bestThreadSpeedup) {
            bestThreadSpeedup = speedup;
            bestThreads = r.threads;
        }
        double ideal = r.threads;
        double eff = (speedup / ideal) * 100.0;
        std::cout << std::left << std::setw(12) << r.threads
                  << std::right << std::setw(14) << std::fixed << std::setprecision(0) << r.timeStats.mean
                  << std::setw(12) << std::setprecision(2) << speedup << "x"
                  << std::setw(12) << std::setprecision(2) << ideal << "x"
                  << std::setw(13) << std::setprecision(1) << eff << "%"
                  << std::endl;
    }
    std::cout << std::string(60, '-') << std::endl;
    
    std::cout << "\n🏆 Meilleur nombre de threads: " << bestThreads 
              << " (speedup " << bestThreadSpeedup << "x)" << std::endl;
    
    results.insert(results.end(), scalingResults.begin(), scalingResults.end());
    
    // ============================================================
    // TEST C: Différentes tailles - meilleure méthode + meilleur nombre de threads
    // ============================================================
    std::cout << "\n📊 TEST C: DIFFÉRENTES TAILLES" << std::endl;
    std::cout << "   (" << results[bestMethod].methodName << " - meilleure méthode, " 
              << bestThreads << " threads, " << steps << " étapes)" << std::endl;
    std::cout << std::string(60, '-') << std::endl;
    
    std::vector<int> testSizes = {128, 256, 512, 1024, 2048, 4096};
    std::vector<BenchmarkResult> sizeResults;
    
    for (int s : testSizes) {
        if (s <= size * 2) {
            sizeResults.push_back(runMethod(bestMethod, s, steps, bestThreads));
        }
    }
    
    std::cout << "\n📊 SCALABILITÉ PAR TAILLE" << std::endl;
    std::cout << std::string(70, '-') << std::endl;
    std::cout << std::left << std::setw(12) << "Taille"
              << std::right << std::setw(14) << "Temps (ms)"
              << std::setw(14) << "Cellules"
              << std::setw(14) << "ms/Mcell"
              << std::setw(14) << "Speedup"
              << std::endl;
    std::cout << std::string(70, '-') << std::endl;
    
    double baseTimeSize = sizeResults[0].timeStats.mean;
    for (const auto& r : sizeResults) {
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
    
    results.insert(results.end(), sizeResults.begin(), sizeResults.end());
    
    saveToCSV(results, "benchmark_results.csv");
    
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "✅ BENCHMARK TERMINÉ" << std::endl;
    std::cout << "🏆 Meilleure méthode: " << results[bestMethod].methodName << std::endl;
    std::cout << "🏆 Meilleur nombre de threads: " << bestThreads << std::endl;
    std::cout << "🏆 Speedup maximal: " << bestSpeedup << "x" << std::endl;
    std::cout << std::string(80, '=') << std::endl;
}

// ============================================================
// MAIN
// ============================================================

int main(int argc, char* argv[])
{
    int size = 2048;    // Valeur par défaut
    int steps = 500;    // Valeur par défaut
    
    if (argc > 1) size = std::atoi(argv[1]);
    if (argc > 2) steps = std::atoi(argv[2]);
    
    std::cout << "Benchmark avec terrain " << size << "x" << size 
              << ", " << steps << " étapes" << std::endl;
    
    Benchmark::runAll(size, steps);
    return 0;
}