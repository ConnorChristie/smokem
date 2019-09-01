#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera
{
public:
    Camera(int width, int height);

    void update(GLFWwindow* window, long long deltaTime);

    glm::mat4 getView();
    glm::mat4 getProjection();

private:
    glm::mat4 projection;

    glm::vec3 cameraPos   = glm::vec3(3, 0, 2);
    glm::vec3 cameraFront = glm::vec3(1, 0, 0);
    glm::vec3 cameraUp    = glm::vec3(0, 1, 0);

    int mWidth, mHeight;

    float fov = 45;

    float yaw = 0;
    float pitch = 0;

    double lastX = -1;
    double lastY = -1;
};
