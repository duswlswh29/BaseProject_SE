#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include "Dependencies/GLEW/include/GL/glew.h"
#define GLFW_INCLUDE_NONE
#include "Dependencies/GLFW/glfw-3.4/include/GLFW/glfw3.h"

namespace Platform
{
extern GLFWwindow *window;

enum
{
    ElapsedTime,
    WindowWidth,
    WindowHeight,
    Visible = 1,
    LeftButton = 0,
    Down = 0
};

inline int Get(int value)
{
    if (value == ElapsedTime)
        return int(glfwGetTime() * 1000.0);
    int w = 0, h = 0;
    glfwGetFramebufferSize(window, &w, &h);
    return value == WindowWidth ? w : h;
}

inline int HasWindow()
{
    return window && !glfwWindowShouldClose(window);
}

inline void RequestRedraw()
{
} // The GLFW loop renders each active iteration.

inline void SwapBuffers()
{
    glfwSwapBuffers(window);
}
}
