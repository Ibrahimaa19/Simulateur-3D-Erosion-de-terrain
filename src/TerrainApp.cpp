#include "TerrainApp.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath> 

#include "FaultFormationTerrain.hpp"
#include "MidpointDisplacement.hpp"
#include "PerlinNoiseTerrain.hpp" 

TerrainApp::TerrainApp(unsigned int seed)
    : mWindow(nullptr), mScreenWidth(1224), mScreenHeight(868),
      mLastX(mScreenWidth/2.0f), mLastY(mScreenHeight/2.0f),
      mFirstMouse(true), mMouseSensitivity(0.1f),
      mCameraSpeed(5.0f),
      mShowMenu(true)
{
    std::srand(seed);
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

    mGui.Init(mWindow); 

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

    if (mShowMenu) {
        glfwSetInputMode(mWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    } else {
        glfwSetInputMode(mWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
}

void TerrainApp::InitCamera()
{
    mCamera.MoveTo(glm::vec3{-54.0f, 220.0f, -42.0f});
    mCamera.TurnTo(glm::vec3{0.0f, 0.0f, 0.0f});
}

void TerrainApp::InitScene()
{
    mShader = new Shader("../shaders/terrain.vs", "../shaders/terrain.fs");
    mShader->Use();

    mTerrain = std::make_unique<PerlinNoiseTerrain>();
    
    if (auto perlin = dynamic_cast<PerlinNoiseTerrain*>(mTerrain.get())) {
       perlin->CreatePerlinNoise(512, 512, 0, 100); 
    }

    mTerrain->setup_terrain(mVAO, mVBO, mIBO);

    mModel = glm::mat4(1.0f);
    mView = mCamera.GetViewMatrix();
    mProjection = glm::perspective(glm::radians(45.0f), (float)mScreenWidth / (float)mScreenHeight, 0.1f, 5000.0f);
}

void TerrainApp::GenerateTerrainFromGui()
{
    std::string nomMethode = "Inconnue";
    if (mGui.selectedMethod == GEN_HEIGHTMAP) nomMethode = "Image (Heightmap)";
    else if (mGui.selectedMethod == GEN_FAULT_FORMATION) nomMethode = "Faille (Fault Formation)";
    else if (mGui.selectedMethod == GEN_MIDPOINT_DISPLACEMENT) nomMethode = "Deplacement (Midpoint)";
    else if (mGui.selectedMethod == GEN_PERLIN_NOISE) nomMethode = "Perlin Noise";

    std::cout << "Generation via GUI... Methode: " << nomMethode << std::endl;

    delete mShader;

    if (mGui.selectedMethod == GEN_HEIGHTMAP) 
    {
        mShader = new Shader("../shaders/shader.vs", "../shaders/shader.fs");
        
        mTerrain = std::make_unique<Terrain>(); 

        const char* path = "../src/heightmap/iceland_heightmap.png";
        if (mGui.selectedImage == 1) path = "../src/heightmap/heightmap.png";

        mTerrain->load_terrain(path, 400.0f, 1.0f);

        mCamera.MoveTo(glm::vec3{0.0f, 5.0f, 0.0f});
        mCamera.TurnTo(glm::vec3{mTerrain->get_terrain_width()/2.0f, 0.0f, mTerrain->get_terrain_height()/2.0f});
    }
    
    else 
    {
        mShader = new Shader("../shaders/terrain.vs", "../shaders/terrain.fs");

        if (mGui.selectedMethod == GEN_FAULT_FORMATION) 
        {
            auto generator = std::make_unique<FaultFormationTerrain>();
            generator->CreateFaultFormation(
                mGui.faultWidth, mGui.faultHeight, 
                mGui.faultIterations, 
                mGui.faultMinHeight, mGui.faultMaxHeight, 
                1.0f, mGui.faultUseFilter, mGui.faultFilter
            );
            mTerrain = std::move(generator);
        }
        else if (mGui.selectedMethod == GEN_MIDPOINT_DISPLACEMENT) 
        {
            auto generator = std::make_unique<MidpointDisplacement>();
            generator->CreateMidpointDisplacement(
                mGui.midpointSize, 
                mGui.midpointMinHeight, mGui.midpointMaxHeight, 
                1.0f, mGui.midpointRoughness
            );
            mTerrain = std::move(generator);
        }
        else if (mGui.selectedMethod == GEN_PERLIN_NOISE)
        {
            auto generator = std::make_unique<PerlinNoiseTerrain>();
            generator->CreatePerlinNoise(
                mGui.perlinWidth, mGui.perlinHeight,
                mGui.perlinMinHeight, mGui.perlinMaxHeight,
                1.0f, 
                mGui.perlinFrequency,
                mGui.perlinOctaves,
                mGui.perlinPersistence,
                mGui.perlinLacunarity
            );
            mTerrain = std::move(generator);
        }
        // -----------------------------------------------

        mCamera.MoveTo(glm::vec3{-54.0f, 220.0f, -42.0f});
        mCamera.TurnTo(glm::vec3{mTerrain->get_terrain_width()/2.0f, 0.0f, mTerrain->get_terrain_height()/2.0f});
    }

    mShader->Use();
    mTerrain->setup_terrain(mVAO, mVBO, mIBO);
}

void TerrainApp::Run()
{
    while (!glfwWindowShouldClose(mWindow))
    {
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (mGui.startGeneration) {
            GenerateTerrainFromGui();
            mGui.startGeneration = false;
            mShowMenu = true;
            glfwSetInputMode(mWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }

        if (mGui.resetSimulation) {
            mGui.resetSimulation = false;
            glfwSetInputMode(mWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }

        if (mGui.showWelcomeScreen) {
            glfwSetInputMode(mWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            mGui.Render(nullptr);
        }
        else if (mGui.showConfigScreen) {
            glfwSetInputMode(mWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            mGui.Render(nullptr);
        }
        else {
            RenderScene();
            mGui.cameraPos = glm::vec3(glm::inverse(mView)[3]);
            if (mShowMenu) {
                mGui.Render(mTerrain.get()); 
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
    mShader->SetFloat("gMaxHeight", mTerrain->get_max_height());
    mShader->SetFloat("gMixHeight", mTerrain->get_min_height());
    
    glBindVertexArray(mVAO);
    mTerrain->renderer(); 
}

void TerrainApp::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    TerrainApp* app = (TerrainApp*)glfwGetWindowUserPointer(window);
    if (!app) return;

    if (!app->mGui.showWelcomeScreen && !app->mGui.showConfigScreen) {
        if (action == GLFW_PRESS) {
            if (key == GLFW_KEY_M || key == GLFW_KEY_SEMICOLON) {
                app->mShowMenu = !app->mShowMenu;
                if (app->mShowMenu) {
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                } else {
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                    app->mFirstMouse = true;
                }
                return; 
            }
        }
    }

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
            if(state) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            else glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            break;
        }
    }
}

void TerrainApp::MouseCallback(GLFWwindow* window, double xpos, double ypos)
{
    TerrainApp* app = (TerrainApp*)glfwGetWindowUserPointer(window);
    if (!app) return;

    if (app->mShowMenu || app->mGui.showWelcomeScreen || app->mGui.showConfigScreen) return;

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

    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) return;

    app->mCamera.Move(app->mCamera.GetForward(), (float)yoffset);
}

void TerrainApp::FramebufferCallback(GLFWwindow* window, int width, int height)
{
    TerrainApp* app = (TerrainApp*)glfwGetWindowUserPointer(window);
    if (!app) return;

    app->mScreenWidth = width;
    app->mScreenHeight = height;

    glViewport(0, 0, width, height);
    app->mProjection = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 5000.0f);

    app->mLastX = width / 2.0f;
    app->mLastY = height / 2.0f;
};