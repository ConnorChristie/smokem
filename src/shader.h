#ifndef SHADER_HPP
#define SHADER_HPP

#include <glad/glad.h>
#include <string>
#include <initializer_list>

class Shader
{
public:
    Shader(GLenum shaderType, const std::string &filename);
    ~Shader();

    GLuint getHandle() const { return mHandle; }

private:
    GLuint mHandle;
};

#endif // SHADER_HPP