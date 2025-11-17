#include "test-shader.hpp"
#include "../src/Shader.hpp"
#include <fstream>
#include <cstdio>

/**
 * @brief Test : lecture et compilation minimale d'un shader
 */
TEST_F(ShaderTest, CanCompileShader)
{
    const char* vertPath = "temp.vert";
    const char* fragPath = "temp.frag";

    // Créer un vertex shader minimal
    {
        std::ofstream file(vertPath);
        file << "#version 330 core\nvoid main(){}";
    }

    // Créer un fragment shader minimal
    {
        std::ofstream file(fragPath);
        file << "#version 330 core\nout vec4 FragColor; void main(){ FragColor = vec4(1.0); }";
    }

    // Instancier Shader
    Shader shader(vertPath, fragPath);
    EXPECT_NE(shader.GetProgramID(), 0u);

    // Nettoyer fichiers temporaires
    std::remove(vertPath);
    std::remove(fragPath);
}

/**
 * @brief Test : destruction correcte du shader
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
    } // shader détruit ici

    // On ne peut pas tester glDeleteProgram directement, mais aucun crash = succès

    std::remove(vertPath);
    std::remove(fragPath);
}
