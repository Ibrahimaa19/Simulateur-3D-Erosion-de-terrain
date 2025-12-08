#pragma once

#include <string>
#include <GL/glew.h>
#include <glm/glm.hpp>

/**
 * @class Shader
 * @brief Full management of an OpenGL shader program.
 *
 * This class provides:
 *   - loading GLSL files,
 *   - shader compilation,
 *   - program linking,
 *   - uniform handling (int, float, vectors, matrices),
 *   - program usage within the OpenGL pipeline.
 */
class Shader
{
public:
    /**
     * @brief Constructs the shader program from GLSL file paths.
     * @param vertexPath Path to the vertex shader 
     * @param fragmentPath Path to the fragment shader
     */
    Shader(const std::string &vertexPath, const std::string &fragmentPath);

    /// @brief Destructor
    ~Shader();

    /**
     * @brief Activates the shader program.
     */
    void Use() const;

    /**
     * @brief Returns the OpenGL shader program ID.
     * @return GLuint Shader program identifier
     */
    GLuint GetProgramID() const { return mProgramID; }

    // -------------------------------------------------------------------------
    //                             UNIFORM HANDLING
    // -------------------------------------------------------------------------

    /// @brief Sets a boolean uniform.
    void SetBool(const std::string &name, bool value) const;

    /// @brief Sets an integer uniform.
    void SetInt(const std::string &name, int value) const;

    /// @brief Sets a float uniform.
    void SetFloat(const std::string &name, float value) const;

    /// @brief Sets a vec2 uniform.
    void SetVec2(const std::string &name, const glm::vec2 &value) const;

    /// @brief Sets a vec3 uniform.
    void SetVec3(const std::string &name, const glm::vec3 &value) const;

    /// @brief Sets a vec4 uniform.
    void SetVec4(const std::string &name, const glm::vec4 &value) const;

    /// @brief Sets a mat4 uniform.
    void SetMat4(const std::string &name, const glm::mat4 &mat) const;

    /// @brief Sets a mat3 uniform.
    void SetMat3(const std::string &name, const glm::mat3 &mat) const;

private:
    GLuint mProgramID; ///< OpenGL shader program identifier.

    /**
     * @brief Reads a text file and returns its content as a string.
     * @param path Path to the file
     * @return File content as a string
     */
    std::string ReadFile(const std::string &path) const;

    /**
     * @brief Compiles a shader from source code.
     * @param source GLSL source code
     * @param type Shader type (vertex, fragment...)
     * @return GLuint ID of the compiled shader
     */
    GLuint CompileShader(const std::string &source, GLenum type) const;
};
