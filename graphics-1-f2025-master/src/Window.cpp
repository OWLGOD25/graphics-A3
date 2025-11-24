#define GLFW_INCLUDE_NONE
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Window.h"
#include <cassert>
struct App
{
	GLFWwindow* window = nullptr;
} gApp;

void CreateWindow(int width, int height, const char* title)
{
    /* Initialize the library */
    assert(glfwInit() == GLFW_TRUE);

    // Request OpenGL 4.3 (1.0 loaded by default)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    /* Create a windowed mode window and its OpenGL context */
    gApp.window = glfwCreateWindow(width, height, title, NULL, NULL);
    assert(gApp.window != nullptr);

    /* Make the window's context current */
    glfwMakeContextCurrent(gApp.window);

    // Load OpenGL extensions
    assert(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress));
}

bool WindowShouldClose()
{
    return glfwWindowShouldClose(gApp.window);
}

void Loop()
{
    /* Swap front and back buffers */
    glfwSwapBuffers(gApp.window);

    /* Poll for and process events */
    glfwPollEvents();
}

void DestroyWindow()
{
    glfwTerminate();
}
