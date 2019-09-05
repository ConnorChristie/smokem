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

constexpr auto Pi = (3.14159265f);

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
    void update(float dt);
    void updateGui();
    void render();
    void exit();

    void updateSmoke(float dt);
    void renderSmoke();

    Config getConfig() const { return config; };

private:
    Config config = {
        "Smokem",
        1920, 1080
    };

    GLFWwindow* window;
    Camera* camera;

    void initSmoke();
};

const float ImpulseTemperature = 10.0f;
const float ImpulseDensity = 1.25f;
const float TemperatureDissipation = 0.99f;
const float VelocityDissipation = 0.99f;
const float DensityDissipation = 0.999f;
