#include "stdafx.h"
#include "Profiler.h"
#include "RpgWorld.h"
#include <algorithm>
#include <cmath>
#include <queue>

namespace Rpg
{
namespace
{
std::uint32_t ForestHash(int x, int z, std::uint32_t seed)
{
    std::uint32_t h = std::uint32_t(x) * 0x8da6b343u ^ std::uint32_t(z) * 0xd8163841u ^ seed;
    h ^= h >> 16;
    h *= 0x7feb352du;
    h ^= h >> 15;
    h *= 0x846ca68bu;
    h ^= h >> 16;
    return h;
}

float ForestNoise(float x, float z, std::uint32_t seed)
{
    const int ix = int(std::floor(x)), iz = int(std::floor(z));
    float u = x - ix, v = z - iz;
    u = u * u * (3 - 2 * u);
    v = v * v * (3 - 2 * v);
    auto sample = [seed](int x, int z)
    {
        return (ForestHash(x, z, seed) & 65535u) / 65535.f;
    };
    const float a = sample(ix, iz) * (1 - u) + sample(ix + 1, iz) * u;
    const float b = sample(ix, iz + 1) * (1 - u) + sample(ix + 1, iz + 1) * u;
    return a * (1 - v) + b * v;
}
}

float Distance(Point a, Point b)
{
    return std::hypot(a.x - b.x, a.z - b.z);
}

Tile World::GlobalTile(int x, int z) const
{
    if (x >= 0 && z >= 0 && x < Size && z < Size)
        return tiles[z * Size + x];
    // All even rows/columns remain connected corridors. Obstacles cannot enclose ground.
    if ((x & 1) == 0 || (z & 1) == 0)
        return Tile::Ground;
    const std::uint32_t h = ForestHash(x, z, mapSeed);
    // Broad irregular groves and clearings, rather than uniform orchard density.
    const float grove = ForestNoise(x * .075f, z * .075f, mapSeed ^ 0xa531u);
    const unsigned treeChance = static_cast<unsigned>(18.f + 65.f * grove);
    return h % 100 < treeChance ? Tile::Tree : h % 100 < treeChance + 8 ? Tile::Rock : Tile::Ground;
}

Tile World::At(int x, int z) const
{
    return GlobalTile(x + originX, z + originZ);
}

bool World::Focus(Point position)
{
    const int x = int(std::floor(position.x / (CellSize * 8))) * 8;
    const int z = int(std::floor(position.z / (CellSize * 8))) * 8;
    if (x == originX && z == originZ)
        return false;
    originX = x;
    originZ = z;
    return true;
}

bool World::IsGround(int x, int z) const
{
    return x >= 0 && z >= 0 && x < Size && z < Size && At(x, z) == Tile::Ground;
}

Point World::Center(int x, int z) const
{
    return {(x + originX - Size / 2) * CellSize, (z + originZ - Size / 2) * CellSize};
}

Point World::PropPosition(int x, int z) const
{
    Point p = Center(x, z);
    const Tile tile = At(x, z);
    if (tile != Tile::Tree && tile != Tile::Rock)
        return p;
    // Keep trunks within their collision cell and retain translation-only GPU instancing.
    const auto hx = ForestHash(x + originX, z + originZ, mapSeed ^ 0x17b5u);
    const auto hz = ForestHash(x + originX, z + originZ, mapSeed ^ 0x9e37u);
    p.x += ((hx & 65535u) / 65535.f - .5f) * 1.1f;
    p.z += ((hz & 65535u) / 65535.f - .5f) * 1.1f;
    return p;
}

int World::Index(Point position) const
{
    const int x = static_cast<int>(std::floor(position.x / CellSize + Size / 2 + .5f)) - originX;
    const int z = static_cast<int>(std::floor(position.z / CellSize + Size / 2 + .5f)) - originZ;
    return x >= 0 && x < Size && z >= 0 && z < Size ? z * Size + x : -1;
}

bool World::Walkable(Point position, float radius) const
{
    Profiling::Scope profile("world.collision_query");
    const Point corners[] = {{position.x - radius, position.z - radius},
                             {position.x + radius, position.z - radius},
                             {position.x - radius, position.z + radius},
                             {position.x + radius, position.z + radius}};
    for (Point corner : corners)
    {
        const int x = int(std::floor(corner.x / CellSize + Size / 2 + .5f));
        const int z = int(std::floor(corner.z / CellSize + Size / 2 + .5f));
        if (GlobalTile(x, z) != Tile::Ground)
        {
            return false;
        }
    }
    return true;
}

std::vector<int> World::DistancesFrom(Point target) const
{
    Profiling::Scope profile("world.pathfinding");
    std::vector<int> distances(Size * Size, -1);
    const int start = Index(target);
    if (start < 0 || At(start % Size, start / Size) != Tile::Ground)
    {
        return distances;
    }
    std::queue<int> pending;
    distances[start] = 0;
    pending.push(start);
    const int dx[] = {-1, 1, 0, 0};
    const int dz[] = {0, 0, -1, 1};
    while (!pending.empty())
    {
        const int current = pending.front();
        pending.pop();
        for (int side = 0; side < 4; ++side)
        {
            const int x = current % Size + dx[side];
            const int z = current / Size + dz[side];
            if (!IsGround(x, z))
            {
                continue;
            }
            const int next = z * Size + x;
            if (distances[next] < 0)
            {
                distances[next] = distances[current] + 1;
                pending.push(next);
            }
        }
    }
    return distances;
}

bool World::Connected() const
{
    Profiling::Scope profile("world.connectivity");
    const auto distances = DistancesFrom({0, 0});
    for (int cell = 0; cell < Size * Size; ++cell)
    {
        if (At(cell % Size, cell / Size) == Tile::Ground && distances[cell] < 0)
        {
            return false;
        }
    }
    return true;
}

void World::Generate(std::uint32_t seed)
{
    Profiling::Scope profile("world.generate");
    originX = originZ = 0;
    mapSeed = seed;
    tiles.fill(Tile::Ground);
    std::mt19937 random(seed);
    std::uniform_int_distribution<int> coordinate(1, Size - 2);
    for (int attempt = 0; attempt < 280; ++attempt)
    {
        const int x = coordinate(random);
        const int z = coordinate(random);
        const bool lake = attempt < 7;
        const int extent = lake ? 2 : 0;
        const auto previous = tiles;
        for (int oz = -extent; oz <= extent; ++oz)
        {
            for (int ox = -extent; ox <= extent; ++ox)
            {
                const int tx = x + ox;
                const int tz = z + oz;
                if (tx <= 0 || tz <= 0 || tx >= Size - 1 || tz >= Size - 1 ||
                    (std::abs(tx - Size / 2) <= 4 && std::abs(tz - Size / 2) <= 4))
                {
                    continue;
                }
                if (lake && ox * ox + oz * oz > 5)
                {
                    continue;
                }
                tiles[tz * Size + tx] =
                    lake ? Tile::Water : (attempt % 3 == 0 ? Tile::Rock : Tile::Tree);
            }
        }
        // Reject an obstacle group if it would strand even one walkable tile.
        if (!Connected())
        {
            tiles = previous;
        }
    }
}

Point World::RandomGround(std::mt19937 &random, float distanceFromCamp) const
{
    std::vector<int> candidates;
    for (int cell = 0; cell < Size * Size; ++cell)
    {
        if (At(cell % Size, cell / Size) == Tile::Ground &&
            Distance(Center(cell % Size, cell / Size), {0, 0}) >= distanceFromCamp)
        {
            candidates.push_back(cell);
        }
    }
    if (candidates.empty())
    {
        return {0, 0};
    }
    std::uniform_int_distribution<std::size_t> choose(0, candidates.size() - 1);
    const int cell = candidates[choose(random)];
    return Center(cell % Size, cell / Size);
}

Point World::StepToward(Point from, const std::vector<int> &distances) const
{
    const int cell = Index(from);
    if (cell < 0 || distances.size() != Size * Size || distances[cell] <= 0)
    {
        return from;
    }
    int best = cell;
    const int dx[] = {-1, 1, 0, 0};
    const int dz[] = {0, 0, -1, 1};
    for (int side = 0; side < 4; ++side)
    {
        const int x = cell % Size + dx[side];
        const int z = cell / Size + dz[side];
        if (IsGround(x, z))
        {
            const int next = z * Size + x;
            if (distances[next] >= 0 && distances[next] < distances[best])
            {
                best = next;
            }
        }
    }
    return Center(best % Size, best / Size);
}

std::uint32_t World::Seed() const
{
    return mapSeed;
}

int Stats::MaxHealth() const
{
    return 100 + (level - 1) * 20;
}

int Stats::Attack() const
{
    return 18 + (level - 1) * 5;
}

int Stats::Defense() const
{
    return 2 + (level - 1) * 2;
}

int Stats::RequiredExperience() const
{
    return 40 + (level - 1) * 25;
}

int Stats::GainExperience(int amount)
{
    if (amount <= 0)
    {
        return 0;
    }
    totalExperience += amount;
    if (level == MaximumLevel)
    {
        return 0;
    }
    experience += amount;
    int gained = 0;
    while (level < MaximumLevel && experience >= RequiredExperience())
    {
        experience -= RequiredExperience();
        ++level;
        ++gained;
        health = (std::min)(health + 20, MaxHealth());
    }
    if (level == MaximumLevel)
    {
        experience = 0;
    }
    return gained;
}

bool Stats::DrinkPotion()
{
    if (potions <= 0 || health >= MaxHealth())
    {
        return false;
    }
    --potions;
    health = (std::min)(MaxHealth(), health + 50);
    return true;
}
}
