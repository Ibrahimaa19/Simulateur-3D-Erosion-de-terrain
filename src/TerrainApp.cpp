#include "TerrainApp.hpp"
#include "RendererManager.hpp"

void TerrainApp::setCameraSpeed(float value){
    mCameraSpeed = value;
}

TerrainApp::TerrainApp(unsigned int seed)
    : mWindow(nullptr), mScreenWidth(1224), mScreenHeight(868),
      mLastX(mScreenWidth/2.0f), mLastY(mScreenHeight/2.0f),
      mFirstMouse(true), mMouseSensitivity(0.1f),
      mCameraSpeed(5.0f),
      thermalEnabled(false), thermalStarted(false),
      hydraulicEnabled(false), hydraulicStarted(false),
      mShowMenu(true)
{
    std::srand(seed);
}

TerrainApp::~TerrainApp()
{
    if (mVAO) glDeleteVertexArrays(1, &mVAO);
    if (mVBO) glDeleteBuffers(1, &mVBO);
    if (mIBO) glDeleteBuffers(1, &mIBO);

    mShader.reset();

    if (mWindow)
        glfwDestroyWindow(mWindow);

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
    mShader = std::make_unique<Shader>("../shaders/terrain.vs", "../shaders/terrain.fs");

    mShader->Use();

    mModel = glm::mat4(1.0f);
    mView = mCamera.GetViewMatrix();
    mProjection = glm::perspective(glm::radians(45.0f), (float)mScreenWidth / (float)mScreenHeight, 0.01f, 5000.0f);

}

void TerrainApp::GenerateTerrainFromGui()
{
    mTerrain = BuildTerrainFromGuiSelection();
    FinalizeTerrainAfterBuild();
}

void TerrainApp::Run()
{
    int stepCounter = 0;

    while (!glfwWindowShouldClose(mWindow))
    {
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (mGui.startGeneration) {
            StartTerrainGenerationAsync();
            mGui.startGeneration = false;
            mShowMenu = true;

            mGui.showWelcomeScreen = false;
            mGui.showConfigScreen = false;

            stepCounter = 0;
            mGui.thermalCurrentStep = 0;
            mGui.thermalCellsModified = 0;
            mGui.thermalRunning = false;
            thermalEnabled = false;

            glfwSetInputMode(mWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }

        if (mGui.resetSimulation) {
            mGui.resetSimulation = false;
            thermalEnabled = false;
            mGui.thermalRunning = false;
            stepCounter = 0;
            mGui.thermalCurrentStep = 0;
            mGui.thermalCellsModified = 0;
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
            UpdateTerrainGeneration();
            RenderScene();

            mThermalErosion.setTalusAngle(mGui.talusAngle);
            mThermalErosion.setTransferRate(mGui.thermalK);

            thermalEnabled = mGui.thermalRunning;

            if (thermalEnabled && mTerrain)
            {
                ThermalFrameResult frameResult = AdvanceThermalErosionFrame();

                mGui.thermalCellsModified = frameResult.cellsModified;

                if (frameResult.iterationFinished) {
                    stepCounter++;
                    mGui.thermalCurrentStep = stepCounter;
                }
            }

            mGui.cameraPos = glm::vec3(glm::inverse(mView)[3]);
            if (mShowMenu) {
                mGui.Render(mTerrain ? mTerrain.get() : nullptr);
            }
        }

        glfwSwapBuffers(mWindow);
        glfwPollEvents();
    }
}
void TerrainApp::RenderScene()
{
    if (!mShader || !mTerrain || !mTerrain->getRendererManager() || !mTerrain->getTexture())
        return;

    mView = mCamera.GetViewMatrix();
    glm::mat4 finalMatrix = mProjection * mView * mModel;

    mShader->SetMat4("gFinalMatrix", finalMatrix);
    mShader->SetFloat("gMaxHeight", mTerrain->getMaxHeight());
    mShader->SetFloat("gMinHeight", mTerrain->getMinHeight());

    for (int i = 0; i < 4; i++) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, mTerrain->getTexture()->getTextureId(i));
    }

    mShader->SetInt("terrainTexture0", 0);
    mShader->SetInt("terrainTexture1", 1);
    mShader->SetInt("terrainTexture2", 2);
    mShader->SetInt("terrainTexture3", 3);

    glBindVertexArray(mVAO);
    mTerrain->getRendererManager()->renderLod(mCamera.GetPosition(), mProjection, mView);
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
            {
                static bool state{false};
                state = !state;
                if(state) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                else glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                break;
            }

            case GLFW_KEY_F:{
                app->thermalEnabled = !app->thermalEnabled;
                app->mGui.thermalRunning = app->thermalEnabled;
                
                if (app->thermalEnabled) {
                    std::cout << "Thermal erosion STARTED" << std::endl;
                } else {
                    std::cout << "Thermal erosion PAUSED" << std::endl;
                }
                break;
            }
            
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

std::unique_ptr<Terrain> TerrainApp::BuildTerrainFromGuiSelection() {
    std::string nomMethode = "Inconnue";

    if (mGui.selectedMethod == GEN_HEIGHTMAP)
        nomMethode = "Image (Heightmap)";
    else if (mGui.selectedMethod == GEN_FAULT_FORMATION)
        nomMethode = "Faille (Fault Formation)";
    else if (mGui.selectedMethod == GEN_MIDPOINT_DISPLACEMENT)
        nomMethode = "Deplacement (Midpoint)";
    else if (mGui.selectedMethod == GEN_PERLIN_NOISE)
        nomMethode = "Perlin Noise";

    std::cout << "Generation via GUI... Methode: " << nomMethode << std::endl;

    if (mGui.selectedMethod == GEN_HEIGHTMAP) 
    {
        auto terrain = std::make_unique<Terrain>();

        const char* path = "../src/heightmap/helbert_heightmap.png";

        if (mGui.selectedImage == 0)
            path = "../src/heightmap/canyon_heightmap.png";
        else if (mGui.selectedImage == 1)
            path = "../src/heightmap/fuji_heightmap.png";
        else if (mGui.selectedImage == 2)
            path = "../src/heightmap/paris_heightmap.png";
        else if (mGui.selectedImage == 3)
            path = "../src/heightmap/helbert_heightmap.png";
        else if (mGui.selectedImage == 4)
            path = "../src/heightmap/sopka_heightmap.png";
        else if (mGui.selectedImage == 5)
            path = "../src/heightmap/grandCayon_heightmap.png";

        terrain->loadTerrain(path, 1.0f, 1.0f);
        terrain->getRendererManager()->setTerrain(terrain.get());

        return terrain;
    }
    else 
    {
        if (mGui.selectedMethod == GEN_FAULT_FORMATION) 
        {
            auto generator = std::make_unique<FaultFormationTerrain>();

            generator->CreateFaultFormation(
                mGui.faultWidth,
                mGui.faultHeight,
                mGui.faultIterations,
                mGui.faultMinHeight,
                mGui.faultMaxHeight,
                1.0f,
                mGui.faultUseFilter,
                mGui.faultFilter
            );

            auto renderer = std::make_unique<RendererManager>(generator.get());
            generator->setRenderer(std::move(renderer));
            return generator;
        }
        else if (mGui.selectedMethod == GEN_MIDPOINT_DISPLACEMENT) 
        {
            auto generator = std::make_unique<MidpointDisplacement>();

            generator->CreateMidpointDisplacement(
                mGui.midpointSize,
                mGui.midpointMinHeight,
                mGui.midpointMaxHeight,
                1.0f,
                mGui.midpointRoughness
            );

            auto renderer = std::make_unique<RendererManager>(generator.get());
            generator->setRenderer(std::move(renderer));
            return generator;
        }
        else if (mGui.selectedMethod == GEN_PERLIN_NOISE)
        {
            auto generator = std::make_unique<PerlinNoiseTerrain>();

            generator->CreatePerlinNoise(
                mGui.perlinWidth,
                mGui.perlinHeight,
                mGui.perlinMinHeight,
                mGui.perlinMaxHeight,
                1.0f,
                mGui.perlinFrequency,
                mGui.perlinOctaves,
                mGui.perlinPersistence,
                mGui.perlinLacunarity
            );

            auto renderer = std::make_unique<RendererManager>(generator.get());
            generator->setRenderer(std::move(renderer));
            return generator;
        }
    }

    return nullptr;
}

void TerrainApp::FinalizeTerrainAfterBuild()
{
    mShader = std::make_unique<Shader>("../shaders/terrain.vs", "../shaders/terrain.fs");
    mShader->Use();

    if (!mTerrain)
        return;

    setCameraSpeed(5.0f);
    mCamera.MoveTo(glm::vec3{-54.0f, 220.0f, -42.0f});
    mCamera.TurnTo(glm::vec3{
        mTerrain->getTerrainWidth() / 2.0f,
        0.0f,
        mTerrain->getTerrainHeight() / 2.0f
    });

    mTerrain->initTexture();
    mTerrain->setupTerrainLod(mVAO, mVBO, mIBO);
    mThermalErosion.loadTerrainInfo(mTerrain);
    mGui.thermalCurrentStep = 0;
    mGui.thermalCellsModified = 0;
    mGui.thermalRunning = false;
    thermalEnabled = false;
}

void TerrainApp::StartTerrainGenerationAsync() {
    if (mIsGenerating)
        return;

    mIsGenerating = true;
    mPendingFinalize = false;

    mGenerationFuture = std::async(std::launch::async, [this]() {
        auto generatedTerrain = BuildTerrainFromGuiSelection();

        {
            std::lock_guard<std::mutex> lock(mGenerationMutex);
            mPendingTerrain = std::move(generatedTerrain);
        }

        mIsGenerating = false;
        mPendingFinalize = true;
    });
}

void TerrainApp::UpdateTerrainGeneration() {
    if (!mPendingFinalize || mIsGenerating)
        return;

    if (mGenerationFuture.valid()) {
        mGenerationFuture.get();
    }

    {
        std::lock_guard<std::mutex> lock(mGenerationMutex);
        mTerrain = std::move(mPendingTerrain);
    }

    if (mTerrain) {
        FinalizeTerrainAfterBuild();
    }

    mPendingFinalize = false;
}

TerrainApp::ThermalFrameResult TerrainApp::AdvanceThermalErosionFrame()
{
    ThermalFrameResult result{};

    if (!mTerrain) {
        return result;
    }

    if (mGui.thermalUseFourNeighbors) {
        mThermalErosion.useFourNeighbors();
    } else {
        mThermalErosion.useEightNeighbors();
    }

    const bool chunked = (mGui.thermalExecutionMode == THERMAL_EXEC_CHUNKED);

    switch (mGui.thermalKernel)
    {
        case THERMAL_KERNEL_PURE_TWO_PHASE:
            result.cellsModified = chunked
                ? mThermalErosion.stepPureTwoPhaseChunk(mGui.thermalChunkBudgetCells)
                : mThermalErosion.stepPureTwoPhase();
            break;

        case THERMAL_KERNEL_BLOCKED_PURE_TWO_PHASE:
            result.cellsModified = chunked
                ? mThermalErosion.stepBlockedPureTwoPhaseChunk(mGui.thermalChunkBudgetBlocks)
                : mThermalErosion.stepBlockedPureTwoPhase();
            break;

        case THERMAL_KERNEL_BLOCKED_PARALLEL_PURE_TWO_PHASE:
            result.cellsModified = chunked
                ? mThermalErosion.stepBlockedParallelPureTwoPhaseChunk(mGui.thermalChunkBudgetBlocks)
                : mThermalErosion.stepBlockedParallelPureTwoPhase();
            break;

        case THERMAL_KERNEL_CHECKERBOARD_PURE_TWO_PHASE:
            result.cellsModified = chunked
                ? mThermalErosion.stepCheckerboardPureTwoPhaseChunk(mGui.thermalChunkBudgetCells)
                : mThermalErosion.stepCheckerboardPureTwoPhase();
            break;

        case THERMAL_KERNEL_BLOCKED_CHECKERBOARD_PURE_TWO_PHASE:
            result.cellsModified = chunked
                ? mThermalErosion.stepBlockedCheckerboardPureTwoPhaseChunk(mGui.thermalChunkBudgetBlocks)
                : mThermalErosion.stepBlockedCheckerboardPureTwoPhase();
            break;

        case THERMAL_KERNEL_CHECKERBOARD_IN_PLACE:
            result.cellsModified = chunked
                ? mThermalErosion.stepCheckerboardInPlaceChunk(mGui.thermalChunkBudgetCells)
                : mThermalErosion.stepCheckerboardInPlace();
            break;

        case THERMAL_KERNEL_CHECKERBOARD_IN_PLACE_PARALLEL:
            result.cellsModified = chunked
                ? mThermalErosion.stepCheckerboardInPlaceParallelChunk(mGui.thermalChunkBudgetBlocks)
                : mThermalErosion.stepCheckerboardInPlaceParallel();
            break;
    }

    if (mThermalErosion.needsVisualUpdate()) {
        mThermalErosion.commitWorkingData();
        mTerrain->updateVerticesGpuLod(mThermalErosion.getDirtyPatchIndices());
        mThermalErosion.clearDirtyPatchIndices();
    }

    result.iterationFinished = mThermalErosion.isIterationFinished();

    if (result.iterationFinished) {
        mTerrain->updateVerticesGpuLod(mThermalErosion.getDirtyPatchIndices());
        mThermalErosion.clearDirtyPatchIndices();
    }

    return result;
}