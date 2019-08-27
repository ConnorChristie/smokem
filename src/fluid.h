#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cmath>
#include <cstdio>
#include <vector>
#include <string>
#include <chrono>

#include "utility.h"
#include "shader.h"
#include "vmath.hpp"

using namespace vmath;
using std::string;

typedef std::chrono::duration<double, std::ratio<1, 1000000>> us;

static struct {
    SlabPod Velocity;
    SlabPod Density;
    SlabPod Pressure;
    SlabPod Temperature;
} Slabs;

static struct {
    SurfacePod Divergence;
    SurfacePod Obstacles;
    SurfacePod LightCache;
    SurfacePod BlurredDensity;
} Surfaces;

static struct {
    Matrix4 Projection;
    Matrix4 Modelview;
    Matrix4 View;
    Matrix4 ModelviewProjection;
} Matrices;

static struct {
    GLuint CubeCenter;
    GLuint FullscreenQuad;
} Vaos;

typedef struct PezConfigRec
{
    const char* Title;
    int Width;
    int Height;
    bool Multisampling;
    bool VerticalSync;
} PezConfig;

PezConfig PezGetConfig();
void PezInitialize();
void PezUpdate(long long dt);
void PezRender();

extern const float ImpulseTemperature;
extern const float ImpulseDensity;
extern const float TemperatureDissipation;
extern const float VelocityDissipation;
extern const float DensityDissipation;
