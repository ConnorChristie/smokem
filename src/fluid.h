#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <cmath>
#include <cstdio>
#include <vector>
#include <string>
#include <chrono>
#include <iostream>

#include "camera.h"
#include "object.h"
#include "shader.h"
#include "utility.h"
#include "vmath.hpp"

constexpr auto Pi = (3.14159265f);

using namespace vmath;
using std::string;

typedef std::chrono::duration<double, std::ratio<1, 1000000>> us;

typedef struct PezConfigRec
{
    const char* Title;
    int Width;
    int Height;
    bool Multisampling;
    bool VerticalSync;
} PezConfig;

typedef struct Light
{
    glm::vec3 mPosition; // position of the light in the world space.
    glm::vec3 mAmbient; // ambient light
    glm::vec3 mDiffuse; // diffuse light
    glm::vec3 mSpecular; // specular light
    float mBrightness;
} Light;

PezConfig PezGetConfig();
void PezInitialize(GLFWwindow* window);
void PezUpdate(GLFWwindow* window, long long dt);
void PezRender();
