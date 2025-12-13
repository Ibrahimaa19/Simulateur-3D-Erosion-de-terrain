#ifndef GUI_HPP
#define GUI_HPP

#include "imgui.h"
#include <glm/glm.hpp> 

struct GLFWwindow;
class Terrain; 

enum GenMethod {
    GEN_HEIGHTMAP = 0,
    GEN_FAULT_FORMATION = 1,
    GEN_MIDPOINT_DISPLACEMENT = 2
};

class Gui {
public:
    Gui();
    ~Gui();

    void Init(GLFWwindow* window);
    void Render(Terrain* terrain = nullptr);
    void Shutdown();

    // =========================================================
    // VARIABLES D'ÉTAT 
    // =========================================================
    bool showWelcomeScreen = true; 
    bool showConfigScreen = false; 

    bool startGeneration = false;  
    bool resetSimulation = false;  

    // =========================================================
    // PARAMÈTRES DE GÉNÉRATION 
    // =========================================================
   
    int selectedMethod = GEN_HEIGHTMAP; 

    // --- Paramètres Méthode 1 : Heightmap (Image) ---
    int selectedImage = 0; 

    // --- Paramètres Méthode 2 : Fault Formation ---
    int faultWidth = 512;
    int faultHeight = 512;
    int faultIterations = 500;
    float faultMinHeight = 0.0f;
    float faultMaxHeight = 255.0f;
    bool faultUseFilter = true;
    float faultFilter = 0.5f;

    // --- Paramètres Méthode 3 : Midpoint Displacement ---
    int midpointSize = 513; 
    float midpointRoughness = 1.0f;
    float midpointMinHeight = 0.0f;
    float midpointMaxHeight = 255.0f;


    // =========================================================
    // PARAMÈTRES DE SIMULATION 
    // =========================================================
    bool isPaused = false;      
    float timeSpeed = 1.0f;     

    float verticalScale = 1.0f; 
    float terrainColor[3] = {0.3f, 0.5f, 0.3f};   

    int thermalIterations = 10000;
    float talusAngle = 30.0f;   
    float thermalK = 0.5f;      

    int hydroIterations = 50000;
    float rainAmount = 1.0f;
    float evaporationRate = 0.5f;

    glm::vec3 cameraPos = glm::vec3(0.0f); 
};

#endif