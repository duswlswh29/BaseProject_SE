#pragma once

namespace LevelOne
{
bool Initialize(int width, int height);
void Shutdown();
void Render();
void Resize(int width, int height);
void KeyDown(unsigned char key, int x, int y);
void KeyUp(unsigned char key, int x, int y);
void Mouse(int button, int state, int x, int y);
void Visibility(int state);
void Tick(int value);
}
