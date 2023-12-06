#include <fmt/core.h>

#include <GLFW/glfw3.h>

int main(int /*argc*/, char** /*argv*/)
{    
    if (!glfwInit())
    {
        fmt::print("Failed to initialize GLFW\n");
        return 1;
    }

    GLFWwindow* const window = glfwCreateWindow(640, 480, "Hello Triangle", nullptr, nullptr);
    if(!window)
    {
        fmt::print("Failed to create window\n");
        return 1;
    }

    while(!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
