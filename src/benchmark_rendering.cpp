#include "TerrainApp.hpp"
#include <chrono>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <thread>
#include <GLFW/glfw3.h>

struct RenderingResult {
    int terrainSize;
    int lodLevel;
    double fps;
    double frameTimeMs;
    double minFrameTimeMs;
    double maxFrameTimeMs;
    double stddevMs;
};

std::vector<RenderingResult> results;

void printUsage() {
    std::cout << "Usage: ./benchmark_rendering [size] [duration_seconds]" << std::endl;
    std::cout << "  size: 512, 1024, 2048, 4096 (défaut: 2048)" << std::endl;
    std::cout << "  duration_seconds: durée du test en secondes (défaut: 10)" << std::endl;
}

int main(int argc, char* argv[])
{
    int terrainSize = 2048;
    int durationSec = 10;
    
    if (argc > 1) terrainSize = std::atoi(argv[1]);
    if (argc > 2) durationSec = std::atoi(argv[2]);
    
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "🔬 BENCHMARK RENDU PUR - SANS SIMULATION" << std::endl;
    std::cout << std::string(80, '=') << std::endl;
    std::cout << "📊 Terrain: " << terrainSize << "x" << terrainSize << std::endl;
    std::cout << "📊 Durée: " << durationSec << " secondes" << std::endl;
    std::cout << "📊 LOD: désactivé (niveau 0 uniquement)" << std::endl;
    std::cout << "📊 Frustum: désactivé (rendu complet)" << std::endl;
    std::cout << std::string(80, '=') << "\n" << std::endl;
    
    // Initialiser l'application
    TerrainApp app;
    if (!app.Init()) {
        std::cerr << "Erreur: impossible d'initialiser le rendu" << std::endl;
        return 1;
    }
    
    // Forcer le terrain à la taille demandée (via génération Perlin)
    // À adapter selon ton architecture
    
    std::cout << "📊 Mesure des performances de rendu..." << std::endl;
    std::cout << std::string(60, '-') << std::endl;
    
    std::vector<double> frameTimes;
    auto startTime = std::chrono::steady_clock::now();
    auto lastPrintTime = startTime;
    int frameCount = 0;
    
    // Variables pour les stats temps réel
    double currentFps = 0;
    double minFrameTime = 1e9;
    double maxFrameTime = 0;
    
    // Boucle de rendu pendant durationSec secondes
    while (true) {
        auto frameStart = std::chrono::high_resolution_clock::now();
        
        // Une frame de rendu (sans simulation)
        // À adapter selon ta boucle de rendu
        // Ici on suppose que app.Run() fait une frame
        // Sinon, il faut extraire la boucle de rendu
        
        // Calcul du temps de frame
        auto frameEnd = std::chrono::high_resolution_clock::now();
        double frameTimeMs = std::chrono::duration<double, std::milli>(frameEnd - frameStart).count();
        
        frameTimes.push_back(frameTimeMs);
        frameCount++;
        
        if (frameTimeMs < minFrameTime) minFrameTime = frameTimeMs;
        if (frameTimeMs > maxFrameTime) maxFrameTime = frameTimeMs;
        
        // Afficher les stats en direct toutes les secondes
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration<double>(now - lastPrintTime).count() >= 1.0) {
            double elapsed = std::chrono::duration<double>(now - startTime).count();
            currentFps = frameCount / elapsed;
            std::cout << "\r  FPS: " << std::fixed << std::setprecision(1) << currentFps
                      << " | Frame time: " << std::setprecision(2) << (1000.0/currentFps) << " ms"
                      << " | Frames: " << frameCount << std::flush;
            lastPrintTime = now;
        }
        
        // Vérifier la durée
        auto now2 = std::chrono::steady_clock::now();
        if (std::chrono::duration<double>(now2 - startTime).count() >= durationSec) {
            break;
        }
        
        // Petit sleep pour ne pas surcharger (optionnel)
        // std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    // Calcul des statistiques
    double totalTime = 0;
    for (double t : frameTimes) totalTime += t;
    double meanFrameTime = totalTime / frameTimes.size();
    double fps = 1000.0 / meanFrameTime;
    
    double variance = 0;
    for (double t : frameTimes) {
        variance += (t - meanFrameTime) * (t - meanFrameTime);
    }
    variance /= (frameTimes.size() - 1);
    double stddev = std::sqrt(variance);
    
    // Affichage des résultats finaux
    std::cout << "\n\n" << std::string(60, '=') << std::endl;
    std::cout << "📊 RÉSULTATS DU BENCHMARK RENDU" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    std::cout << "Terrain: " << terrainSize << "x" << terrainSize << std::endl;
    std::cout << "Frames analysées: " << frameTimes.size() << std::endl;
    std::cout << "Durée totale: " << durationSec << " s" << std::endl;
    std::cout << std::string(60, '-') << std::endl;
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "FPS moyen: " << fps << std::endl;
    std::cout << "Temps moyen par frame: " << meanFrameTime << " ms" << std::endl;
    std::cout << "Temps min par frame: " << minFrameTime << " ms" << std::endl;
    std::cout << "Temps max par frame: " << maxFrameTime << " ms" << std::endl;
    std::cout << "Écart-type: " << stddev << " ms" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    // Sauvegarde CSV
    std::ofstream csv("benchmark_rendering.csv");
    csv << "terrain_size,duration_sec,frames,avg_fps,avg_frame_time_ms,min_frame_time_ms,max_frame_time_ms,stddev_ms\n";
    csv << terrainSize << ","
        << durationSec << ","
        << frameTimes.size() << ","
        << fps << ","
        << meanFrameTime << ","
        << minFrameTime << ","
        << maxFrameTime << ","
        << stddev << "\n";
    csv.close();
    
    std::cout << "\n✅ Résultats sauvegardés dans benchmark_rendering.csv" << std::endl;
    
    return 0;
}