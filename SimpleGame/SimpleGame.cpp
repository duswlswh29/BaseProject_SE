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

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitContextVersion(3,3);
    glutInitContextProfile(GLUT_COMPATIBILITY_PROFILE);
    glutInitWindowSize(1280,800);
    glutCreateWindow("Willowmere");
    SetWindowTextW(WindowFromDC(wglGetCurrentDC()),L"윌로미어 - 작은 시작");
    if(glewInit()!=GLEW_OK) {
        MessageBoxW(nullptr,L"그래픽 기능을 초기화하지 못했습니다.",L"실행 오류",MB_OK|MB_ICONERROR);
        return 1;
    }
    if(!Tutorial::Initialize(1280,800)) {
        MessageBoxW(nullptr,L"렌더러 초기화에 실패했습니다. OpenGL 3.3 호환 드라이버를 확인하세요.",L"실행 오류",MB_OK|MB_ICONERROR);
        Tutorial::Shutdown();
        return 1;
    }
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
    Tutorial::Reset();
    glutDisplayFunc(Tutorial::Render); glutReshapeFunc(Tutorial::Resize);
    glutKeyboardFunc(Tutorial::KeyDown); glutKeyboardUpFunc(Tutorial::KeyUp);
    glutVisibilityFunc(Tutorial::Visibility); glutIgnoreKeyRepeat(1);
    glutCloseFunc(Tutorial::Shutdown);
    glutTimerFunc(16,Tutorial::Tick,0);
    glutMainLoop(); return 0;
}

