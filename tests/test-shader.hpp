#pragma once

#include <gtest/gtest.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

/**
 * @class ShaderTest
 * @brief Fixture for testing the Shader class with an OpenGL context.
 *
 * Initializes a minimal OpenGL context to create and compile
 * shaders in tests.
 */
class ShaderTest : public ::testing::Test
{
protected:
    GLFWwindow* mWindow = nullptr;

    void SetUp() override
    {
        // Initialize GLFW
        if (!glfwInit()) {
            FAIL() << "Failed to initialize GLFW";
        }

        // Create an invisible window
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        mWindow = glfwCreateWindow(100, 100, "Test OpenGL", nullptr, nullptr);
        if (!mWindow) {
            glfwTerminate();
            FAIL() << "Failed to create a GLFW window for tests";
        }

        glfwMakeContextCurrent(mWindow);

        // Initialize GLEW
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
