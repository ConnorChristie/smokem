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

typedef struct Config
{
    const char* Title;
    int Width;
    int Height;
} Config;

typedef struct Light
{
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    float brightness;
} Light;

class Smokem
{
public:
    void initialize(GLFWwindow* window);
    void update(GLFWwindow* window, long long dt);
    void updateGui();
    void render();

    Config getConfig() const { return config; };

private:
    Config config = {
        "Smokem",
        1920, 1080
    };
};
