#pragma once


#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <iostream>
#include <memory> 

#include "Camera.hpp"
#include "Shader.hpp"
#include "Terrain.hpp"
#include "FaultFormationTerrain.hpp"
#include "MidpointDisplacement.hpp"
#include "PerlinNoiseTerrain.hpp" 

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

    Shader* mShader;                  ///< Pointer to the shader program
    
    std::unique_ptr<Terrain> mTerrain; ///< Smart pointer to the Terrain object (Polymorphic)

    GLuint mVAO;                      ///< Vertex Array Object
    GLuint mVBO;                      ///< Vertex Buffer Object
    GLuint mIBO;                      ///< Index Buffer Object

    float mCameraSpeed;               ///< Speed used for keyboard movement

    Gui mGui;                         ///< User Interface instance
    bool mShowMenu;                   ///< Boolean to toggle menu visibility

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