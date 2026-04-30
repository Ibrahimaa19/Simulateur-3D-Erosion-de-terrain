#include "Terrain.hpp"
#include "PerlinNoiseTerrain.hpp"
#include "ThermalErosion.hpp"
#include <chrono>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <cmath>
#include <omp.h>

// Désactiver LOD et Frustum pour le benchmark
// À adapter selon ton architecture

struct BenchmarkConfig {
    int size;
    int steps;
    int warmup;
    int methodId;
    int threads;
    std::string methodName;
};

struct BenchmarkResult {
    int size;
    int steps;
    int threads;
    int methodId;
    std::string methodName;
    double totalTimeMs;
    double timePerStepMs;
    double cellsModified;
    double massError;
    double stddev;
};

std::vector<BenchmarkResult> results;

std::string getMethodName(int methodId) {
    switch(methodId) {
        case 0: return "Pure two-phase";
        case 1: return "Blocked pure";
        case 2: return "Blocked parallel";
        case 3: return "Checkerboard pure";
        case 4: return "Blocked checkerboard";
        case 5: return "Checkerboard in-place";
        case 6: return "Checkerboard parallel";
        default: return "Unknown";
    }
}

double computeMassError(const std::vector<float>& initial, const std::vector<float>& final)
{
    double sumInitial = 0.0, sumFinal = 0.0;
    for (float v : initial) sumInitial += v;
    for (float v : final) sumFinal += v;
    return std::abs(sumFinal - sumInitial) / (sumInitial + 1e-10);
}

void runBenchmark(int size, int steps, int warmup, int methodId, int threads)
{
    std::cout << "  " << getMethodName(methodId) << " (" << size << "x" << size 
              << ", " << steps << " steps, " << threads << " threads)..." << std::flush;
    
    // Sauvegarder le nombre de threads original
    int originalThreads = omp_get_max_threads();
    if (threads > 0) {
        omp_set_num_threads(threads);
    }
    
    std::vector<double> times;
    std::vector<double> errors;
    std::vector<double> cellsModified;
    
    // Warmup
    for (int w = 0; w < warmup; ++w) {
        auto terrain = std::make_unique<PerlinNoiseTerrain>();
        terrain->CreatePerlinNoise(size, size, 0, 100, 1.0f, 0.01f);
        
        ThermalErosion erosion;
        erosion.loadTerrainInfo(*terrain);
        erosion.setTalusAngle(30.0f);
        erosion.setTransferRate(0.1f);
        erosion.useEightNeighbors();
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
    
    // Mesures (5 runs)
    const int RUNS = 5;
    for (int run = 0; run < RUNS; ++run) {
        auto terrain = std::make_unique<PerlinNoiseTerrain>();
        terrain->CreatePerlinNoise(size, size, 0, 100, 1.0f, 0.01f);
        std::vector<float> initialData = *terrain->getData();
        
        ThermalErosion erosion;
        erosion.loadTerrainInfo(*terrain);
        erosion.setTalusAngle(30.0f);
        erosion.setTransferRate(0.1f);
        erosion.useEightNeighbors();
        erosion.resetProgress();
        
        auto start = std::chrono::high_resolution_clock::now();
        
        int changes = 0;
        for (int i = 0; i < steps; ++i) {
            switch (methodId) {
                case 0: changes += erosion.stepPureTwoPhase(); break;
                case 1: changes += erosion.stepBlockedPureTwoPhase(); break;
                case 2: changes += erosion.stepBlockedParallelPureTwoPhase(); break;
                case 3: changes += erosion.stepCheckerboardPureTwoPhase(); break;
                case 4: changes += erosion.stepBlockedCheckerboardPureTwoPhase(); break;
                case 5: changes += erosion.stepCheckerboardInPlace(); break;
                case 6: changes += erosion.stepCheckerboardInPlaceParallel(); break;
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        double timeMs = std::chrono::duration<double, std::milli>(end - start).count();
        double massError = computeMassError(initialData, *terrain->getData());
        
        times.push_back(timeMs);
        errors.push_back(massError);
        cellsModified.push_back(changes);
    }
    
    // Calcul des statistiques
    double mean = 0, stddev = 0;
    for (double t : times) mean += t;
    mean /= times.size();
    
    for (double t : times) {
        stddev += (t - mean) * (t - mean);
    }
    stddev = std::sqrt(stddev / (times.size() - 1));
    
    BenchmarkResult result;
    result.size = size;
    result.steps = steps;
    result.threads = threads;
    result.methodId = methodId;
    result.methodName = getMethodName(methodId);
    result.totalTimeMs = mean;
    result.timePerStepMs = mean / steps;
    result.cellsModified = cellsModified.back();
    result.massError = errors.back();
    result.stddev = stddev;
    
    results.push_back(result);
    
    std::cout << " " << std::fixed << std::setprecision(0) << mean << " ± " 
              << std::setprecision(0) << stddev << " ms" << std::endl;
    
    if (threads > 0) omp_set_num_threads(originalThreads);
}

void saveToCSV(const std::string& filename)
{
    std::ofstream file(filename);
    file << "size,steps,threads,method_id,method_name,total_time_ms,time_per_step_ms,stddev_ms,cells_modified,mass_error\n";
    
    for (const auto& r : results) {
        file << r.size << ","
             << r.steps << ","
             << r.threads << ","
             << r.methodId << ","
             << r.methodName << ","
             << r.totalTimeMs << ","
             << r.timePerStepMs << ","
             << r.stddev << ","
             << r.cellsModified << ","
             << r.massError << "\n";
    }
    file.close();
    std::cout << "\n✅ Résultats sauvegardés dans " << filename << std::endl;
}

int main(int argc, char* argv[])
{
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "🔬 BENCHMARK PUR - SIMULATION SANS RENDU" << std::endl;
    std::cout << std::string(80, '=') << std::endl;
    std::cout << "📊 LOD et Frustum désactivés" << std::endl;
    std::cout << "📊 Warmup: 60 étapes, puis 5 runs mesurés" << std::endl;
    std::cout << std::string(80, '=') << "\n" << std::endl;
    
    const int STEPS = 100;
    const int WARMUP = 60;
    const int METHOD = 2;  // Blocked parallel
    const int THREADS = 8; // 8 threads (E-cores)
    
    // Taille de terrain à tester
    std::vector<int> sizes = {128, 256, 512, 1024, 2048, 4096};
    
    std::cout << "📊 TEST: Différentes tailles de terrain" << std::endl;
    std::cout << "   Méthode: Blocked parallel, " << STEPS << " étapes, " 
              << THREADS << " threads" << std::endl;
    std::cout << "   Warmup: " << WARMUP << " étapes" << std::endl;
    std::cout << std::string(60, '-') << std::endl;
    
    for (int size : sizes) {
        runBenchmark(size, STEPS, WARMUP, METHOD, THREADS);
    }
    
    saveToCSV("benchmark_pure_simulation.csv");
    
    // Résumé
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "📊 RÉSUMÉ" << std::endl;
    std::cout << std::string(80, '=') << std::endl;
    std::cout << std::left << std::setw(12) << "Taille"
              << std::right << std::setw(14) << "Temps (ms)"
              << std::setw(14) << "ms/step"
              << std::setw(14) << "Cellules (M)"
              << std::setw(14) << "ms/Mcell"
              << std::endl;
    std::cout << std::string(60, '-') << std::endl;
    
    for (const auto& r : results) {
        double millionCells = (r.size * r.size) / 1e6;
        double msPerMcell = r.totalTimeMs / millionCells;
        std::cout << std::left << std::setw(12) << (std::to_string(r.size) + "x" + std::to_string(r.size))
                  << std::right << std::setw(14) << std::fixed << std::setprecision(0) << r.totalTimeMs
                  << std::setw(14) << std::setprecision(2) << r.timePerStepMs
                  << std::setw(14) << std::setprecision(2) << millionCells
                  << std::setw(14) << std::setprecision(0) << msPerMcell
                  << std::endl;
    }
    std::cout << std::string(60, '-') << std::endl;
    
    return 0;
}