#define STB_IMAGE_IMPLEMENTATION

#include "PerlinNoiseTerrain.hpp"
#include "Shader.hpp"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#ifndef BENCHMARK_PROJECT_ROOT
#define BENCHMARK_PROJECT_ROOT "."
#endif

namespace
{
using Clock = std::chrono::steady_clock;

struct Options
{
    std::vector<int> sizes{512, 1024, 2048};
    int warmup = 50;
    int frames = 500;
    int windowWidth = 1224;
    int windowHeight = 868;
    bool forceBufferUpdate = false;
    bool help = false;
    std::string output = "benchmarks/results/render_profile.csv";
};

struct FrameTimes
{
    double renderMs = 0.0;
    double bufferUpdateMs = 0.0;
    double drawMs = 0.0;
};

double elapsedMs(const Clock::time_point& start, const Clock::time_point& end)
{
    return std::chrono::duration<double, std::milli>(end - start).count();
}

void printUsage(const char* exe)
{
    std::cout
        << "Usage: " << exe << " [options]\n"
        << "\n"
        << "Options:\n"
        << "  --sizes 512 1024 2048      Tailles de terrain a mesurer\n"
        << "  --warmup 50                Frames de chauffe non ecrites dans le CSV\n"
        << "  --frames 500               Frames mesurees par taille\n"
        << "  --output path.csv          Fichier CSV brut\n"
        << "  --force-buffer-update      Met a jour le VBO a chaque frame\n"
        << "  --width 1224               Largeur de la fenetre OpenGL\n"
        << "  --height 868               Hauteur de la fenetre OpenGL\n"
        << "  --help                     Affiche cette aide\n";
}

int parsePositiveInt(const std::string& value, const std::string& optionName)
{
    int parsed = 0;
    try
    {
        parsed = std::stoi(value);
    }
    catch (const std::exception&)
    {
        throw std::runtime_error("Valeur invalide pour " + optionName + ": " + value);
    }

    if (parsed <= 0)
    {
        throw std::runtime_error(optionName + " doit etre strictement positif");
    }
    return parsed;
}

int parseNonNegativeInt(const std::string& value, const std::string& optionName)
{
    int parsed = 0;
    try
    {
        parsed = std::stoi(value);
    }
    catch (const std::exception&)
    {
        throw std::runtime_error("Valeur invalide pour " + optionName + ": " + value);
    }

    if (parsed < 0)
    {
        throw std::runtime_error(optionName + " doit etre positif ou nul");
    }
    return parsed;
}

Options parseOptions(int argc, char** argv)
{
    Options options;

    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h")
        {
            options.help = true;
        }
        else if (arg == "--sizes")
        {
            options.sizes.clear();
            while (i + 1 < argc)
            {
                std::string next = argv[i + 1];
                if (next.rfind("--", 0) == 0)
                {
                    break;
                }
                options.sizes.push_back(parsePositiveInt(next, "--sizes"));
                ++i;
            }

            if (options.sizes.empty())
            {
                throw std::runtime_error("--sizes attend au moins une taille");
            }
        }
        else if (arg == "--warmup")
        {
            if (i + 1 >= argc) throw std::runtime_error("--warmup attend une valeur");
            options.warmup = parseNonNegativeInt(argv[++i], "--warmup");
        }
        else if (arg == "--frames")
        {
            if (i + 1 >= argc) throw std::runtime_error("--frames attend une valeur");
            options.frames = parsePositiveInt(argv[++i], "--frames");
        }
        else if (arg == "--output")
        {
            if (i + 1 >= argc) throw std::runtime_error("--output attend un chemin");
            options.output = argv[++i];
        }
        else if (arg == "--force-buffer-update")
        {
            options.forceBufferUpdate = true;
        }
        else if (arg == "--width")
        {
            if (i + 1 >= argc) throw std::runtime_error("--width attend une valeur");
            options.windowWidth = parsePositiveInt(argv[++i], "--width");
        }
        else if (arg == "--height")
        {
            if (i + 1 >= argc) throw std::runtime_error("--height attend une valeur");
            options.windowHeight = parsePositiveInt(argv[++i], "--height");
        }
        else
        {
            throw std::runtime_error("Option inconnue: " + arg);
        }
    }

    return options;
}

class GlContext
{
public:
    GlContext(int width, int height)
    {
        if (!glfwInit())
        {
            throw std::runtime_error("Impossible d'initialiser GLFW");
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        mWindow = glfwCreateWindow(width, height, "Benchmark rendu terrain", nullptr, nullptr);
        if (!mWindow)
        {
            glfwTerminate();
            throw std::runtime_error("Impossible de creer la fenetre OpenGL");
        }

        glfwMakeContextCurrent(mWindow);
        glewExperimental = GL_TRUE;
        GLenum glewStatus = glewInit();
        glGetError(); // GLEW peut laisser GL_INVALID_ENUM sur certains drivers core profile.

        if (glewStatus != GLEW_OK)
        {
            std::string message = reinterpret_cast<const char*>(glewGetErrorString(glewStatus));
            glfwDestroyWindow(mWindow);
            mWindow = nullptr;
            glfwTerminate();
            throw std::runtime_error("Impossible d'initialiser GLEW: " + message);
        }

        glEnable(GL_DEPTH_TEST);
        glViewport(0, 0, width, height);
        glfwSwapInterval(0); // Evite de mesurer l'attente de synchronisation verticale.
    }

    ~GlContext()
    {
        if (mWindow)
        {
            glfwDestroyWindow(mWindow);
        }
        glfwTerminate();
    }

    GLFWwindow* window() const { return mWindow; }

private:
    GLFWwindow* mWindow = nullptr;
};

void destroyTerrainBuffers(GLuint& vao, GLuint& vbo, GLuint& ibo)
{
    if (vao != 0) glDeleteVertexArrays(1, &vao);
    if (vbo != 0) glDeleteBuffers(1, &vbo);
    if (ibo != 0) glDeleteBuffers(1, &ibo);

    vao = 0;
    vbo = 0;
    ibo = 0;
}

glm::mat4 buildFinalMatrix(int terrainSize, int windowWidth, int windowHeight)
{
    const float size = static_cast<float>(terrainSize);
    const glm::vec3 center{size * 0.5f, 40.0f, size * 0.5f};
    const glm::vec3 eye{size * 0.5f, size * 0.7f + 120.0f, -size * 0.9f};

    const glm::mat4 model(1.0f);
    const glm::mat4 view = glm::lookAt(eye, center, glm::vec3{0.0f, 1.0f, 0.0f});
    const glm::mat4 projection = glm::perspective(
        glm::radians(45.0f),
        static_cast<float>(windowWidth) / static_cast<float>(windowHeight),
        0.1f,
        size * 5.0f + 1000.0f);

    return projection * view * model;
}

FrameTimes renderOneFrame(GLFWwindow* window,
                          Terrain& terrain,
                          Shader& shader,
                          GLuint vao,
                          GLuint vbo,
                          const glm::mat4& finalMatrix,
                          bool forceBufferUpdate)
{
    FrameTimes times;
    const auto frameStart = Clock::now();

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    shader.Use();
    shader.SetMat4("gFinalMatrix", finalMatrix);
    shader.SetFloat("gMaxHeight", terrain.get_max_height());
    shader.SetFloat("gMinHeight", terrain.get_min_height());

    // Dans le cas statique, aucune mise a jour GPU n'est effectuee pendant la mesure.
    if (forceBufferUpdate)
    {
        const auto bufferStart = Clock::now();
        terrain.update_vertices_gpu(vbo);
        glFinish();
        const auto bufferEnd = Clock::now();
        times.bufferUpdateMs = elapsedMs(bufferStart, bufferEnd);
    }

    glBindVertexArray(vao);

    // glFinish force la fin du travail GPU pour rendre la mesure CPU exploitable.
    const auto drawStart = Clock::now();
    terrain.renderer();
    glFinish();
    const auto drawEnd = Clock::now();

    glfwSwapBuffers(window);

    const auto frameEnd = Clock::now();
    times.drawMs = elapsedMs(drawStart, drawEnd);
    times.renderMs = elapsedMs(frameStart, frameEnd);

    return times;
}

void writeHeader(std::ofstream& csv)
{
    csv << "terrain_size,frame_id,render_ms,buffer_update_ms,draw_ms,"
        << "total_vertices,total_triangles,visible_patches,total_patches,fps\n";
}

void runBenchmarkForSize(const Options& options,
                         GLFWwindow* window,
                         Shader& shader,
                         int terrainSize,
                         std::ofstream& csv)
{
    std::cout << "[benchmark_render] Generation du terrain " << terrainSize << " x "
              << terrainSize << std::endl;

    auto terrain = std::make_unique<PerlinNoiseTerrain>();
    terrain->CreatePerlinNoise(terrainSize, terrainSize, 0.0f, 100.0f);

    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ibo = 0;

    // Creation initiale des buffers hors des frames mesurees.
    terrain->setup_terrain(vao, vbo, ibo);
    glFinish();

    const glm::mat4 finalMatrix = buildFinalMatrix(
        terrainSize,
        options.windowWidth,
        options.windowHeight);

    const long long totalVertices = static_cast<long long>(terrain->get_vertices_size()) / 3;
    const long long totalTriangles = static_cast<long long>(terrain->get_indices_size()) / 3;
    const int totalPatches = totalTriangles > 0 ? 1 : 0;
    const int visiblePatches = totalPatches;

    std::cout << "[benchmark_render] Warm-up: " << options.warmup
              << " frames, mesure: " << options.frames << " frames" << std::endl;

    for (int i = 0; i < options.warmup && !glfwWindowShouldClose(window); ++i)
    {
        renderOneFrame(window, *terrain, shader, vao, vbo, finalMatrix, options.forceBufferUpdate);
        glfwPollEvents();
    }

    for (int frame = 0; frame < options.frames && !glfwWindowShouldClose(window); ++frame)
    {
        const FrameTimes times = renderOneFrame(
            window,
            *terrain,
            shader,
            vao,
            vbo,
            finalMatrix,
            options.forceBufferUpdate);

        const double fps = times.renderMs > 0.0 ? 1000.0 / times.renderMs : 0.0;

        csv << terrainSize << ','
            << frame << ','
            << times.renderMs << ','
            << times.bufferUpdateMs << ','
            << times.drawMs << ','
            << totalVertices << ','
            << totalTriangles << ','
            << visiblePatches << ','
            << totalPatches << ','
            << fps << '\n';

        glfwPollEvents();
    }

    destroyTerrainBuffers(vao, vbo, ibo);
}

} // namespace

int main(int argc, char** argv)
{
    try
    {
        const Options options = parseOptions(argc, argv);
        if (options.help)
        {
            printUsage(argv[0]);
            return 0;
        }

        std::srand(1);

        const std::filesystem::path outputPath(options.output);
        const std::filesystem::path parentPath = outputPath.parent_path();
        if (!parentPath.empty())
        {
            std::filesystem::create_directories(parentPath);
        }

        GlContext context(options.windowWidth, options.windowHeight);

        std::ofstream csv(outputPath);
        if (!csv)
        {
            throw std::runtime_error("Impossible d'ouvrir le fichier de sortie: " + options.output);
        }
        csv << std::fixed << std::setprecision(6);
        writeHeader(csv);

        const std::filesystem::path shaderDir =
            std::filesystem::path(BENCHMARK_PROJECT_ROOT) / "shaders";
        Shader shader(
            (shaderDir / "terrain.vs").string(),
            (shaderDir / "terrain.fs").string());

        for (const int terrainSize : options.sizes)
        {
            if (glfwWindowShouldClose(context.window()))
            {
                break;
            }
            runBenchmarkForSize(options, context.window(), shader, terrainSize, csv);
        }

        std::cout << "[benchmark_render] Resultats ecrits dans " << options.output << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[benchmark_render] Erreur: " << e.what() << std::endl;
        printUsage(argv[0]);
        return 1;
    }

    return 0;
}
