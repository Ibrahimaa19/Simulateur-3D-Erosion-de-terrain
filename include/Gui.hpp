#ifndef GUI_HPP
#define GUI_HPP

#include "imgui.h"
#include <glm/glm.hpp> 

struct GLFWwindow;
class Terrain; 

class Gui {
public:
    Gui();
    ~Gui();

    void Init(GLFWwindow* window);
    void Render(Terrain* terrain = nullptr);
    void Shutdown();

    // VARIABLES DE CONTRÔLE

    bool showWelcomeScreen = true; 

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