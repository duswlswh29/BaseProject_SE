#include "stdafx.h"
#include "Actor.h"
#include <algorithm>
#include <cmath>
#include <utility>

namespace
{
bool Same(Vector3 a, Vector3 b)
{
    return a.x == b.x && a.y == b.y && a.z == b.z;
}
}

Matrix4 Matrix4::operator*(const Matrix4 &other) const
{
    Matrix4 result;
    result.values.fill(0);
    for (int column = 0; column < 4; ++column)
    {
        for (int row = 0; row < 4; ++row)
        {
            for (int k = 0; k < 4; ++k)
            {
                result.values[column * 4 + row] +=
                    values[k * 4 + row] * other.values[column * 4 + k];
            }
        }
    }
    return result;
}

Matrix4 Matrix4::FromTransform(const ActorTransform &t)
{
    const float radians = .01745329252f;
    Matrix4 translation, x, y, z, scale;
    translation.values[12] = t.position.x;
    translation.values[13] = t.position.y;
    translation.values[14] = t.position.z;
    x.values[5] = x.values[10] = std::cos(t.rotation.x * radians);
    x.values[6] = std::sin(t.rotation.x * radians);
    x.values[9] = -x.values[6];
    y.values[0] = y.values[10] = std::cos(t.rotation.y * radians);
    y.values[8] = std::sin(t.rotation.y * radians);
    y.values[2] = -y.values[8];
    z.values[0] = z.values[5] = std::cos(t.rotation.z * radians);
    z.values[1] = std::sin(t.rotation.z * radians);
    z.values[4] = -z.values[1];
    scale.values[0] = t.scale.x;
    scale.values[5] = t.scale.y;
    scale.values[10] = t.scale.z;
    return translation * z * y * x * scale;
}

Vector3 Matrix4::TransformPoint(Vector3 p) const
{
    return {values[0] * p.x + values[4] * p.y + values[8] * p.z + values[12],
            values[1] * p.x + values[5] * p.y + values[9] * p.z + values[13],
            values[2] * p.x + values[6] * p.y + values[10] * p.z + values[14]};
}

ActorBounds ActorBounds::Box(Vector3 low, Vector3 high)
{
    ActorBounds result;
    result.Include(low);
    result.Include(high);
    return result;
}

void ActorBounds::Include(Vector3 point)
{
    if (!valid)
    {
        minimum = maximum = point;
        valid = true;
        return;
    }
    minimum = {(std::min)(minimum.x, point.x),
               (std::min)(minimum.y, point.y),
               (std::min)(minimum.z, point.z)};
    maximum = {(std::max)(maximum.x, point.x),
               (std::max)(maximum.y, point.y),
               (std::max)(maximum.z, point.z)};
}

void ActorBounds::Include(const ActorBounds &other)
{
    if (other.valid)
    {
        Include(other.minimum);
        Include(other.maximum);
    }
}

ActorBounds ActorBounds::Transformed(const Matrix4 &matrix) const
{
    ActorBounds result;
    if (valid)
    {
        for (int corner = 0; corner < 8; ++corner)
        {
            result.Include(matrix.TransformPoint({corner & 1 ? maximum.x : minimum.x,
                                                  corner & 2 ? maximum.y : minimum.y,
                                                  corner & 4 ? maximum.z : minimum.z}));
        }
    }
    return result;
}

Actor::Actor(ActorId value, std::wstring label) : id(value), name(std::move(label))
{
}

ActorId Actor::Id() const
{
    return id;
}

const std::wstring &Actor::Name() const
{
    return name;
}

const ActorTransform &Actor::LocalTransform() const
{
    return local;
}

const Matrix4 &Actor::WorldMatrix() const
{
    return world;
}

const ActorBounds &Actor::WorldBounds() const
{
    return worldBounds;
}

const ActorAppearance &Actor::Appearance() const
{
    return appearance;
}

void Actor::Invalidate()
{
    for (Actor *node = this; node; node = node->parent)
    {
        node->subtreeDirty = true;
    }
}

void Actor::SetTransform(const ActorTransform &value)
{
    if (!Same(local.position, value.position) || !Same(local.rotation, value.rotation) ||
        !Same(local.scale, value.scale))
    {
        local = value;
        localDirty = true;
        Invalidate();
    }
}

void Actor::SetBounds(const ActorBounds &value)
{
    localBounds = value;
    localDirty = true;
    Invalidate();
}

void Actor::SetEnabled(bool value)
{
    if (enabled != value)
    {
        enabled = value;
        Invalidate();
    }
}

void Actor::SetPasses(bool worldPass, bool shadowPass, bool uiPass)
{
    passes = (worldPass ? 1u : 0u) | (shadowPass ? 2u : 0u) | (uiPass ? 4u : 0u);
    Invalidate();
}

void Actor::SetAppearance(const ActorAppearance &value)
{
    appearance = value;
}

void Actor::SetDraw(std::function<void()> value)
{
    draw = std::move(value);
    Invalidate();
}

void Actor::Update(const Matrix4 &parentWorld, bool parentChanged)
{
    if (!subtreeDirty && !parentChanged)
    {
        return;
    }
    const bool changed = localDirty || parentChanged;
    if (changed)
    {
        world = parentWorld * Matrix4::FromTransform(local);
        worldBounds = localBounds.Transformed(world);
    }
    subtreeBounds = {};
    subtreePasses = 0;
    if (enabled && draw)
    {
        subtreeBounds.Include(worldBounds);
        subtreePasses |= passes;
    }
    for (auto &child : children)
    {
        child->Update(world, changed);
        if (enabled)
        {
            subtreeBounds.Include(child->subtreeBounds);
            subtreePasses |= child->subtreePasses;
        }
    }
    localDirty = subtreeDirty = false;
}
