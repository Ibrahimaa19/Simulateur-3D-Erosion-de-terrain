#include "test-shader.hpp"
#include "Shader.hpp"
#include <fstream>
#include <cstdio>

/**
 * @brief Test: minimal shader reading and compilation
 */
TEST_F(ShaderTest, CanCompileShader)
{
    const char* vertPath = "temp.vert";
    const char* fragPath = "temp.frag";

    // Create a minimal vertex shader
    {
        std::ofstream file(vertPath);
        file << "#version 330 core\nvoid main(){}";
    }

    // Create a minimal fragment shader
    {
        std::ofstream file(fragPath);
        file << "#version 330 core\nout vec4 FragColor; void main(){ FragColor = vec4(1.0); }";
    }

    // Instantiate Shader
    Shader shader(vertPath, fragPath);
    EXPECT_NE(shader.GetProgramID(), 0u);

    // Clean up temporary files
    std::remove(vertPath);
    std::remove(fragPath);
}

/**
 * @brief Test: proper shader destruction
 */
TEST_F(ShaderTest, ShaderDestructsProperly)
{
    const char* vertPath = "temp.vert";
    const char* fragPath = "temp.frag";

    std::ofstream(vertPath) << "#version 330 core\nvoid main(){}";
    std::ofstream(fragPath) << "#version 330 core\nout vec4 FragColor; void main(){ FragColor = vec4(1.0); }";

    GLuint programID = 0;
    {
        Shader shader(vertPath, fragPath);
        programID = shader.GetProgramID();
        EXPECT_NE(programID, 0u);
    } // shader destroyed here

    // We cannot test glDeleteProgram directly, but no crash = success

    std::remove(vertPath);
    std::remove(fragPath);
}
