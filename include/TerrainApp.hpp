#pragma once


#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <iostream>

#include "Camera.hpp"
#include "Shader.hpp"
#include "Terrain.hpp"

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
     * @brief Default constructor.
     */
    TerrainApp();

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
    Terrain mTerrain;                 ///< Terrain object

    GLuint mVAO[4];                      ///< Vertex Array Object
    GLuint mVBO[4];                      ///< Vertex Buffer Object
    GLuint mIBO[4];                      ///< Index Buffer Object

    float mCameraSpeed;               ///< Speed used for keyboard movement

private:
    /** Keyboard callback wrapper */
    static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

    /** Mouse movement callback wrapper */
    static void MouseCallback(GLFWwindow* window, double xpos, double ypos);

    /** Scroll callback wrapper */
    static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

    /** Framebuffer resize callback wrapper */
    static void FramebufferCallback(GLFWwindow* window, int width, int height);

    /** Return the LOD levels to use for renderer*/
    int SelectLOD(const glm::vec3&& cameraPos);
};