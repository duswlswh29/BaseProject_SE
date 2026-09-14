#pragma once

#include "Renderer.h"
#include <array>
#include <string>
#include <vector>

enum class Model
{
    Hero0,
    Hero1,
    Hero2,
    Hero3,
    Hero4,
    Hero5,
    Hero6,
    Hero7,
    Slime,
    Boar,
    Tree,
    Rock,
    Crystal,
    Potion,
    Orb,
    Ground,
    Water,
    Flame,
    Camp,
    Count
};

class ModelCache
{
  public:
    ModelCache() = default;
    ~ModelCache();
    ModelCache(const ModelCache &) = delete;
    ModelCache &operator=(const ModelCache &) = delete;
    bool Initialize();
    void Draw(Model model) const;
    const std::wstring &Status() const;

  private:
    struct Vertex
    {
        float position[3];
        float normal[3];
        float color[4];
        float uv[2];
    };

    struct Mesh
    {
        std::vector<Vertex> vertices;
        GLuint buffer = 0;
        GLsizei count = 0;
    };

    void Generate();
    bool Load(const std::wstring &path);
    bool Save(const std::wstring &path) const;
    void Box(Mesh &mesh,
             float x,
             float y,
             float z,
             float sx,
             float sy,
             float sz,
             float r,
             float g,
             float b);
    void Ellipsoid(Mesh &mesh,
                   float x,
                   float y,
                   float z,
                   float sx,
                   float sy,
                   float sz,
                   float r,
                   float g,
                   float b);
    void Grid(Mesh &mesh, int divisions, bool vertical);
    std::array<Mesh, static_cast<std::size_t>(Model::Count)> meshes;
    std::wstring status;
};
