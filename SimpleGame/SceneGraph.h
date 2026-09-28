#pragma once

#include "Actor.h"
#include <unordered_map>
#include <unordered_set>
#include <tuple>

class Renderer;

class SceneGraph
{
  public:
    struct Statistics
    {
        unsigned visited = 0;
        unsigned rejectedSubtrees = 0;
        unsigned submitted = 0;
    };

    SceneGraph();
    Actor &Create(const std::wstring &name, ActorId parent = 0);
    Actor *Find(ActorId id);
    bool Remove(ActorId id);
    // Keeps the existing LOCAL transform; rejects cycles and invalid handles.
    bool Reparent(ActorId id, ActorId newParent);
    void Clear();
    void RetainChildren(ActorId parent, const std::unordered_set<ActorId> &keep);
    void UpdateTransforms();
    void Draw(Renderer &renderer, ScenePass pass, float seconds);
    bool WasVisible(ActorId id, ScenePass pass = ScenePass::World) const;
    const Statistics &LastStatistics(ScenePass pass) const;

  private:
    struct Frustum
    {
        float planes[6][4] = {};
        bool Intersects(const ActorBounds &bounds) const;
    };

    void EraseIndex(Actor &actor);
    void Gather(Actor &actor, const Frustum &frustum, ScenePass pass, std::vector<Actor *> &result);
    std::unique_ptr<Actor> root;
    std::unordered_map<ActorId, Actor *> index;
    ActorId nextId = 1;
    std::array<std::unordered_set<ActorId>, 3> visible;
    std::array<Statistics, 3> statistics;
    using SortKey = std::tuple<ActorId, int, int, std::uint32_t>;
    std::array<std::vector<SortKey>, 3> sortKeys;
    std::array<std::vector<Actor *>, 3> sortedOpaque;
};
