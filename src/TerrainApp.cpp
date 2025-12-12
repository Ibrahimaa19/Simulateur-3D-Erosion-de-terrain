#include "TerrainApp.hpp"
#include "ThermalErosion.hpp"
#include "HydraulicErosion.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>

TerrainApp::TerrainApp()
    : mWindow(nullptr), mScreenWidth(1224), mScreenHeight(868),
      mLastX(mScreenWidth/2.0f), mLastY(mScreenHeight/2.0f),
      mFirstMouse(true), mMouseSensitivity(0.1f),
      mCameraSpeed(0.1f),
      thermalEnabled(false), thermalStarted(false),
      hydraulicEnabled(false), hydraulicStarted(false)
{
}

TerrainApp::~TerrainApp()
{
    delete mShader;
    glfwTerminate();
}

bool TerrainApp::Init()
{
    if (!InitWindow()) return false;
    InitCallbacks();
    InitScene();
    InitCamera();
    return true;
}

bool TerrainApp::InitWindow()
{
    if (!glfwInit()) return false;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    mWindow = glfwCreateWindow(mScreenWidth, mScreenHeight, "Height Map", nullptr, nullptr);
    if (!mWindow) return false;
    glfwMakeContextCurrent(mWindow);

    glewInit();
    glEnable(GL_DEPTH_TEST);

    return true;
}

void TerrainApp::InitCallbacks()
{
    glfwSetWindowUserPointer(mWindow, this);

    glfwSetFramebufferSizeCallback(mWindow, FramebufferCallback);
    glfwSetCursorPosCallback(mWindow, MouseCallback);
    glfwSetKeyCallback(mWindow, KeyCallback);
    glfwSetScrollCallback(mWindow, ScrollCallback);

    glfwSetInputMode(mWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void TerrainApp::InitCamera()
{
    mCamera.MoveTo(glm::vec3{0.0f, 5.0f, 0.0f});
    mCamera.TurnTo(glm::vec3{mTerrain.get_terrain_width() / 2, 0.0f, mTerrain.get_terrain_height()/ 2});
}

void TerrainApp::InitScene()
{
    mShader = new Shader("../shaders/shader.vs", "../shaders/shader.fs");
    mShader->Use();

    mTerrain.load_terrain("../src/heightmap/iceland_heightmap.png", 1.f, 100.f);
    mTerrain.setup_terrain(mVAO, mVBO, mIBO);

    mModel = glm::mat4(1.0f);
    mView = mCamera.GetViewMatrix();
    mProjection = glm::perspective(glm::radians(45.0f), (float)mScreenWidth / (float)mScreenHeight, 0.1f, 100.0f);
}

void TerrainApp::Run()
{
    // --- Initialisation de l'érosion thermique
    ThermalErosion thermalErosion;
    thermalErosion.loadTerrainInfo(&mTerrain);

    // Paramètres initiaux (modifiables plus tard via l’UI)
    thermalErosion.setTalusAngle(0.6f);
    thermalErosion.setTransferRate(0.05f);

    int stepCounter = 0;

    while (!glfwWindowShouldClose(mWindow))
    {
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        RenderScene();

        // --- Erosion thermique
        if (thermalEnabled)
        {
            thermalErosion.step();
            stepCounter++;

            // Mise à jour CPU des vertices
            mTerrain.update_vertices();

            // Mise à jour GPU
            glBindBuffer(GL_ARRAY_BUFFER, mVBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0,
                mTerrain.get_vertices().size() * sizeof(float),
                mTerrain.get_vertices().data());

            if (stepCounter % 10 == 0) {
                std::cout << "Thermal erosion step " << stepCounter << std::endl;
            }
        }

        glfwSwapBuffers(mWindow);
        glfwPollEvents();
    }
}


void TerrainApp::RenderScene()
{
    mView = mCamera.GetViewMatrix();
    glm::mat4 finalMatrix = mProjection * mView * mModel;
    
    mShader->SetMat4("gFinalMatrix", finalMatrix);
    glBindVertexArray(mVAO);
    mTerrain.renderer();
}

void TerrainApp::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    TerrainApp* app = (TerrainApp*)glfwGetWindowUserPointer(window);
    if (!app) return;

    if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {
        switch (key)
        {
        case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose(window, GLFW_TRUE); break;
        case GLFW_KEY_W: app->mCamera.Move(app->mCamera.GetForward(), app->mCameraSpeed); break;
        case GLFW_KEY_S: app->mCamera.Move(-app->mCamera.GetForward(), app->mCameraSpeed); break;
        case GLFW_KEY_D: app->mCamera.Move(app->mCamera.GetRight(), app->mCameraSpeed); break;
        case GLFW_KEY_A: app->mCamera.Move(-app->mCamera.GetRight(), app->mCameraSpeed); break;
        case GLFW_KEY_Q: app->mCamera.Move(app->mCamera.GetUp(), app->mCameraSpeed); break;
        case GLFW_KEY_E: app->mCamera.Move(-app->mCamera.GetUp(), app->mCameraSpeed); break;
        case GLFW_KEY_P: 
            static bool state{false};
            state = !state;
            if(state)
            {
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            }
            else
            {
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            }
            break;
        case GLFW_KEY_F:
            app->thermalEnabled = !app->thermalEnabled;
            if (app->thermalEnabled && !app->thermalStarted) {
                std::cout << "Thermal erosion STARTED" << std::endl;
                app->thermalStarted = true;
            } else if (!app->thermalEnabled) {
                std::cout << "Thermal erosion PAUSED" << std::endl;
            }
            break;
        case GLFW_KEY_G:
            app->hydraulicEnabled = !app->hydraulicEnabled;
            if (app->hydraulicEnabled && !app->hydraulicStarted) {
                std::cout << "Hydraulic erosion STARTED" << std::endl;
                app->hydraulicStarted = true;
            } else if (!app->hydraulicEnabled) {
                std::cout << "Hydraulic erosion PAUSED" << std::endl;
            }
            break;
        }
    }
}

void TerrainApp::MouseCallback(GLFWwindow* window, double xpos, double ypos)
{
    TerrainApp* app = (TerrainApp*)glfwGetWindowUserPointer(window);
    if (!app) return;

    if (app->mFirstMouse) {
        app->mLastX = (float)xpos;
        app->mLastY = (float)ypos;
        app->mFirstMouse = false;
    }

    float xoffset = float(xpos - app->mLastX);
    float yoffset = float(ypos - app->mLastY);

    app->mLastX = (float)xpos;
    app->mLastY = (float)ypos;

    xoffset *= app->mMouseSensitivity;
    yoffset *= app->mMouseSensitivity;

    app->mCamera.Yaw(xoffset);
    app->mCamera.Pitch(yoffset);
}

void TerrainApp::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    TerrainApp* app = (TerrainApp*)glfwGetWindowUserPointer(window);
    if (!app) return;

    app->mCamera.Move(app->mCamera.GetForward(), (float)yoffset);
}

void TerrainApp::FramebufferCallback(GLFWwindow* window, int width, int height)
{
    TerrainApp* app = (TerrainApp*)glfwGetWindowUserPointer(window);
    if (!app) return;

    app->mScreenWidth = width;
    app->mScreenHeight = height;

    glViewport(0, 0, width, height);
    app->mProjection = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 100.0f);

    app->mLastX = width / 2.0f;
    app->mLastY = height / 2.0f;
}
