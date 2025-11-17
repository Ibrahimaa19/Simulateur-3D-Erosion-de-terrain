#ifndef TEST_SHADER_HPP
#define TEST_SHADER_HPP

#include <gtest/gtest.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

/**
 * @class ShaderTest
 * @brief Fixture pour tester la classe Shader avec contexte OpenGL.
 *
 * Initialise un contexte OpenGL minimal pour pouvoir créer et compiler
 * des shaders dans les tests.
 */
class ShaderTest : public ::testing::Test
{
protected:
    GLFWwindow* mWindow = nullptr;

    void SetUp() override
    {
        // Initialiser GLFW
        if (!glfwInit()) {
            FAIL() << "Impossible d'initialiser GLFW";
        }

        // Créer une fenêtre invisible
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        mWindow = glfwCreateWindow(100, 100, "Test OpenGL", nullptr, nullptr);
        if (!mWindow) {
            glfwTerminate();
            FAIL() << "Impossible de créer une fenêtre GLFW pour les tests";
        }

        glfwMakeContextCurrent(mWindow);

        // Initialiser GLEW
        glewExperimental = GL_TRUE;
        GLenum err = glewInit();
        if (err != GLEW_OK) {
            std::cerr << "Failed to initialize GLEW: "
                    << glewGetErrorString(err) << std::endl;
        }
    }

    void TearDown() override
    {
        if (mWindow) {
            glfwDestroyWindow(mWindow);
            glfwTerminate();
            mWindow = nullptr;
        }
    }
};

#endif // TEST_SHADER_HPP
