/*
Copyright 2022 Lee Taek Hee (Tech University of Korea)

This program is free software: you can redistribute it and/or modify
it under the terms of the What The Hell License. Do it plz.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY.
*/

#include "stdafx.h"
#include <iostream>
#include "Dependencies/GLEW/include/GL/glew.h"
#include "WindowPlatform.h"

#include "TutorialLevel.h"
#include "LevelOne.h"
#include <cstring>

namespace Platform
{
GLFWwindow *window = nullptr;
}

namespace
{
bool village = true;
bool closed = false;
bool ready = false;
int windowWidth = 1280, windowHeight = 800;

void RequestRedraw()
{
    if (!closed && Platform::HasWindow())
        Platform::RequestRedraw();
}

void Display()
{
    if (village)
        Tutorial::Render();
    else
        LevelOne::Render();
}

void Resize(int w, int h)
{
    windowWidth = w;
    windowHeight = h;
    if (village)
        Tutorial::Resize(w, h);
    else
        LevelOne::Resize(w, h);
}

void SwitchRegion(bool toVillage)
{
    LevelOne::SaveSharedProgress();
    village = toVillage;
    if (village)
        Tutorial::Activate();
    else
        LevelOne::Activate();
    Resize(windowWidth, windowHeight);
    SetWindowTextW(WindowFromDC(wglGetCurrentDC()),
                   village ? L"윌로미어 - 마을 [Tab: 사냥터]" : L"윌로미어 - 사냥터 [Tab: 마을]");
    RequestRedraw();
}

void KeyDown(unsigned char key, int x, int y)
{
    if (key == '\t')
    {
        SwitchRegion(!village);
        return;
    }
    if (village)
        Tutorial::KeyDown(key, x, y);
    else
        LevelOne::KeyDown(key, x, y);
}

void KeyUp(unsigned char key, int x, int y)
{
    if (village)
        Tutorial::KeyUp(key, x, y);
    else
        LevelOne::KeyUp(key, x, y);
}

void Mouse(int button, int state, int x, int y)
{
    if (village)
        Tutorial::Mouse(button, state, x, y);
    else
        LevelOne::Mouse(button, state, x, y);
}

void Visibility(int state)
{
    if (village)
        Tutorial::Visibility(state);
    else
        LevelOne::Visibility(state);
}

void Tick(int)
{
    if (closed || !Platform::HasWindow())
        return;
    if (village)
    {
        Tutorial::Tick(0);
        if (Tutorial::ConsumeLevelOneTransition())
            SwitchRegion(false);
    }
    else
        LevelOne::Tick(0);
}

void Shutdown()
{
    if (closed)
        return;
    closed = true;
    if (ready)
        LevelOne::SaveSharedProgress();
    Tutorial::Shutdown();
    LevelOne::Shutdown();
}
}

int main(int argc, char **argv)
{
    village = !(argc > 1 && std::strcmp(argv[1], "--level1") == 0);
    if (!glfwInit())
        return 1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
    Platform::window = glfwCreateWindow(windowWidth, windowHeight, "Willowmere", nullptr, nullptr);
    if (!Platform::window)
    {
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(Platform::window);
    glfwSwapInterval(0);
    if (glewInit() != GLEW_OK)
    {
        glfwDestroyWindow(Platform::window);
        Platform::window = nullptr;
        glfwTerminate();
        return 1;
    }
    // Load saved progression once. Both regions remain alive while travelling.
    if (!LevelOne::Initialize(windowWidth, windowHeight) ||
        !Tutorial::Initialize(windowWidth, windowHeight))
    {
        MessageBoxW(nullptr, L"지역 초기화에 실패했습니다.", L"실행 오류", MB_OK | MB_ICONERROR);
        Shutdown();
        glfwDestroyWindow(Platform::window);
        Platform::window = nullptr;
        glfwTerminate();
        return 1;
    }
    Tutorial::Reset();
    ready = true;
    SetWindowTextW(WindowFromDC(wglGetCurrentDC()),
                   village ? L"윌로미어 - 마을 [Tab: 사냥터]" : L"윌로미어 - 사냥터 [Tab: 마을]");
    glfwSetFramebufferSizeCallback(Platform::window,
                                   [](GLFWwindow *, int w, int h)
                                   {
                                       if (w > 0 && h > 0)
                                           Resize(w, h);
                                   });
    glfwSetKeyCallback(Platform::window,
                       [](GLFWwindow *, int key, int, int action, int)
                       {
                           if (action == GLFW_REPEAT)
                               return;
                           unsigned char c = 0;
                           if (key >= GLFW_KEY_A && key <= GLFW_KEY_Z)
                               c = static_cast<unsigned char>('a' + key - GLFW_KEY_A);
                           else if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9)
                               c = static_cast<unsigned char>('0' + key - GLFW_KEY_0);
                           else if (key == GLFW_KEY_TAB)
                               c = '\t';
                           else if (key == GLFW_KEY_ESCAPE)
                               c = 27;
                           else if (key == GLFW_KEY_SPACE)
                               c = ' ';
                           if (c)
                           {
                               if (action == GLFW_PRESS)
                                   KeyDown(c, 0, 0);
                               else
                                   KeyUp(c, 0, 0);
                           }
                       });
    glfwSetMouseButtonCallback(Platform::window,
                               [](GLFWwindow *w, int button, int action, int)
                               {
                                   double x, y;
                                   glfwGetCursorPos(w, &x, &y);
                                   int ww, wh, fw, fh;
                                   glfwGetWindowSize(w, &ww, &wh);
                                   glfwGetFramebufferSize(w, &fw, &fh);
                                   Mouse(button,
                                         action == GLFW_PRESS ? Platform::Down : 1,
                                         int(x * fw / (ww ? ww : 1)),
                                         int(y * fh / (wh ? wh : 1)));
                               });
    glfwSetWindowFocusCallback(Platform::window,
                               [](GLFWwindow *, int focus)
                               {
                                   Visibility(focus ? Platform::Visible : 0);
                               });
    glfwSetWindowIconifyCallback(Platform::window,
                                 [](GLFWwindow *, int iconified)
                                 {
                                     Visibility(iconified ? 0 : Platform::Visible);
                                 });
    glfwGetFramebufferSize(Platform::window, &windowWidth, &windowHeight);
    Resize(windowWidth, windowHeight);
    double nextTick = glfwGetTime();
    while (!glfwWindowShouldClose(Platform::window))
    {
        if (glfwGetWindowAttrib(Platform::window, GLFW_ICONIFIED))
        {
            glfwWaitEvents();
            continue;
        }
        glfwPollEvents();
        if (glfwWindowShouldClose(Platform::window))
            break;
        const double now = glfwGetTime();
        if (now >= nextTick)
        {
            Tick(0);
            nextTick = now + .016;
        }
        Display();
    }
    Shutdown(); // Release GPU resources while the context is still current.
    glfwDestroyWindow(Platform::window);
    Platform::window = nullptr;
    glfwTerminate();
    return 0;
}
