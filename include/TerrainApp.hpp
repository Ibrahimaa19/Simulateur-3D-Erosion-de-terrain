#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <iostream>
#include <memory>
#include <string.h>
#include <cmath>
#include <future>
#include <atomic>
#include <mutex>

#include "Camera.hpp"
#include "Shader.hpp"
#include "Terrain.hpp"
#include "FaultFormationTerrain.hpp"
#include "MidpointDisplacement.hpp"
#include "PerlinNoiseTerrain.hpp"
#include "ThermalErosion.hpp"
#include "Gui.hpp"


/**
 * @class TerrainApp
 * @brief Manages window creation, input callbacks, camera, rendering loop, and terrain rendering.
 *
 * This class encapsulates the full OpenGL application logic to keep main.cpp clean.
 */
class TerrainApp
{
public:
    /**
     * @brief Constructs a TerrainApp instance
     * @param seed Seed used to initialize the random number generator
     */
    TerrainApp(unsigned int seed = 1);

    /**
     * @brief Destructor.
     */
    ~TerrainApp();

    /**
     * @brief Initializes GLFW, window, callbacks, camera, shaders and terrain.
     */
    bool Init();

    /**
     * @brief Starts the rendering loop.
     */
    void Run();

    /**
     * @brief Change the camera speed
     */
    void setCameraSpeed(float value);


private:
    /**
     * @brief Internal function to initialize the window.
     */
    bool InitWindow();

    /**
     * @brief Initializes all GLFW input callbacks.
     */
    void InitCallbacks();

    /**
     * @brief Initializes the camera system.
     */
    void InitCamera();

    /**
     * @brief Loads shaders and prepares terrain data.
     */
    void InitScene();

    /**
     * @brief Main rendering function called each frame.
     */
    void RenderScene();

    /**
     * @brief Handles terrain regeneration based on GUI settings.
     */
    void GenerateTerrainFromGui();

    /**
     * @brief Builds a terrain object from current GUI selection.
     */
    std::unique_ptr<Terrain> BuildTerrainFromGuiSelection();

    /**
     * @brief Finalizes terrain after CPU build (camera, textures, LOD, erosion info).
     */
    void FinalizeTerrainAfterBuild();

    /**
     * @brief Starts asynchronous terrain generation.
     */
    void StartTerrainGenerationAsync();

    /**
     * @brief Checks async generation completion and finalizes on main thread.
     */
    void UpdateTerrainGeneration();
    struct ThermalFrameResult
    {
        int cellsModified = 0;
        bool iterationFinished = false;
    };

    ThermalFrameResult AdvanceThermalErosionFrame();
private:
    GLFWwindow* mWindow;              ///< Pointer to the GLFW window

    int mScreenWidth;                 ///< Current window width
    int mScreenHeight;                ///< Current window height

    float mLastX;                     ///< Last mouse X position
    float mLastY;                     ///< Last mouse Y position
    bool mFirstMouse;                 ///< Flag for first mouse movement
    float mMouseSensitivity;          ///< Mouse sensitivity multiplier

    Camera mCamera;                   ///< 3D camera
    glm::mat4 mModel;                 ///< Model matrix
    glm::mat4 mView;                  ///< View matrix
    glm::mat4 mProjection;            ///< Projection matrix

    std::unique_ptr<Shader> mShader;   ///< Smart pointer to the shader program
    std::unique_ptr<Terrain> mTerrain; ///< Smart pointer to the Terrain object (Polymorphic)
    ThermalErosion mThermalErosion;    ///< Objet gérant l’érosion thermique appliquée au terrain courant

    GLuint mVAO = 0;                   ///< Vertex Array Object
    GLuint mVBO = 0;                   ///< Vertex Buffer Object
    GLuint mIBO = 0;                   ///< Index Buffer Object

    float mCameraSpeed;                ///< Speed used for keyboard movement

    bool thermalEnabled;
    bool thermalStarted;

    bool hydraulicEnabled;
    bool hydraulicStarted;

    Gui mGui;                          ///< User Interface instance
    bool mShowMenu;                    ///< Boolean to toggle menu visibility

    std::future<void> mGenerationFuture;   ///< Tâche asynchrone de génération du terrain
    std::atomic<bool> mIsGenerating{false}; ///< Indique si une génération est en cours
    std::atomic<bool> mPendingFinalize{false}; ///< Indique qu'une finalisation sur le thread principal est attendue

    std::mutex mGenerationMutex;           ///< Mutex protégeant l'échange du terrain généré
    std::unique_ptr<Terrain> mPendingTerrain; ///< Terrain généré en arrière-plan en attente d'intégration

private:
    /** Keyboard callback wrapper */
    static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

    /** Mouse movement callback wrapper */
    static void MouseCallback(GLFWwindow* window, double xpos, double ypos);

    /** Scroll callback wrapper */
    static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

    /** Framebuffer resize callback wrapper */
    static void FramebufferCallback(GLFWwindow* window, int width, int height);
};