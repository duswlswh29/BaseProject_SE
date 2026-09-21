#include "stdafx.h"
#include "SceneGraph.h"
#include "Renderer.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <tuple>

SceneGraph::SceneGraph()
{
    Clear();
}

void SceneGraph::Clear()
{
    index.clear();
    root.reset(new Actor(0, L"장면 루트"));
    index.emplace(0, root.get());
    for (auto &set : visible)
        set.clear();
    statistics = {};
    // IDs are never recycled, so stale game handles cannot target new objects.
}

Actor &SceneGraph::Create(const std::wstring &name, ActorId parentId)
{
    Actor *parent = Find(parentId);
    if (!parent)
        throw std::invalid_argument("Invalid scene parent");
    std::unique_ptr<Actor> actor(new Actor(nextId++, name));
    actor->parent = parent;
    Actor &result = *actor;
    index.emplace(result.Id(), &result);
    parent->children.push_back(std::move(actor));
    parent->Invalidate();
    return result;
}

Actor *SceneGraph::Find(ActorId id)
{
    const auto found = index.find(id);
    return found == index.end() ? nullptr : found->second;
}

void SceneGraph::EraseIndex(Actor &actor)
{
    for (auto &child : actor.children)
        EraseIndex(*child);
    for (auto &set : visible)
        set.erase(actor.id);
    index.erase(actor.id);
}

bool SceneGraph::Remove(ActorId id)
{
    Actor *actor = Find(id);
    if (!actor || !actor->parent)
        return false;
    Actor *parent = actor->parent;
    EraseIndex(*actor);
    auto &children = parent->children;
    children.erase(std::remove_if(children.begin(),
                                  children.end(),
                                  [id](const std::unique_ptr<Actor> &node)
                                  {
                                      return node->Id() == id;
                                  }),
                   children.end());
    parent->Invalidate();
    return true;
}

bool SceneGraph::Reparent(ActorId id, ActorId parentId)
{
    Actor *actor = Find(id);
    Actor *target = Find(parentId);
    if (!actor || !actor->parent || !target)
        return false;
    for (Actor *ancestor = target; ancestor; ancestor = ancestor->parent)
        if (ancestor == actor)
            return false;
    if (actor->parent == target)
        return true;
    Actor *previous = actor->parent;
    auto &siblings = previous->children;
    auto found = std::find_if(siblings.begin(),
                              siblings.end(),
                              [id](const std::unique_ptr<Actor> &node)
                              {
                                  return node->Id() == id;
                              });
    std::unique_ptr<Actor> moved = std::move(*found);
    siblings.erase(found);
    moved->parent = target;
    moved->localDirty = true;
    target->children.push_back(std::move(moved));
    actor->Invalidate();
    previous->Invalidate();
    return true;
}

void SceneGraph::RetainChildren(ActorId parentId, const std::unordered_set<ActorId> &keep)
{
    Actor *parent = Find(parentId);
    if (!parent)
        return;
    std::vector<ActorId> obsolete;
    for (const auto &child : parent->children)
        if (!keep.count(child->Id()))
            obsolete.push_back(child->Id());
    for (ActorId id : obsolete)
        Remove(id);
}

void SceneGraph::UpdateTransforms()
{
    root->Update(Matrix4{}, false);
}

bool SceneGraph::Frustum::Intersects(const ActorBounds &bounds) const
{
    if (!bounds.valid)
        return false;
    for (const auto &plane : planes)
    {
        const Vector3 positive = {plane[0] >= 0 ? bounds.maximum.x : bounds.minimum.x,
                                  plane[1] >= 0 ? bounds.maximum.y : bounds.minimum.y,
                                  plane[2] >= 0 ? bounds.maximum.z : bounds.minimum.z};
        if (plane[0] * positive.x + plane[1] * positive.y + plane[2] * positive.z + plane[3] <
            -.001f)
            return false;
    }
    return true;
}

void SceneGraph::Gather(Actor &actor,
                        const Frustum &frustum,
                        ScenePass pass,
                        std::vector<Actor *> &result)
{
    auto &stat = statistics[static_cast<int>(pass)];
    ++stat.visited;
    const unsigned bit = 1u << static_cast<unsigned>(pass);
    if (!actor.enabled || !(actor.subtreePasses & bit))
        return;
    if (!frustum.Intersects(actor.subtreeBounds))
    {
        ++stat.rejectedSubtrees;
        return;
    }
    if (actor.draw && (actor.passes & bit) && frustum.Intersects(actor.worldBounds))
    {
        result.push_back(&actor);
        visible[static_cast<int>(pass)].insert(actor.id);
    }
    for (auto &child : actor.children)
        Gather(*child, frustum, pass, result);
}

void SceneGraph::Draw(Renderer &renderer, ScenePass pass, float seconds)
{
    UpdateTransforms();
    const int passIndex = static_cast<int>(pass);
    statistics[passIndex] = {};
    visible[passIndex].clear();
    Matrix4 projection, view;
    glGetFloatv(GL_PROJECTION_MATRIX, projection.values.data());
    glGetFloatv(GL_MODELVIEW_MATRIX, view.values.data());
    const Matrix4 clip = projection * view;
    Frustum frustum;
    for (int axis = 0; axis < 3; ++axis)
        for (int side = 0; side < 2; ++side)
            for (int component = 0; component < 4; ++component)
                frustum.planes[axis * 2 + side][component] =
                    clip.values[component * 4 + 3] +
                    (side == 0 ? 1.f : -1.f) * clip.values[component * 4 + axis];
    std::vector<Actor *> draws;
    Gather(*root, frustum, pass, draws);
    if (pass != ScenePass::UI)
    {
        std::stable_sort(draws.begin(),
                         draws.end(),
                         [&view](const Actor *a, const Actor *b)
                         {
                             const auto &x = a->appearance;
                             const auto &y = b->appearance;
                             if (x.additive != y.additive)
                                 return !x.additive;
                             if (x.additive)
                             {
                                 return view.TransformPoint(a->world.TransformPoint({})).z <
                                        view.TransformPoint(b->world.TransformPoint({})).z;
                             }
                             return std::tie(x.material, x.effect, x.meshKey) <
                                    std::tie(y.material, y.effect, y.meshKey);
                         });
    }
    for (Actor *actor : draws)
    {
        if (pass != ScenePass::UI)
        {
            const auto &look = actor->appearance;
            renderer.MaterialMode(look.material, look.emission, look.textureRepeat);
            renderer.EffectMode(look.effect, seconds);
            if (pass == ScenePass::World && look.additive)
            {
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                glDepthMask(GL_FALSE);
            }
            else
            {
                glDisable(GL_BLEND);
                glDepthMask(GL_TRUE);
            }
        }
        glPushMatrix();
        glMultMatrixf(actor->world.values.data());
        actor->draw();
        glPopMatrix();
        ++statistics[passIndex].submitted;
    }
    if (pass != ScenePass::UI)
    {
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        renderer.EffectMode();
        renderer.MaterialMode();
    }
}

bool SceneGraph::WasVisible(ActorId id, ScenePass pass) const
{
    return visible[static_cast<int>(pass)].count(id) != 0;
}

const SceneGraph::Statistics &SceneGraph::LastStatistics(ScenePass pass) const
{
    return statistics[static_cast<int>(pass)];
}
