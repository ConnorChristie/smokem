#include "shader.h"
#include <fstream>
#include <cerrno>
#include <sstream>
#include <memory>

GLuint loadShader(GLenum shaderType, const std::string &filename)
{
    std::ifstream shaderFile(filename);

    if (!shaderFile.is_open())
    {
        return 0;
    }

    std::string shaderText(static_cast<const std::stringstream &>(std::stringstream() << shaderFile.rdbuf()).str());

    GLuint shader = glCreateShader(shaderType);
    if (shader == 0)
    {
        return 0;
    }

    // For some reason I can't get &shaderText.c_str() to work...
    const char *shaderTextCstr = shaderText.c_str();
    glShaderSource(shader, 1, &shaderTextCstr, nullptr);
    glCompileShader(shader);

    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE)
    {
        GLint infoLogLength;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);

        std::unique_ptr<GLchar[]> strInfoLog(new GLchar[infoLogLength]);
        glGetShaderInfoLog(shader, infoLogLength, nullptr, strInfoLog.get());
        printf("Shader compile error: %s", strInfoLog.get());

        std::string strShaderType;
        switch (shaderType)
        {
        case GL_VERTEX_SHADER:
            strShaderType = "vertex";
            break;
        case GL_GEOMETRY_SHADER:
            strShaderType = "geometry";
            break;
        case GL_FRAGMENT_SHADER:
            strShaderType = "fragment";
            break;
        }
    }

    return shader;
}

Shader::Shader(GLenum shaderType, const std::string &filename) : mHandle(loadShader(shaderType, filename))
{
}

Shader::~Shader()
{
    if (mHandle > 0)
    {
        glDeleteShader(mHandle);
    }
}
