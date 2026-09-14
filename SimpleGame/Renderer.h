#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include "Dependencies/glew.h"
#include <map>
#include <string>

// Owns GPU targets, shadow filtering, procedural materials and Korean text.
// Requires an OpenGL 3.3 compatibility context; destroy before closing it.
class Renderer
{
  public:
    enum Material
    {
        Grass,
        Stone,
        Wood,
        Roof,
        MaterialCount
    };

    Renderer(int width, int height);
    ~Renderer();
    Renderer(const Renderer &) = delete;
    Renderer &operator=(const Renderer &) = delete;

    bool IsInitialized() const
    {
        return initialized;
    }

    const std::string &Error() const
    {
        return error;
    }

    void Resize(int width, int height);
    void BeginShadow(float cameraX, float cameraZ);
    void BeginScene(float cameraX, float cameraZ);
    void MaterialMode(int material = -1, float emission = 0, float textureRepeat = 1);
    // 0: ordinary mesh, 1: water, 2: flame. Geometry remains cached on the GPU.
    void EffectMode(int effect = 0, float seconds = 0);
    void EndScene();
    void BeginUI();
    void Text(float x,
              float baseline,
              const std::wstring &text,
              float r = .94f,
              float g = .94f,
              float b = .87f);
    // Retained for small tools that used the original renderer interface.
    void DrawSolidRect(float x, float y, float z, float size, float r, float g, float b, float a);

  private:
    struct Label
    {
        GLuint texture = 0;
        int width = 0, height = 0;
    };

    GLuint Compile(const char *vs, const char *fs);
    void MakeMaterials();
    void ReleaseTargets();
    GLuint sceneFbo = 0, sceneColor = 0, sceneDepth = 0, shadowFbo = 0, shadowDepth = 0;
    GLuint worldProgram = 0, postProgram = 0, materials[MaterialCount] = {};
    GLint materialLocation = -1, emissionLocation = -1;
    float lightProjection[16] = {}, lightView[16] = {};
    int width = 1, height = 1;
    bool initialized = false, shadowPass = false;
    std::string error;
    HDC textDC = nullptr;
    HFONT font = nullptr;
    HGDIOBJ previousFont = nullptr;
    std::map<std::wstring, Label> labels;
};
