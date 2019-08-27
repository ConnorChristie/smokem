#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <string>
#include <assert.h>
#include <initializer_list>
#include <memory>

#include "vmath.hpp"
#include "shader.h"

enum AttributeSlot {
    SlotPosition,
    SlotTexCoord,
};

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
void ApplyImpulse(SurfacePod dest, vmath::Vector3 position, float value);
void ApplyBuoyancy(SurfacePod velocity, SurfacePod temperature, SurfacePod density, SurfacePod dest);

void SetUniform(const char* name, int value);
void SetUniform(const char* name, float value);
void SetUniform(const char* name, float x, float y);
void SetUniform(const char* name, vmath::Matrix4 value);
void SetUniform(const char* name, vmath::Matrix3 value);
void SetUniform(const char* name, vmath::Vector3 value);
void SetUniform(const char* name, vmath::Point3 value);
void SetUniform(const char* name, vmath::Vector4 value);

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
extern const vmath::Vector3 ImpulsePosition;
