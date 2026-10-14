#pragma once

namespace Tutorial
{
bool Initialize(int width, int height);
void Activate();
void Mouse(int button, int state, int x, int y);
void Shutdown();
void Reset();
void Render();
void Resize(int width, int height);
void KeyDown(unsigned char key, int x, int y);
void KeyUp(unsigned char key, int x, int y);
void Visibility(int state);
void Tick(int value);
}
