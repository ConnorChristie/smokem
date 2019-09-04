#include "utility.h"

static struct {
    GLuint Advect;
    GLuint Jacobi;
    GLuint SubtractGradient;
    GLuint ComputeDivergence;
    GLuint ApplyImpulse;
    GLuint ApplyBuoyancy;
} Programs;

const float CellSize = 1.25f;
const int GridWidth = 128;
const int GridHeight = 128;
const int GridDepth = 128;
const float SplatRadius = GridWidth / 8.0f;
const float AmbientTemperature = 0.0f;
const int NumJacobiIterations = 40;
const float TimeStep = 0.25f;
const float SmokeBuoyancy = 1.0f;
const float SmokeWeight = 0.0f;
const float GradientScale = 1.125f / CellSize;

const glm::vec3 ImpulsePosition(GridWidth / 2.0f, GridHeight - (int) SplatRadius / 2.0f, GridDepth / 2.0f);

static std::map<std::pair<GLuint, std::string>, GLuint> uniformCache;

GLuint makeProgram(std::initializer_list<Shader> shaders)
{
    GLuint program = glCreateProgram();
    if (program == 0) return 0;

    for (auto it = shaders.begin(); it != shaders.end(); ++it)
    {
        glAttachShader(program, it->id());
    }

    glLinkProgram(program);

    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status == GL_FALSE)
    {
        GLint infoLogLength;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);

        std::unique_ptr<GLchar[]> strInfoLog(new GLchar[infoLogLength]);
        glGetProgramInfoLog(program, infoLogLength, nullptr, strInfoLog.get());
    }

    glLinkProgram(program);

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

void InitializeSlabPrograms()
{
    Programs.Advect = makeProgram({
        Shader(GL_VERTEX_SHADER, "shaders/fluid/fluid.vert"),
        Shader(GL_GEOMETRY_SHADER, "shaders/fluid/pick-layer.gs"),
        Shader(GL_FRAGMENT_SHADER, "shaders/fluid/advect.frag")
    });

    Programs.Jacobi = makeProgram({
        Shader(GL_VERTEX_SHADER, "shaders/fluid/fluid.vert"),
        Shader(GL_GEOMETRY_SHADER, "shaders/fluid/pick-layer.gs"),
        Shader(GL_FRAGMENT_SHADER, "shaders/fluid/jacobi.frag")
    });

    Programs.SubtractGradient = makeProgram({
        Shader(GL_VERTEX_SHADER, "shaders/fluid/fluid.vert"),
        Shader(GL_GEOMETRY_SHADER, "shaders/fluid/pick-layer.gs"),
        Shader(GL_FRAGMENT_SHADER, "shaders/fluid/subtract-gradient.frag")
    });

    Programs.ComputeDivergence = makeProgram({
        Shader(GL_VERTEX_SHADER, "shaders/fluid/fluid.vert"),
        Shader(GL_GEOMETRY_SHADER, "shaders/fluid/pick-layer.gs"),
        Shader(GL_FRAGMENT_SHADER, "shaders/fluid/divergence.frag")
    });

    Programs.ApplyImpulse = makeProgram({
        Shader(GL_VERTEX_SHADER, "shaders/fluid/fluid.vert"),
        Shader(GL_GEOMETRY_SHADER, "shaders/fluid/pick-layer.gs"),
        Shader(GL_FRAGMENT_SHADER, "shaders/fluid/impulse.frag")
    });

    Programs.ApplyBuoyancy = makeProgram({
        Shader(GL_VERTEX_SHADER, "shaders/fluid/fluid.vert"),
        Shader(GL_GEOMETRY_SHADER, "shaders/fluid/pick-layer.gs"),
        Shader(GL_FRAGMENT_SHADER, "shaders/fluid/buoyancy.frag")
    });
}

void CreateObstacles(SurfacePod dest)
{
    glBindFramebuffer(GL_FRAMEBUFFER, dest.FboHandle);
    glViewport(0, 0, dest.Width, dest.Height);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    assert(checkError());

    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    auto program = makeProgram({
        Shader(GL_VERTEX_SHADER, "shaders/fluid/fluid.vert"),
        Shader(GL_FRAGMENT_SHADER, "shaders/fluid/fill.frag")
    });
    assert(checkError());

    glUseProgram(program);
    assert(checkError());

    GLuint lineVbo;
    glGenBuffers(1, &lineVbo);

    GLuint circleVbo;
    glGenBuffers(1, &circleVbo);

    auto posAttr = glGetAttribLocation(program, "Position");
    glEnableVertexAttribArray(posAttr);

    assert(checkError());

    for (int slice = 0; slice < dest.Depth; ++slice)
    {
        glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, dest.ColorTexture, 0, dest.Depth - 1 - slice);

        float z = dest.Depth / 2.0f;
        z = std::abs(slice - z) / z;
        float fraction = 1 - sqrt(z);
        float radius = 0.25f * fraction;

        if (slice == 0 || slice == dest.Depth - 1)
        {
            radius *= 100;
        }

        const bool DrawBorder = true;
        if (DrawBorder && slice != 0 && slice != dest.Depth - 1)
        {
            #define T 0.9999f
            float positions[] = { -T, -T, T, -T, T,  T, -T,  T, -T, -T };
            #undef T

            GLsizeiptr size = sizeof(positions);
            glBindBuffer(GL_ARRAY_BUFFER, lineVbo);
            glBufferData(GL_ARRAY_BUFFER, size, positions, GL_STATIC_DRAW);

            GLsizeiptr stride = 2 * sizeof(positions[0]);
            glVertexAttribPointer(posAttr, 2, GL_FLOAT, GL_FALSE, stride, 0);
            glDrawArrays(GL_LINE_STRIP, 0, 5);
        }

        const bool DrawSphere = false;
        if (DrawSphere || slice == 0 || slice == dest.Depth - 1)
        {
            const int slices = 64;
            float positions[slices * 2 * 3];
            float twopi = 8 * atan(1.0f);
            float theta = 0;
            float dtheta = twopi / (float) (slices - 1);
            float* pPositions = &positions[0];

            for (int i = 0; i < slices; i++)
            {
                *pPositions++ = 0;
                *pPositions++ = 0;

                *pPositions++ = radius * cos(theta);
                *pPositions++ = radius * sin(theta);
                theta += dtheta;

                *pPositions++ = radius * cos(theta);
                *pPositions++ = radius * sin(theta);
            }

            GLsizeiptr size = sizeof(positions);
            glBindBuffer(GL_ARRAY_BUFFER, circleVbo);
            glBufferData(GL_ARRAY_BUFFER, size, positions, GL_STATIC_DRAW);

            GLsizeiptr stride = 2 * sizeof(positions[0]);
            glVertexAttribPointer(posAttr, 2, GL_FLOAT, GL_FALSE, stride, 0);
            glDrawArrays(GL_TRIANGLES, 0, slices * 3);
        }
    }

    // Cleanup
    glDeleteProgram(program);
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &lineVbo);
    glDeleteBuffers(1, &circleVbo);
}

SlabPod CreateSlab(GLsizei width, GLsizei height, GLsizei depth, int numComponents)
{
    SlabPod slab;
    slab.Ping = CreateVolume(width, height, depth, numComponents);
    slab.Pong = CreateVolume(width, height, depth, numComponents);
    return slab;
}

SurfacePod CreateSurface(GLsizei width, GLsizei height, int numComponents)
{
    GLuint fboHandle;
    glGenFramebuffers(1, &fboHandle);
    glBindFramebuffer(GL_FRAMEBUFFER, fboHandle);

    GLuint textureHandle;
    glGenTextures(1, &textureHandle);
    glBindTexture(GL_TEXTURE_2D, textureHandle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    switch (numComponents)
    {
        case 1:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, width, height, 0, GL_RED, GL_HALF_FLOAT, 0);
            break;
        case 2:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, width, height, 0, GL_RG, GL_HALF_FLOAT, 0);
            break;
        case 3:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_HALF_FLOAT, 0);
            break;
        case 4:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_HALF_FLOAT, 0);
            break;
    }

    GLuint colorbuffer;
    glGenRenderbuffers(1, &colorbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, colorbuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureHandle, 0);
    
    SurfacePod surface = { fboHandle, textureHandle };

    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    surface.Width = width;
    surface.Height = height;
    surface.Depth = 1;
    return surface;
}

SurfacePod CreateVolume(GLsizei width, GLsizei height, GLsizei depth, int numComponents)
{
    GLuint fboHandle;
    glGenFramebuffers(1, &fboHandle);
    glBindFramebuffer(GL_FRAMEBUFFER, fboHandle);

    GLuint textureHandle;
    glGenTextures(1, &textureHandle);
    glBindTexture(GL_TEXTURE_3D, textureHandle);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    switch (numComponents)
    {
        case 1:
            glTexImage3D(GL_TEXTURE_3D, 0, GL_R16F, width, height, depth, 0, GL_RED, GL_HALF_FLOAT, 0);
            break;
        case 2:
            glTexImage3D(GL_TEXTURE_3D, 0, GL_RG16F, width, height, depth, 0, GL_RG, GL_HALF_FLOAT, 0);
            break;
        case 3:
            glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB16F, width, height, depth, 0, GL_RGB, GL_HALF_FLOAT, 0);
            break;
        case 4:
            glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA16F, width, height, depth, 0, GL_RGBA, GL_HALF_FLOAT, 0);
            break;
    }

    GLuint colorbuffer;
    glGenRenderbuffers(1, &colorbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, colorbuffer);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, textureHandle, 0);

    SurfacePod surface = { fboHandle, textureHandle };

    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    surface.Width = width;
    surface.Height = height;
    surface.Depth = depth;
    return surface;
}

GLuint CreateQuadVbo()
{
    short positions[] = {
        -1, -1,
         1, -1,
        -1,  1,
         1,  1,
    };
    GLuint vbo;
    GLsizeiptr size = sizeof(positions);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, size, positions, GL_STATIC_DRAW);
    return vbo;
}

void Advect(SurfacePod velocity, SurfacePod source, SurfacePod obstacles, SurfacePod dest, float dissipation)
{
    GLuint pid = Programs.Advect;
    glUseProgram(pid);
    SetUniform(pid, "InverseSize", 1.0f / glm::vec3(GridWidth, GridHeight, GridDepth));
    SetUniform(pid, "TimeStep", TimeStep);
    SetUniform(pid, "Dissipation", dissipation);
    SetUniform(pid, "VelocityTexture", 0);
    SetUniform(pid, "SourceTexture", 1);
    SetUniform(pid, "Obstacles", 2);

    glBindFramebuffer(GL_FRAMEBUFFER, dest.FboHandle);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, velocity.ColorTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_3D, source.ColorTexture);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_3D, obstacles.ColorTexture);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, dest.Depth);

    ResetState();
}

void Jacobi(SurfacePod pressure, SurfacePod divergence, SurfacePod obstacles, SurfacePod dest)
{
    GLuint pid = Programs.Jacobi;
    glUseProgram(pid);
    SetUniform(pid, "Alpha", -CellSize * CellSize);
    SetUniform(pid, "InverseBeta", 0.1666f);
    SetUniform(pid, "Pressure", 0);
    SetUniform(pid, "Divergence", 1);
    SetUniform(pid, "Obstacles", 2);

    glBindFramebuffer(GL_FRAMEBUFFER, dest.FboHandle);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, pressure.ColorTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_3D, divergence.ColorTexture);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_3D, obstacles.ColorTexture);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, dest.Depth);
    ResetState();
}

void SubtractGradient(SurfacePod velocity, SurfacePod pressure, SurfacePod obstacles, SurfacePod dest)
{
    GLuint pid = Programs.SubtractGradient;
    glUseProgram(pid);
    SetUniform(pid, "GradientScale", GradientScale);
    SetUniform(pid, "HalfInverseCellSize", 0.5f / CellSize);
    SetUniform(pid, "Velocity", 0);
    SetUniform(pid, "Pressure", 1);
    SetUniform(pid, "Obstacles", 2);

    glBindFramebuffer(GL_FRAMEBUFFER, dest.FboHandle);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, velocity.ColorTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_3D, pressure.ColorTexture);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_3D, obstacles.ColorTexture);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, dest.Depth);
    ResetState();
}

void ComputeDivergence(SurfacePod velocity, SurfacePod obstacles, SurfacePod dest)
{
    GLuint pid = Programs.ComputeDivergence;
    glUseProgram(pid);
    SetUniform(pid, "HalfInverseCellSize", 0.5f / CellSize);
    SetUniform(pid, "Velocity", 0);
    SetUniform(pid, "Obstacles", 1);

    glBindFramebuffer(GL_FRAMEBUFFER, dest.FboHandle);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, velocity.ColorTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_3D, obstacles.ColorTexture);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, dest.Depth);
    ResetState();
}

void ApplyImpulse(SurfacePod dest, glm::vec3 position, float value)
{
    GLuint pid = Programs.ApplyImpulse;
    glUseProgram(pid);
    SetUniform(pid, "Point", position);
    SetUniform(pid, "Radius", SplatRadius);
    SetUniform(pid, "FillColor", glm::vec3(value));

    glBindFramebuffer(GL_FRAMEBUFFER, dest.FboHandle);
    glEnable(GL_BLEND);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, dest.Depth);
    ResetState();
}

void ApplyBuoyancy(SurfacePod velocity, SurfacePod temperature, SurfacePod density, SurfacePod dest)
{
    GLuint pid = Programs.ApplyBuoyancy;
    glUseProgram(pid);
    SetUniform(pid, "Velocity", 0);
    SetUniform(pid, "Temperature", 1);
    SetUniform(pid, "Density", 2);
    SetUniform(pid, "AmbientTemperature", AmbientTemperature);
    SetUniform(pid, "TimeStep", TimeStep);
    SetUniform(pid, "Sigma", SmokeBuoyancy);
    SetUniform(pid, "Kappa", SmokeWeight);

    glBindFramebuffer(GL_FRAMEBUFFER, dest.FboHandle);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, velocity.ColorTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_3D, temperature.ColorTexture);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_3D, density.ColorTexture);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, dest.Depth);
    ResetState();
}

void SwapSurfaces(SlabPod* slab)
{
    SurfacePod temp = slab->Ping;
    slab->Ping = slab->Pong;
    slab->Pong = temp;
}

void ClearSurface(SurfacePod s, float v)
{
    glBindFramebuffer(GL_FRAMEBUFFER, s.FboHandle);
    glClearColor(v, v, v, v);
    glClear(GL_COLOR_BUFFER_BIT);
}

void ResetState()
{
    glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_3D, 0);
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_3D, 0);
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_3D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_BLEND);
}

GLuint CreatePointVbo(float x, float y, float z)
{
    float p[3] = { x, y, z };
    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(p), &p[0], GL_STATIC_DRAW);
    return vbo;
}

GLuint getUniformLocation(GLuint program, const char* name)
{
    auto key = std::pair(program, name);
    auto cachedLoc = uniformCache[key];
    if (cachedLoc) return cachedLoc;

    auto loc = glGetUniformLocation(program, name);
    uniformCache[key] = loc;

    return loc;
}

void SetUniform(GLuint program, const char* name, int value)
{
    GLint location = getUniformLocation(program, name);
    glUniform1i(location, value);
}

void SetUniform(GLuint program, const char* name, float value)
{
    GLint location = getUniformLocation(program, name);
    glUniform1f(location, value);
}

void SetUniform(GLuint program, const char* name, std::vector<float> values)
{
    GLint location = getUniformLocation(program, name);
    glUniform1fv(location, values.size(), &values.front());
}

void SetUniform(GLuint program, const char* name, glm::mat4 value)
{
    GLint location = getUniformLocation(program, name);
    glUniformMatrix4fv(location, 1, false, glm::value_ptr(value));
}

void SetUniform(GLuint program, const char* name, glm::mat3 value)
{
    GLint location = getUniformLocation(program, name);
    glUniformMatrix3fv(location, 1, false, glm::value_ptr(value));
}

void SetUniform(GLuint program, const char* name, glm::vec3 value)
{
    GLint location = getUniformLocation(program, name);
    glUniform3f(location, value.x, value.y, value.z);
}

void SetUniform(GLuint program, const char* name, std::vector<glm::vec3> values)
{
    GLint location = getUniformLocation(program, name);
    glUniform3fv(location, values.size(), glm::value_ptr(values.front()));
}

void SetUniform(GLuint program, const char* name, float x, float y)
{
    GLint location = getUniformLocation(program, name);
    glUniform2f(location, x, y);
}

bool checkError()
{
    auto glErr = glGetError();

    if (glErr != GL_NO_ERROR)
        printf("OpenGL Error: %d\n", glErr);

    return glErr == GL_NO_ERROR;
}
