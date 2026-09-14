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

int main(int argc, char **argv)
{
    const bool tutorial = argc > 1 && std::strcmp(argv[1], "--tutorial") == 0;
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitContextVersion(3, 3);
    glutInitContextProfile(GLUT_COMPATIBILITY_PROFILE);
    glutInitWindowSize(1280, 800);
    glutCreateWindow("Willowmere");
    SetWindowTextW(WindowFromDC(wglGetCurrentDC()),
                   tutorial ? L"윌로미어 - 작은 시작" : L"윌로미어 - 레벨 1 반딧불 사냥터");
    if (glewInit() != GLEW_OK)
    {
        MessageBoxW(
            nullptr, L"그래픽 기능을 초기화하지 못했습니다.", L"실행 오류", MB_OK | MB_ICONERROR);
        return 1;
    }
    if (!(tutorial ? Tutorial::Initialize(1280, 800) : LevelOne::Initialize(1280, 800)))
    {
        MessageBoxW(nullptr,
                    L"렌더러 초기화에 실패했습니다. OpenGL 3.3 호환 드라이버를 확인하세요.",
                    L"실행 오류",
                    MB_OK | MB_ICONERROR);
        if (tutorial)
        {
            Tutorial::Shutdown();
        }
        else
        {
            LevelOne::Shutdown();
        }
        return 1;
    }
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
    if (tutorial)
    {
        Tutorial::Reset();
    }
    glutDisplayFunc(tutorial ? Tutorial::Render : LevelOne::Render);
    glutReshapeFunc(tutorial ? Tutorial::Resize : LevelOne::Resize);
    glutKeyboardFunc(tutorial ? Tutorial::KeyDown : LevelOne::KeyDown);
    glutKeyboardUpFunc(tutorial ? Tutorial::KeyUp : LevelOne::KeyUp);
    glutVisibilityFunc(tutorial ? Tutorial::Visibility : LevelOne::Visibility);
    if (!tutorial)
    {
        glutMouseFunc(LevelOne::Mouse);
    }
    glutIgnoreKeyRepeat(1);
    glutCloseFunc(tutorial ? Tutorial::Shutdown : LevelOne::Shutdown);
    glutTimerFunc(16, tutorial ? Tutorial::Tick : LevelOne::Tick, 0);
    glutMainLoop();
    return 0;
}
