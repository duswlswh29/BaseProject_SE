#pragma once

#include <array>
#include <cstdint>
#include <random>
#include <vector>

namespace Rpg
{
struct Point
{
    float x = 0;
    float z = 0;
};

float Distance(Point a, Point b);

enum class Tile : unsigned char
{
    Ground,
    Tree,
    Rock,
    Water
};

class World
{
  public:
    static constexpr int Size = 40;
    static constexpr float CellSize = 1.5f;

    void Generate(std::uint32_t seed);
    Tile At(int x, int z) const;
    Point Center(int x, int z) const;
    int Index(Point position) const;
    bool Walkable(Point position, float radius = .27f) const;
    Point RandomGround(std::mt19937 &random, float distanceFromCamp = 5) const;
    std::vector<int> DistancesFrom(Point target) const;
    Point StepToward(Point from, const std::vector<int> &distances) const;
    bool Connected() const;
    std::uint32_t Seed() const;

  private:
    bool IsGround(int x, int z) const;
    std::array<Tile, Size * Size> tiles = {};
    std::uint32_t mapSeed = 0;
};

struct Stats
{
    static constexpr int MaximumLevel = 20;
    int level = 1;
    int experience = 0;
    int totalExperience = 0;
    int health = 100;
    int potions = 3;
    int crystals = 0;
    int coins = 0;
    int kills = 0;

    int MaxHealth() const;
    int Attack() const;
    int Defense() const;
    int RequiredExperience() const;
    int GainExperience(int amount);
    bool DrinkPotion();
};
}
