#define STB_IMAGE_IMPLEMENTATION

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include "Shader.hpp"
#include "Camera.hpp"

#include "terrain.hpp"
#include "ThermalErosion.h"


Camera camera;
const float cameraSpeed = 0.1f;

// Screen size globals
int SCR_WIDTH = 1224;
int SCR_HEIGHT = 868;

// Mouse tracking globals
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;
float mouseSensitivity = 0.1f;

// Transform matrices globals
glm::mat4 model = glm::mat4(1.0f);
glm::mat4 view;
glm::mat4 projection;

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    Vertex() {}
    Vertex(float x, float y, float z, float r, float g, float b) {
        pos = glm::vec3{x, y, z};
        color = glm::vec3{r, g, b};
    }
};

void ShowMenu()
{
    std::cout << "================== CAMERA CONTROL MENU ==================\n";
    std::cout << "Keyboard :\n";
    std::cout << "  W : Move forward\n";
    std::cout << "  S : Move backward\n";
    std::cout << "  A : Move left\n";
    std::cout << "  D : Move right\n";
    std::cout << "  Q : Move up\n";
    std::cout << "  E : Move down\n";
    std::cout << "  ESC : Quit\n";
    std::cout << "  H : Show this menu\n\n";
    std::cout << "Mouse :\n";
    std::cout << "  Move the mouse to rotate the camera\n";
    std::cout << "  Scroll wheel : Zoom in/out\n";
    std::cout << "=========================================================\n";
}

// Callback
void FramebufferSizeCallback(GLFWwindow* window, int width, int height) {
    SCR_WIDTH = width;
    SCR_HEIGHT = height;
    glViewport(0, 0, width, height);

    // Update projection matrix
    projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);

    // Reset mouse to center
    lastX = width / 2.0f;
    lastY = height / 2.0f;
}

// Keyboard input
void HandleKeyBoardInput(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {
        switch (key)
        {
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, GLFW_TRUE);
            break;
        case GLFW_KEY_W:
            camera.Move(camera.GetForward(), cameraSpeed);
            break;
        case GLFW_KEY_S:
            camera.Move(-camera.GetForward(), cameraSpeed);
            break;
        case GLFW_KEY_D:
            camera.Move(camera.GetRight(), cameraSpeed);
            break;
        case GLFW_KEY_A:
            camera.Move(-camera.GetRight(), cameraSpeed);
            break;
        case GLFW_KEY_Q:
            camera.Move(camera.GetUp(), cameraSpeed);
            break;
        case GLFW_KEY_E:
            camera.Move(-camera.GetUp(), cameraSpeed);
            break;
        case GLFW_KEY_H:
            ShowMenu();
            break;
        }
    }
}

void HandleMouseInput(GLFWwindow *window, double xpos, double ypos)
{
    if(firstMouse)
    {
        lastX = (float)xpos;
        lastY = (float)ypos;
        firstMouse = false;
    }

    float xoffset = (float)(xpos - lastX);
    float yoffset = (float)(ypos - lastY);

    lastX = (float)xpos;
    lastY = (float)ypos;

    xoffset *= mouseSensitivity;
    yoffset *= mouseSensitivity;

    camera.Yaw(xoffset);
    camera.Pitch(yoffset);
}

void HandleScrollCallback(GLFWwindow *window, double xoffset, double yoffset)
{
    camera.Move(camera.GetForward(), (float)yoffset);
}

int main() {
    // --- Initialize GLFW
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Height Map", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
    glfwSetKeyCallback(window, HandleKeyBoardInput);
    glfwSetCursorPosCallback(window, HandleMouseInput);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetScrollCallback(window, HandleScrollCallback);

    // --- Initialize GLEW
    glewInit();
    glEnable(GL_DEPTH_TEST);

    GLuint VBO, VAO, IBO;

    Terrain mainTerrain = Terrain("../src/heightmap/iceland_heightmap.png");
    

    ThermalErosion thermal = ThermalErosion();
    for (size_t i =0;i<20;i++){
        thermal.step(mainTerrain);
    }

    mainTerrain.load_incides();
	mainTerrain.load_vectices();
    mainTerrain.setup_terrain(VBO, VAO, IBO);
    


    // --- Shader
    Shader shader("../Shaders/shader.vs", "../Shaders/shader.fs");
    shader.Use();

    // --- Camera setup
    camera.MoveTo(glm::vec3{0.0f, 0.0f, 3.0f});
    camera.TurnTo(glm::vec3{0.0f, 0.0f, 0.0f});

    // --- Initialize global matrices
    model = glm::mat4(1.0f);
    view = camera.GetViewMatrix();
    projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);

    while (!glfwWindowShouldClose(window)) {

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        view = camera.GetViewMatrix();
        glm::mat4 finalMatrix = projection * view * model;
        shader.SetMat4("gFinalMatrix", finalMatrix);


        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, mainTerrain.indices.size(), GL_UNSIGNED_INT, 0);
        

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &IBO);

    glfwTerminate();
    return 0;
}
