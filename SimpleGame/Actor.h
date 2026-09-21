#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

using ActorId = std::uint64_t;

struct Vector3
{
    float x = 0;
    float y = 0;
    float z = 0;
};

struct ActorTransform
{
    Vector3 position;
    Vector3 rotation;
    Vector3 scale = {1, 1, 1};
};

struct Matrix4
{
    std::array<float, 16> values = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    static Matrix4 FromTransform(const ActorTransform &transform);
    Matrix4 operator*(const Matrix4 &other) const;
    Vector3 TransformPoint(Vector3 point) const;
};

struct ActorBounds
{
    Vector3 minimum;
    Vector3 maximum;
    bool valid = false;

    static ActorBounds Box(Vector3 minimum, Vector3 maximum);
    void Include(Vector3 point);
    void Include(const ActorBounds &other);
    ActorBounds Transformed(const Matrix4 &matrix) const;
};

enum class ScenePass
{
    World = 0,
    Shadow = 1,
    UI = 2
};

struct ActorAppearance
{
    int material = -1;
    int effect = 0;
    float emission = 0;
    float textureRepeat = 1;
    std::uint32_t meshKey = 0;
    bool additive = false;
};

// A placed object owns a local transform, bounds, visibility and draw action.
// Primitive parts (eyes, limbs, windows) belong to their containing Actor.
// Game controllers update Actors independently of render visibility.
class Actor
{
  public:
    ActorId Id() const;
    const std::wstring &Name() const;
    const ActorTransform &LocalTransform() const;
    const Matrix4 &WorldMatrix() const;
    const ActorBounds &WorldBounds() const;
    void SetTransform(const ActorTransform &transform);
    void SetBounds(const ActorBounds &bounds);
    void SetEnabled(bool enabled);
    void SetPasses(bool world, bool shadow, bool ui = false);
    void SetAppearance(const ActorAppearance &appearance);
    const ActorAppearance &Appearance() const;
    void SetDraw(std::function<void()> draw);

  private:
    friend class SceneGraph;
    Actor(ActorId id, std::wstring name);
    void Invalidate();
    void Update(const Matrix4 &parentWorld, bool parentChanged);

    ActorId id;
    std::wstring name;
    Actor *parent = nullptr;
    std::vector<std::unique_ptr<Actor>> children;
    ActorTransform local;
    Matrix4 world;
    ActorBounds localBounds;
    ActorBounds worldBounds;
    ActorBounds subtreeBounds;
    ActorAppearance appearance;
    std::function<void()> draw;
    unsigned passes = 3;
    unsigned subtreePasses = 0;
    bool enabled = true;
    bool localDirty = true;
    bool subtreeDirty = true;
};
