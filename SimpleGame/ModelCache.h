#pragma once

#include "Renderer.h"
#include "Actor.h"
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
    GrassTuft,
    HeroIdle,
    MerchantIdle,
    Merchant0,
    Merchant1,
    Merchant2,
    Merchant3,
    Merchant4,
    Merchant5,
    Merchant6,
    Merchant7,
    HealerIdle,
    Healer0,
    Healer1,
    Healer2,
    Healer3,
    Healer4,
    Healer5,
    Healer6,
    Healer7,
    DeerIdle,
    Deer0,
    Deer1,
    Deer2,
    Deer3,
    Deer4,
    Deer5,
    Deer6,
    Deer7,
    CatIdle,
    Cat0,
    Cat1,
    Cat2,
    Cat3,
    Cat4,
    Cat5,
    Cat6,
    Cat7,
    WolfIdle,
    Wolf0,
    Wolf1,
    Wolf2,
    Wolf3,
    Wolf4,
    Wolf5,
    Wolf6,
    Wolf7,
    House,
    Table,
    Stool,
    Chest,
    Barrel,
    Lantern,
    Broadleaf,
    WoodBlock,
    StoneBlock,
    LeafCluster,
    Garden,
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
    void DrawBatch(Model model, const std::vector<Vector3> &positions, ScenePass pass);
    const ActorBounds &Bounds(Model model) const;
    const std::wstring &Status() const;
    void DrawAnimated(Model idle, Model firstWalk, float phase, bool moving) const;
    void DrawTinted(Model model, float r, float g, float b) const;

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
        ActorBounds bounds;
    };

    void Generate();
    void DrawMesh(const Mesh &mesh, bool vertexColors = true) const;

    struct Batch
    {
        Mesh mesh;
        std::vector<Vector3> positions;
    };

    std::array<Batch, 4> batches;
    bool Load(const std::wstring &path);
    bool LoadAssetPack(const std::wstring &path);
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
