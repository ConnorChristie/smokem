#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <map>
#include <vector>
#include <string>
#include <assert.h>
#include <initializer_list>
#include <memory>

#include "shader.h"

struct TexturePod {
    GLuint Handle;
    GLsizei Width;
    GLsizei Height;
};

struct SurfacePod {
    GLuint FboHandle;
    GLuint ColorTexture;
    GLsizei Width;
    GLsizei Height;
    GLsizei Depth;
};

struct SlabPod {
    SurfacePod Ping;
    SurfacePod Pong;
};

GLuint makeProgram(std::initializer_list<Shader> shaders);

GLuint CreatePointVbo(float x, float y, float z);
GLuint CreateQuadVbo();

void CreateObstacles(SurfacePod dest);
SlabPod CreateSlab(GLsizei width, GLsizei height, GLsizei depth, int numComponents);
SurfacePod CreateVolume(GLsizei width, GLsizei height, GLsizei depth, int numComponents);

void InitializeSlabPrograms();
void SwapSurfaces(SlabPod* slab);
void ClearSurface(SurfacePod s, float v);
void Advect(SurfacePod velocity, SurfacePod source, SurfacePod obstacles, SurfacePod dest, float dissipation);
void Jacobi(SurfacePod pressure, SurfacePod divergence, SurfacePod obstacles, SurfacePod dest);
void SubtractGradient(SurfacePod velocity, SurfacePod pressure, SurfacePod obstacles, SurfacePod dest);
void ComputeDivergence(SurfacePod velocity, SurfacePod obstacles, SurfacePod dest);
void ApplyImpulse(SurfacePod dest, glm::vec3 position, float value);
void ApplyBuoyancy(SurfacePod velocity, SurfacePod temperature, SurfacePod density, SurfacePod dest);

GLuint getUniformLocation(GLuint program, const char* name);
void SetUniform(GLuint program, const char* name, int value);
void SetUniform(GLuint program, const char* name, float value);
void SetUniform(GLuint program, const char* name, std::vector<float> values);
void SetUniform(GLuint program, const char* name, float x, float y);
void SetUniform(GLuint program, const char* name, glm::mat4 value);
void SetUniform(GLuint program, const char* name, glm::mat3 value);
void SetUniform(GLuint program, const char* name, glm::vec3 value);
void SetUniform(GLuint program, const char* name, glm::vec4 value);
void SetUniform(GLuint program, const char* name, std::vector<glm::vec3> value);

void ResetState();
bool checkError();

extern const float CellSize;
extern const int ViewportWidth;
extern const int ViewportHeight;
extern const int GridWidth;
extern const int GridHeight;
extern const int GridDepth;
extern const float SplatRadius;
extern const float AmbientTemperature;
extern const int NumJacobiIterations;
extern const float TimeStep;
extern const float SmokeBuoyancy;
extern const float SmokeWeight;
extern const float GradientScale;
extern const glm::vec3 ImpulsePosition;
