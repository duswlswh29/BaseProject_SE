/*
Copyright 2022 Lee Taek Hee (Tech University of Korea)

This program is free software: you can redistribute it and/or modify
it under the terms of the What The Hell License. Do it plz.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY.
*/

#include "stdafx.h"
#include <iostream>
#include "Dependencies\glew.h"
#include "Dependencies\freeglut.h"

#include "TutorialLevel.h"
#include "LevelOne.h"
#include <cstring>

namespace
{
bool village = true;
bool closed = false;
bool ready = false;
int windowWidth = 1280, windowHeight = 800;

void RequestRedraw()
{
    if (!closed && glutGetWindow())
        glutPostRedisplay();
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
    glutIdleFunc(state == GLUT_VISIBLE ? RequestRedraw : nullptr);
}

void Tick(int)
{
    if (closed || !glutGetWindow())
        return;
    if (village)
    {
        Tutorial::Tick(0);
        if (Tutorial::ConsumeLevelOneTransition())
            SwitchRegion(false);
    }
    else
        LevelOne::Tick(0);
    glutTimerFunc(16, Tick, 0);
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
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitContextVersion(3, 3);
    glutInitContextProfile(GLUT_COMPATIBILITY_PROFILE);
    glutInitWindowSize(windowWidth, windowHeight);
    glutCreateWindow("Willowmere");
    if (glewInit() != GLEW_OK)
        return 1;
    // Load saved progression once. Both regions remain alive while travelling.
    if (!LevelOne::Initialize(windowWidth, windowHeight) ||
        !Tutorial::Initialize(windowWidth, windowHeight))
    {
        MessageBoxW(nullptr, L"지역 초기화에 실패했습니다.", L"실행 오류", MB_OK | MB_ICONERROR);
        Shutdown();
        return 1;
    }
    Tutorial::Reset();
    ready = true;
    SetWindowTextW(WindowFromDC(wglGetCurrentDC()),
                   village ? L"윌로미어 - 마을 [Tab: 사냥터]" : L"윌로미어 - 사냥터 [Tab: 마을]");
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
    glutDisplayFunc(Display);
    glutReshapeFunc(Resize);
    glutKeyboardFunc(KeyDown);
    glutKeyboardUpFunc(KeyUp);
    glutMouseFunc(Mouse);
    glutVisibilityFunc(Visibility);
    glutIdleFunc(RequestRedraw);
    glutIgnoreKeyRepeat(1);
    glutCloseFunc(Shutdown);
    glutTimerFunc(16, Tick, 0);
    glutMainLoop();
    return 0;
}
