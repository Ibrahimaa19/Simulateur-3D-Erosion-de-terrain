#ifndef GUI_HPP
#define GUI_HPP

#include "imgui.h"
#include <glm/glm.hpp> 

struct GLFWwindow;
class Terrain; 

enum GenMethod {
    GEN_HEIGHTMAP = 0,
    GEN_FAULT_FORMATION = 1,
    GEN_MIDPOINT_DISPLACEMENT = 2,
    GEN_PERLIN_NOISE = 3 
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
    int selectedImage = 0; 

    int faultWidth = 1024;
    int faultHeight = 1024;
    int faultIterations = 500;
    float faultMinHeight = 0.0f;
    float faultMaxHeight = 255.0f;
    bool faultUseFilter = true;
    float faultFilter = 0.5f;

    int midpointSize = 1025; 
    float midpointRoughness = 1.0f;
    float midpointMinHeight = 0.0f;
    float midpointMaxHeight = 255.0f;

    int perlinWidth = 1024;
    int perlinHeight = 1024;
    float perlinMinHeight = 0.0f;
    float perlinMaxHeight = 255.0f;
    float perlinFrequency = 0.005f;
    int perlinOctaves = 4;      
    float perlinPersistence = 0.5f;
    float perlinLacunarity = 2.0f;


    // =========================================================
    // PARAMÈTRES DE SIMULATION 
    // =========================================================
    bool isPaused = false;      
    float timeSpeed = 1.0f;     

    float verticalScale = 1.0f; 
    float terrainColor[3] = {0.3f, 0.5f, 0.3f};   

    bool thermalRunning = false;   
    int thermalCurrentStep = 0;
    
    int thermalCellsModified = 0;  
    
    
    float talusAngle = 30.0f;   
    float thermalK = 0.5f;      

    int hydroIterations = 50000;
    float rainAmount = 1.0f;
    float evaporationRate = 0.5f;

    glm::vec3 cameraPos = glm::vec3(0.0f); 
};

#endif