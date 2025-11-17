#include "Shader.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

/**
 * @brief Reads a text file and returns its content.
 * @param path Path to the file
 * @return File content as a string, or an empty string on error
 */
std::string Shader::ReadFile(const std::string &path) const
{
    std::ifstream file(path);
    if (!file)
    {
        std::cerr << "Error: unable to open file " << path << std::endl;
        return "";
    }

    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

/**
 * @brief Compiles an OpenGL shader of a given type.
 * @param source GLSL source code
 * @param type OpenGL shader type (e.g., GL_VERTEX_SHADER, GL_FRAGMENT_SHADER)
 * @return GLuint ID of the compiled shader
 *
 * Prints the compilation error log to stderr if compilation fails.
 */
GLuint Shader::CompileShader(const std::string &source, GLenum type) const
{
    GLuint shader = glCreateShader(type);
    const char *src = source.c_str();

    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (!success)
    {
        char log[1024];
        glGetShaderInfoLog(shader, 1024, nullptr, log);
        std::cerr << "[Shader Compile Error] "
                  << (type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT")
                  << ":\n" << log << std::endl;
    }

    return shader;
}

/**
 * @brief Builds the complete shader program by reading, compiling,
 *        and linking vertex and fragment shaders.
 *
 * @param vertexPath   Path to the vertex shader file
 * @param fragmentPath Path to the fragment shader file
 *
 * Prints linking errors to stderr if linking fails.
 */
Shader::Shader(const std::string &vertexPath, const std::string &fragmentPath)
{
    std::string vertexCode   = ReadFile(vertexPath);
    std::string fragmentCode = ReadFile(fragmentPath);

    GLuint vert = CompileShader(vertexCode, GL_VERTEX_SHADER);
    GLuint frag = CompileShader(fragmentCode, GL_FRAGMENT_SHADER);

    mProgramID = glCreateProgram();
    glAttachShader(mProgramID, vert);
    glAttachShader(mProgramID, frag);

    glLinkProgram(mProgramID);

    GLint success;
    glGetProgramiv(mProgramID, GL_LINK_STATUS, &success);
    if (!success)
    {
        char log[1024];
        glGetProgramInfoLog(mProgramID, 1024, nullptr, log);
        std::cerr << "[Shader Link Error]:\n" << log << std::endl;
    }

    glDeleteShader(vert);
    glDeleteShader(frag);
}

/**
 * @brief Destructor that deletes the OpenGL shader program.
 */
Shader::~Shader()
{
    glDeleteProgram(mProgramID);
}

/**
 * @brief Activates the shader program so that it is used by OpenGL.
 */
void Shader::Use() const
{
    glUseProgram(mProgramID);
}

// -----------------------------------------------------------------------------
//                               UNIFORM SETTERS
// -----------------------------------------------------------------------------

/**
 * @brief Sets a boolean uniform.
 * @param name Name of the uniform variable
 * @param value Boolean value
 */
void Shader::SetBool(const std::string &name, bool value) const
{
    glUniform1i(glGetUniformLocation(mProgramID, name.c_str()), (int)value);
}

/**
 * @brief Sets an integer uniform.
 * @param name Name of the uniform variable
 * @param value Integer value
 */
void Shader::SetInt(const std::string &name, int value) const
{
    glUniform1i(glGetUniformLocation(mProgramID, name.c_str()), value);
}

/**
 * @brief Sets a float uniform.
 * @param name Name of the uniform variable
 * @param value Float value
 */
void Shader::SetFloat(const std::string &name, float value) const
{
    glUniform1f(glGetUniformLocation(mProgramID, name.c_str()), value);
}

/**
 * @brief Sets a vec2 uniform.
 * @param name Name of the uniform
 * @param value 2D vector value
 */
void Shader::SetVec2(const std::string &name, const glm::vec2 &value) const
{
    glUniform2fv(glGetUniformLocation(mProgramID, name.c_str()), 1, &value[0]);
}

/**
 * @brief Sets a vec3 uniform.
 * @param name Name of the uniform
 * @param value 3D vector value
 */
void Shader::SetVec3(const std::string &name, const glm::vec3 &value) const
{
    glUniform3fv(glGetUniformLocation(mProgramID, name.c_str()), 1, &value[0]);
}

/**
 * @brief Sets a vec4 uniform.
 * @param name Name of the uniform
 * @param value 4D vector value
 */
void Shader::SetVec4(const std::string &name, const glm::vec4 &value) const
{
    glUniform4fv(glGetUniformLocation(mProgramID, name.c_str()), 1, &value[0]);
}

/**
 * @brief Sets a mat4 uniform.
 * @param name Name of the uniform
 * @param mat 4x4 matrix
 */
void Shader::SetMat4(const std::string &name, const glm::mat4 &mat) const
{
    glUniformMatrix4fv(glGetUniformLocation(mProgramID, name.c_str()),
                       1, GL_FALSE, &mat[0][0]);
}

/**
 * @brief Sets a mat3 uniform.
 * @param name Name of the uniform
 * @param mat 3x3 matrix
 */
void Shader::SetMat3(const std::string &name, const glm::mat3 &mat) const
{
    glUniformMatrix3fv(glGetUniformLocation(mProgramID, name.c_str()),
                       1, GL_FALSE, &mat[0][0]);
}
