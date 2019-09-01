#ifndef SHADER_HPP
#define SHADER_HPP

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>
#include <fstream>
#include <cerrno>
#include <sstream>
#include <memory>
#include <cassert>
#include <initializer_list>

class Shader
{
public:
    Shader(GLenum shaderType, const std::string& filename);
    ~Shader();

    GLuint id() const { return mHandle; }

private:
    GLuint mHandle;
    GLuint loadShader(GLenum shaderType, const std::string& filename);
};

class Program
{
public:
    Program(std::initializer_list<Shader> shaders);
    ~Program();

    GLuint id() const { return mHandle; }

private:
    GLuint mHandle;
    GLuint loadProgram(std::initializer_list<Shader> shaders);

    std::initializer_list<Shader> mShaders;

};

#endif // SHADER_HPP