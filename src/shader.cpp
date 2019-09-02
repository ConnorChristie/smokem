#include "shader.h"

Shader::Shader(GLenum shaderType, const std::string& filename)
    : mHandle(loadShader(shaderType, filename))
{
}

Shader::~Shader()
{
    if (mHandle > 0) glDeleteShader(mHandle);
}

GLuint Shader::loadShader(GLenum shaderType, const std::string& filename)
{
    std::ifstream shaderFile(filename);
    if (!shaderFile.is_open()) return 0;

    std::string shaderText(static_cast<const std::stringstream&>(std::stringstream() << shaderFile.rdbuf()).str());

    auto shader = glCreateShader(shaderType);
    if (shader == 0) return 0;

    const char* shaderTextCstr = shaderText.c_str();
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
        printf("Shader compile error: %s\n", strInfoLog.get());

        exit(1);
    }

    return shader;
}

Program::Program(std::initializer_list<Shader> shaders)
    : mHandle(loadProgram(shaders))
{
}

Program::~Program()
{
    if (mHandle > 0) glDeleteProgram(mHandle);
}

GLuint Program::loadProgram(std::initializer_list<Shader> shaders)
{
    mShaders = shaders;

    auto program = glCreateProgram();
    if (program == 0) return 0;

    for (auto it = shaders.begin(); it != shaders.end(); ++it)
    {
        glAttachShader(program, it->id());
    }

    GLint status;

    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &status);

    if (status == GL_FALSE)
    {
        GLint infoLogLength;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);

        std::unique_ptr<GLchar[]> strInfoLog(new GLchar[infoLogLength]);
        glGetProgramInfoLog(program, infoLogLength, nullptr, strInfoLog.get());
    }

    GLchar compilerSpew[256];
    GLint linkSuccess;

    glGetProgramiv(program, GL_LINK_STATUS, &linkSuccess);
    glGetProgramInfoLog(program, sizeof(compilerSpew), 0, compilerSpew);

    if (!linkSuccess)
    {
        printf("Link error.\n");
        printf("%s\n", compilerSpew);
    }

    return program;
}

void Program::bindDefaultAttribs()
{
    glUseProgram(mHandle);
    glBindAttribLocation(mHandle, 0, "Position");
    glBindAttribLocation(mHandle, 1, "TexCoord");
}
