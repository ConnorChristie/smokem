#include "camera.h"

Camera::Camera(int width, int height, glm::vec3 initialPosition)
    : mWidth(width), mHeight(height), cameraPos(initialPosition)
{
}

void Camera::update(GLFWwindow* window, long long deltaTime)
{
    float cameraSpeed = 1.0f;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraUp;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraUp;

    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    int action = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);

    if (action == GLFW_PRESS)
    {
        if (lastX == -1 && lastY == -1)
        {
            lastX = xpos;
            lastY = ypos;
        }

        float xoffset = xpos - lastX;
        float yoffset = lastY - ypos; 
        lastX = xpos;
        lastY = ypos;

        float sensitivity = 0.05;
        xoffset *= sensitivity;
        yoffset *= sensitivity;

        yaw   += xoffset;
        pitch += yoffset;

        pitch = glm::clamp(pitch, -90.0f, 90.0f);

        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        cameraFront = glm::normalize(front);
    }
    else if (action == GLFW_RELEASE)
    {
        lastX = -1;
        lastY = -1;
    }

    viewMatrix = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    projectionMatrix = glm::infinitePerspective(glm::radians(fov), float(mWidth) / float(mHeight), 1.0f);
}

void Camera::setPosition(glm::vec3 position)
{
    cameraPos = position;
}
