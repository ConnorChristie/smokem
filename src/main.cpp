#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "fluid.h"

std::chrono::time_point<std::chrono::steady_clock> GetMicroseconds()
{
    return std::chrono::high_resolution_clock::now();
}

int main(int argc, char *argv[])
{
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    if (!glfwInit())
    {
        return -1;
    }

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    GLFWwindow* window = glfwCreateWindow(PezGetConfig().Width, PezGetConfig().Height, PezGetConfig().Title, NULL, NULL);
    glfwMakeContextCurrent(window);
    //glfwSetCursorPosCallback(window, PezCursorCallback);

    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glfwSwapInterval(1);

    PezInitialize(window);

    auto previousTime = GetMicroseconds();

    while (!glfwWindowShouldClose(window))
    {
        auto currentTime = GetMicroseconds();
        auto deltaTime = currentTime - previousTime;
        previousTime = currentTime;

        PezUpdate(window, deltaTime.count());
        PezRender();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}