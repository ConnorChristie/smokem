#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera
{
public:
    Camera(int width, int height, glm::vec3 initialPosition);

    void update(GLFWwindow* window, long long deltaTime);

    glm::mat4 getViewMatrix() const { return viewMatrix; };
    glm::mat4 getProjectionMatrix() const { return projectionMatrix; };

    glm::vec3 getTranslation() const { return cameraPos; };
    void setTranslation(glm::vec3 position);

    float getFov() const { return fov; };
    float getYaw() const { return yaw; };
    float getPitch() const { return pitch; };

    void setFov(float value);
    void setYaw(float value);
    void setPitch(float value);

private:
    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;

    glm::vec3 cameraPos;
    glm::vec3 cameraFront = glm::vec3(1, 0, 0);
    glm::vec3 cameraUp    = glm::vec3(0, 1, 0);

    int mWidth, mHeight;

    float fov = 45;
    float yaw = 0;
    float pitch = 0;

    double lastX = -1;
    double lastY = -1;
};
