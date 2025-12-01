#ifndef GUI_HPP
#define GUI_HPP

#include "imgui.h"
#include <glm/glm.hpp> // Nécessaire pour glm::vec3

// Forward declarations
struct GLFWwindow;
class Terrain; 

class Gui {
public:
    Gui();
    ~Gui();

    void Init(GLFWwindow* window);
    void Render(Terrain* terrain = nullptr);
    void Shutdown();

    // =========================================================
    // VARIABLES DE CONTRÔLE
    // =========================================================

    // --- Ecran d'accueil ---
    bool showWelcomeScreen = true; // Commence à VRAI

    // --- 1. Simulation & Temps ---
    bool isPaused = false;      
    float timeSpeed = 1.0f;     

    // --- 2. Paramètres Visuels ---
    float verticalScale = 1.0f; 
    float terrainColor[3] = {0.3f, 0.5f, 0.3f};   

    // --- 3. Paramètres Erosion Thermique ---
    int thermalIterations = 10000;
    float talusAngle = 30.0f;   
    float thermalK = 0.5f;      

    // --- 4. Paramètres Erosion Hydraulique ---
    int hydroIterations = 50000;
    float rainAmount = 1.0f;
    float evaporationRate = 0.5f;

    // --- 5. Debug & Stats ---
    glm::vec3 cameraPos = glm::vec3(0.0f); 
};

#endif