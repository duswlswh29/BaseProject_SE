#include "stdafx.h"
#include "Profiler.h"
#include "Monitoring.h"
#include <sstream>
#include <locale>
#include "TutorialLevel.h"
#include "LevelOne.h"
#include "Renderer.h"
#include "SceneGraph.h"
#include "ModelCache.h"
#include <utility>
#include "Dependencies/freeglut.h"
#include <memory>
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>
#include <random>

namespace Tutorial
{
struct P
{
    float x, z;
};

struct C
{
    float r, g, b;
};

struct NPC
{
    float x, z;
    const wchar_t *name;
    const wchar_t *line;
    C color;
};

static const NPC villagers[] = {
    {0, 0, L"미라 교수", L"숲의 빛씨앗을 찾아오렴. 과제는 먹으면 안 된단다.", {.5f, .35f, .7f}},
    {-2, 4, L"제빵사 핍", L"빵이 또 날아갔어. 오늘도 매출보다 빵이 잘 오르네.", {.8f, .45f, .3f}},
    {2, 6, L"학생 리오", L"WASD로 걸어 봐! 나는 첫날 마을을 움직이려 했어.", {.3f, .5f, .7f}},
    {-2, 8, L"정원사 페른", L"마을 북서쪽 숲에서 금빛 씨앗을 찾아보렴.", {.3f, .6f, .35f}},
    {3, -2, L"낚시꾼 노리", L"호수가 밤새 노래했어. 물고기들은 음치라던데.", {.3f, .65f, .6f}},
    {-5,
     -3,
     L"배달부 베아",
     L"씨앗을 구하면 호숫가 돌 옆에서 스페이스를 눌러 봐.",
     {.7f, .45f, .65f}},
    {-3,
     13,
     L"대장장이 로엔",
     L"마법 지팡이도 수리합니다. 빗자루 주차는 옆집이에요.",
     {.46f, .38f, .31f}},
    {3,
     15,
     L"약초상 세라",
     L"토끼를 따라가도 좋아. 다만 약초는 나눠 먹어야 해.",
     {.46f, .65f, .46f}},
    {-10, 6, L"목수 오린", L"숲 너머까지 길을 냈지. 호수에는 들어가지 말게.", {.61f, .45f, .31f}},
    {11,
     6,
     L"견습생 루나",
     L"등불을 켜는 마법부터 배우래. 용 소환은 다음 학기래!",
     {.65f, .48f, .78f}},
    {-10, 12, L"상인 마르", L"오늘의 특가! 전설의 용 비늘... 모양의 비스킷!", {.78f, .51f, .32f}},
    {8,
     15,
     L"순찰대원 도윤",
     L"사슴과 토끼가 살고 있어. 여우도 보이면 조용히 지켜봐.",
     {.32f, .46f, .61f}},
    {-8, 3, L"모범생 동료", L"공식 기록도 현장의 증거와 비교해 봐야 해.", {.25f, .45f, .8f}},
    {8, 3, L"사고뭉치 동료", L"등불 수리 중이야. 폭발은 아직 계획에 없어!", {.85f, .4f, .25f}},
    {-1,
     -3,
     L"마법생물학 교수",
     L"반응부터 기록하세요. 생물의 정체를 서둘러 단정하지 말고요.",
     {.6f, .4f, .8f}},
    {1, -5, L"학교장", L"학생들의 안전을 먼저 확인하겠습니다.", {.4f, .3f, .65f}},
    {4, 1, L"왕국 조사관", L"소문 대신 현장을 조사하고 증거를 모으겠습니다.", {.35f, .45f, .5f}},
    {-9,
     10,
     L"식료품 상인",
     L"빵 하나에 동전 두 개. E로 거래하면 기운이 돌아옵니다.",
     {.85f, .6f, .25f}},
    {9, 13, L"약초 상인", L"동전 두 개로 회복 약초를 드릴게요.", {.45f, .7f, .3f}},
    {-1, 11, L"여행 성직자", L"다친 분께는 대가 없이 치유를 전합니다.", {.9f, .9f, .7f}},
    {1, 14, L"호수 성직자", L"생명을 지키는 것이 제 소명이지요.", {.75f, .8f, .95f}},
    {-11, 16, L"친절한 농부", L"길 잃은 분을 마을로 안내하고 있어요.", {.5f, .7f, .35f}},
    {10, 17, L"선한 여행자", L"곁에 위험이 보이면 사람들에게 알려야죠.", {.35f, .75f, .65f}},
    {-4, 20, L"경비대원", L"마을 주변을 순찰하고 몬스터를 막겠습니다.", {.3f, .4f, .7f}},
    {4, 22, L"숲 경비대원", L"민가 가까이 온 몬스터부터 쫓아냅니다.", {.3f, .55f, .4f}},
    {-24, -18, L"산적 정찰병", L"여긴 우리 구역이다. 가까이 오지 마!", {.55f, .3f, .2f}},
    {25, -19, L"산적 두목", L"멀리 쫓아가지는 않는다. 이 숲이 내 구역이니까.", {.65f, .2f, .2f}},
    {-20, 24, L"심술궂은 주민", L"학생만 보면 따라가서 잔소리하고 싶군.", {.6f, .45f, .25f}},
    {20, 25, L"수상한 떠돌이", L"귀찮게 굴면 도망가겠어. 주머니는 건드리지 마!", {.4f, .35f, .4f}},
    {-7, 21, L"도서관 견습생", L"같은 기록도 관찰한 사실과 대조해야죠.", {.35f, .5f, .8f}},
    {7, 25, L"장치 수리 견습생", L"고장난 장치 앞에서는 일단 멈추고 살펴봅니다.", {.8f, .5f, .2f}},
    {0, 19, L"친절한 길잡이", L"마을 밖 사냥터에서는 F로 방어 마법을 쓰세요.", {.4f, .75f, .5f}}};
static const int npcCount = sizeof(villagers) / sizeof(villagers[0]);
static const P houses[] = {{-5, 3},   {5, 4},    {-5, 8},    {5, 9},     {-6, 15},   {6, 19},
                           {-13, 8},  {13, 11},  {-46, -22}, {-39, -34}, {-30, -30}, {-20, -36},
                           {-8, -40}, {3, -35},  {20, -31},  {33, -26},  {43, -20},  {45, -4},
                           {40, 7},   {42, 23},  {29, 39},   {18, 37},   {2, 39},    {-9, 31},
                           {-26, 32}, {-36, 18}, {-44, 12},  {-25, 0}};

struct Pond
{
    P center;
    float rx, rz;
};

static const Pond ponds[] = {
    {{-24, -43}, 5, 3.2f}, {{15, -40}, 5.8f, 3.8f}, {{47, 34}, 4.5f, 5}, {{-23, 20}, 5.5f, 3.6f}};
static std::vector<P> shrubs, rocks;

bool InPond(P p, float margin = 0)
{
    for (const auto &pond : ponds)
    {
        const float x = (p.x - pond.center.x) / (pond.rx + margin);
        const float z = (p.z - pond.center.z) / (pond.rz + margin);
        if (x * x + z * z < 1)
            return true;
    }
    return false;
}

static const P campfires[] = {{-3, 10}, {-12, -9}, {14, 5}, {-24, -18}, {25, -19}};

struct Animal
{
    P position, home;
    int kind;
    float phase, heading;
};

enum class Behavior
{
    Work,
    Study,
    Experiment,
    Investigate,
    Guard,
    Merchant,
    Priest,
    Helpful,
    Bandit,
    Bully
};

struct ResidentState
{
    P position, home;
    Behavior behavior = Behavior::Work;
    float heading = 0, cooldown = 0, stun = 0;
    ActorId actor = 0;
};

struct VillageMonster
{
    P position, home;
    int kind = 0, health = 3;
    float heading = 0, cooldown = 0, respawn = 0;
    ActorId actor = 0;
};

static std::vector<ResidentState> residents;
static std::vector<VillageMonster> monsters;
static std::unique_ptr<ModelCache> villageModels;
static Rpg::Stats &sharedStats = LevelOne::PlayerStats();
static int &testHealth = sharedStats.health, &testCoins = sharedStats.coins,
           &testExperience = sharedStats.experience;
static bool mouseAttack = false;
static float sharedSaveTime = 0;
static float attackWait = 0, hurtWait = 0;
void ResetPopulation();
void UpdatePopulation(float dt);
void AttackPopulation();
static std::vector<Animal> animals;
static std::unique_ptr<Renderer> renderer;
static SceneGraph scene;
static ActorId heroActor = 0, creatureActor = 0, seedActor = 0, targetActor = 0, spellActor = 0;
static std::vector<ActorId> animalActors;
static std::vector<ActorId> breadcrumbActors;
void BuildScene();
void SyncScene();
static bool moving = false;
static float facing = 0;
static const P seed = {-8, -6}, stone = {4, -4};
static P player = {0, 3}, camera = {0, 3};
static std::vector<P> trees;
static bool keys[256] = {}, paused = false;
// 0 meet, 1 collect, 2 cast, 3 befriend, 4 report, 5 complete.
static int stage = 0, w = 1280, h = 800, last = 0;
static bool levelOneTransitionPending = false;
static float elapsed = 0, time = 0, spell = 0, walk = 0;
static std::wstring dialog;
static const wchar_t *speaker = L"";

float Dist(P a, P b)
{
    float x = a.x - b.x, z = a.z - b.z;
    return std::sqrt(x * x + z * z);
}

void Color(C c)
{
    glColor3f(c.r, c.g, c.b);
}

void Box(float x, float y, float z, float a, float b, float c, C color)
{
    Color(color);
    glPushMatrix();
    glTranslatef(x, y, z);
    glScalef(a, b, c);
    Renderer::CountToolkitShape();
    glutSolidCube(1);
    glPopMatrix();
}

void Ball(float x, float y, float z, float r, C c)
{
    Color(c);
    glPushMatrix();
    glTranslatef(x, y, z);
    Renderer::CountToolkitShape();
    glutSolidSphere(r, 10, 8);
    glPopMatrix();
}

void Cone(float x, float y, float z, float r, float height, C c)
{
    Color(c);
    glPushMatrix();
    glTranslatef(x, y, z);
    glRotatef(-90, 1, 0, 0);
    Renderer::CountToolkitShape();
    glutSolidCone(r, height, 8, 1);
    glPopMatrix();
}

void Disk(float x, float y, float z, float rx, float rz, C c)
{
    Color(c);
    Renderer::CountedBegin(GL_TRIANGLE_FAN);
    glNormal3f(0, 1, 0);
    glVertex3f(x, y, z);
    for (int i = 0; i <= 48; ++i)
    {
        float a = 6.2831853f * i / 48;
        glVertex3f(x + rx * std::cos(a), y, z + rz * std::sin(a));
    }
    glEnd();
}

void Text(float x, float y, const std::wstring &value, C c = {.94f, .94f, .86f})
{
    renderer->Text(x, y, value, c.r, c.g, c.b);
}

void Panel(float x, float y, float width, float height)
{
    glColor4f(.075f, .13f, .18f, .93f);
    Renderer::CountedBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + width, y);
    glVertex2f(x + width, y + height);
    glVertex2f(x, y + height);
    glEnd();
}

void Say(const wchar_t *name, const wchar_t *line)
{
    speaker = name;
    dialog = line;
}

bool Blocked(P p);

struct Trail
{
    P a, b;
    float width;
};

static const Trail trails[] = {
    {{0, 3}, {0, -3}, 2.4f},        {{0, -3}, {-8, -3}, 2.2f},   {{-8, -3}, {-8, -6}, 2.2f},
    {{-8, -6}, {-17, -12}, 2.8f},   {{-17, -12}, {-24, -18}, 3}, {{-24, -18}, {-36, -27}, 3},
    {{-36, -27}, {-42, -12}, 2.5f}, {{-42, -12}, {-32, 5}, 3},   {{-32, 5}, {-12, 12}, 3},
    {{-12, 12}, {0, 12}, 2.5f},     {{0, 3}, {0, 22}, 3},        {{0, 12}, {12, 12}, 2.5f},
    {{0, 22}, {8, 29}, 3},          {{8, 29}, {24, 32}, 3},      {{24, 32}, {36, 16}, 3},
    {{36, 16}, {32, 1}, 3},         {{32, 1}, {25, -19}, 3},     {{25, -19}, {15, -12}, 2.8f},
    {{15, -12}, {4, -10}, 2.5f},    {{4, -10}, {4, -4}, 2.2f},   {{4, -4}, {0, -3}, 2.2f}};
static const P clearings[] = {{-24, -18}, {25, -19}, {-36, -27}, {-32, 5}, {24, 32}, {36, 16}};
static std::vector<P> trailTiles;

float SegmentDistance(P p, const Trail &trail)
{
    float dx = trail.b.x - trail.a.x, dz = trail.b.z - trail.a.z;
    float t = ((p.x - trail.a.x) * dx + (p.z - trail.a.z) * dz) / (dx * dx + dz * dz);
    t = (std::max)(0.f, (std::min)(1.f, t));
    return Dist(p, {trail.a.x + dx * t, trail.a.z + dz * t});
}

bool OnTrail(P p, float margin = 0)
{
    for (const auto &trail : trails)
        if (SegmentDistance(p, trail) < trail.width * .5f + margin)
            return true;
    for (std::size_t i = 8; i < sizeof(houses) / sizeof(houses[0]); ++i)
    {
        const P door = {houses[i].x, houses[i].z + 2.2f};
        P junction = trails[0].a;
        float best = 10000;
        for (const auto &trail : trails)
        {
            float dx = trail.b.x - trail.a.x, dz = trail.b.z - trail.a.z;
            float t = ((door.x - trail.a.x) * dx + (door.z - trail.a.z) * dz) / (dx * dx + dz * dz);
            t = (std::max)(0.f, (std::min)(1.f, t));
            P candidate = {trail.a.x + dx * t, trail.a.z + dz * t};
            float d = Dist(door, candidate);
            if (d < best)
            {
                best = d;
                junction = candidate;
            }
        }
        if (best > .01f && SegmentDistance(p, {door, junction, 2}) < 1 + margin)
            return true;
    }
    return false;
}

void BuildLandscape()
{
    Profiling::Scope profile("village.landscape_generate");
    trees.clear();
    trailTiles.clear();
    // Bake path coverage once; render and map share this layout.
    for (int x = -112; x < 112; ++x)
        for (int z = -104; z < 104; ++z)
        {
            P p = {x * .5f + .25f, z * .5f + .25f};
            bool open = !InPond(p, .6f);
            for (P house : houses)
                if (std::fabs(p.x - house.x) < 2.2f && std::fabs(p.z - house.z) < 2.f)
                    open = false;
            const float lx = (p.x - 9) / 5.f, lz = (p.z + 4) / 4.6f;
            if (lx * lx + lz * lz < 1)
                open = false;
            if (open && OnTrail(p))
                trailTiles.push_back({x * .5f, z * .5f});
        }
    std::mt19937 random(20261012);
    std::uniform_real_distribution<float> unit(0.f, 1.f);
    const P groves[] = {{-44, -35},
                        {-30, -39},
                        {-13, -27},
                        {7, -30},
                        {35, -34},
                        {45, -12},
                        {45, 12},
                        {36, 38},
                        {12, 43},
                        {-13, 37},
                        {-38, 28},
                        {-46, 3}};
    for (P grove : groves)
        for (int attempt = 0, placed = 0; attempt < 300 && placed < 24; ++attempt)
        {
            float angle = unit(random) * 6.2831853f;
            float radius = std::sqrt(unit(random)) * 10;
            P p = {grove.x + std::cos(angle) * radius, grove.z + std::sin(angle) * radius * .85f};
            if (std::fabs(p.x) > 54 || std::fabs(p.z) > 50 || OnTrail(p, 1.6f))
                continue;
            if (p.x > -17 && p.x < 18 && p.z > -10 && p.z < 28)
                continue;
            bool open = Dist(p, seed) > 4 && Dist(p, {9, -4}) > 8 && !InPond(p, 1);
            for (P clearing : clearings)
                if (Dist(p, clearing) < 6)
                    open = false;
            for (P house : houses)
                if (Dist(p, house) < 4)
                    open = false;
            for (P tree : trees)
                if (Dist(p, tree) < 2.1f)
                    open = false;
            if (open)
            {
                trees.push_back(p);
                ++placed;
            }
        }
    shrubs.clear();
    rocks.clear();
    // Irregular small props break up the open ground without blocking routes.
    for (int attempt = 0; attempt < 4000 && shrubs.size() < 520; ++attempt)
    {
        P p = {unit(random) * 108 - 54, unit(random) * 100 - 50};
        if (OnTrail(p, .7f) || InPond(p, 1) || Dist(p, {9, -4}) < 6)
            continue;
        bool open = Dist(p, seed) > 2 && Dist(p, stone) > 2;
        for (P house : houses)
            if (Dist(p, house) < 3.5f)
                open = false;
        for (P tree : trees)
            if (Dist(p, tree) < 1)
                open = false;
        for (P shrub : shrubs)
            if (Dist(p, shrub) < 1.8f)
                open = false;
        if (!open)
            continue;
        shrubs.push_back(p);
        if (shrubs.size() % 6 == 0)
            rocks.push_back({p.x + .6f, p.z - .4f});
    }
}

void Reset()
{
    player = {0, 3};
    camera = player;
    stage = 0;
    levelOneTransitionPending = false;
    elapsed = time = spell = walk = 0;
    paused = false;
    dialog.clear();
    std::fill(keys, keys + 256, false);
    BuildLandscape();
    animals.clear();
    for (int i = 0; i < 12; ++i)
    {
        P p = {float(-14 + (i % 4) * 7), float(-15 + (i / 4) * 2)};
        for (int attempt = 0; attempt < 8 && Blocked(p); ++attempt)
            p.x += .85f;
        if (!Blocked(p))
            animals.push_back({p, p, i % 3, float(i), 0});
    }
    moving = false;
    facing = 0;
    last = glutGet(GLUT_ELAPSED_TIME);
    ResetPopulation();
    BuildScene();
}

bool Blocked(P p)
{
    if (p.x < -56 || p.x > 56 || p.z < -52 || p.z > 52)
        return true;
    if (InPond(p, .35f))
        return true;
    float x = (p.x - 9) / 4.5f, z = (p.z + 4) / 4.1f;
    if (x * x + z * z < 1)
        return true;
    for (P b : houses)
        if (std::fabs(p.x - b.x) < 1.9f && std::fabs(p.z - b.z) < 1.7f)
            return true;
    for (P t : trees)
        if (Dist(p, t) < .65f)
            return true;
    for (P f : campfires)
        if (Dist(p, f) < .7f)
            return true;
    return false;
}

P FindFree(P desired)
{
    if (!Blocked(desired))
        return desired;
    for (int radius = 1; radius <= 12; ++radius)
        for (int i = 0; i < 16; ++i)
        {
            const float angle = i * 6.2831853f / 16;
            P candidate = {desired.x + std::cos(angle) * radius * .5f,
                           desired.z + std::sin(angle) * radius * .5f};
            if (!Blocked(candidate))
                return candidate;
        }
    return {0, 3};
}

bool SafeVillage(P p)
{
    return p.x > -16 && p.x < 16 && p.z > -8 && p.z < 27;
}

float BodyRadius(const P *position)
{
    for (const auto &monster : monsters)
        if (&monster.position == position)
            return .5f;
    for (const auto &animal : animals)
        if (&animal.position == position)
            return .55f;
    return .4f;
}

bool CanStep(const P &from, P next)
{
    if (Blocked(next))
        return false;
    const float radius = BodyRadius(&from);
    auto touches = [&](const P &other, float otherRadius)
    {
        if (&from == &other)
            return false;
        const float limit = radius + otherRadius + .05f;
        const float oldDistance = Dist(from, other), newDistance = Dist(next, other);
        // Existing overlaps may move out, but can never move deeper into another body.
        return newDistance < limit && newDistance <= oldDistance + .00001f;
    };
    if (touches(player, .4f))
        return false;
    for (const auto &resident : residents)
        if (touches(resident.position, .4f))
            return false;
    for (const auto &monster : monsters)
        if (monster.health > 0 && touches(monster.position, .5f))
            return false;
    for (const auto &animal : animals)
        if (touches(animal.position, .55f))
            return false;
    return true;
}

void WalkToward(
    P &position, P target, float speed, float dt, float &heading, float stopDistance = .15f)
{
    const float distance = Dist(position, target);
    if (distance <= stopDistance)
        return;
    const float step = (std::min)(speed * dt, distance - stopDistance);
    const float dx = (target.x - position.x) / distance * step;
    const float dz = (target.z - position.z) / distance * step;
    if (CanStep(position, {position.x + dx, position.z}))
        position.x += dx;
    if (CanStep(position, {position.x, position.z + dz}))
        position.z += dz;
    heading = std::atan2(dx, dz) * 57.2957795f;
}

void ResetPopulation()
{
    residents.clear();
    monsters.clear();

    attackWait = hurtWait = 0;
    static const Behavior patterns[] = {
        Behavior::Investigate, Behavior::Merchant,   Behavior::Study,       Behavior::Work,
        Behavior::Work,        Behavior::Helpful,    Behavior::Work,        Behavior::Merchant,
        Behavior::Work,        Behavior::Experiment, Behavior::Merchant,    Behavior::Guard,
        Behavior::Study,       Behavior::Experiment, Behavior::Investigate, Behavior::Guard,
        Behavior::Investigate, Behavior::Merchant,   Behavior::Merchant,    Behavior::Priest,
        Behavior::Priest,      Behavior::Helpful,    Behavior::Helpful,     Behavior::Guard,
        Behavior::Guard,       Behavior::Bandit,     Behavior::Bandit,      Behavior::Bully,
        Behavior::Bully,       Behavior::Study,      Behavior::Experiment,  Behavior::Helpful};
    static_assert(sizeof(patterns) / sizeof(patterns[0]) == npcCount, "NPC pattern count");
    for (int i = 0; i < npcCount; ++i)
    {
        ResidentState state;
        state.home = state.position = FindFree({villagers[i].x, villagers[i].z});
        state.behavior = patterns[i];
        residents.push_back(state);
    }
    // Stable spawn layout; outer forest has ample encounters without invading quest space.
    for (int attempt = 0; attempt < 1024 && monsters.size() < 64; ++attempt)
    {
        const float angle = attempt * 2.399963f;
        const float radius = 19.f + (attempt % 5) * 7.f;
        P p = FindFree({std::cos(angle) * radius, std::sin(angle) * radius});
        if (SafeVillage(p) || Dist(p, seed) < 6 || Dist(p, stone) < 6)
            continue;
        bool spaced = true;
        for (const auto &monster : monsters)
            if (Dist(p, monster.home) < 3)
                spaced = false;
        if (!spaced)
            continue;
        VillageMonster monster;
        monster.home = monster.position = p;
        monster.kind = int(monsters.size()) % 2;
        monsters.push_back(monster);
    }
}

void AttackPopulation()
{
    if (attackWait > 0)
        return;
    attackWait = .45f;
    spell = 1;
    for (auto &monster : monsters)
        if (monster.health > 0 && Dist(player, monster.position) < 3)
        {
            monster.health -= (std::max)(1, sharedStats.Attack() / 10);
            if (monster.health <= 0)
            {
                monster.respawn = 12;
                sharedStats.GainExperience(5);
                ++sharedStats.kills;
                testCoins += 1;
            }
        }
    for (auto &resident : residents)
        if ((resident.behavior == Behavior::Bandit || resident.behavior == Behavior::Bully) &&
            Dist(player, resident.position) < 3)
            resident.stun = 5;
}

void UpdatePopulation(float dt)
{
    Profiling::Scope profile("village.population_update");
    attackWait = (std::max)(0.f, attackWait - dt);
    hurtWait = (std::max)(0.f, hurtWait - dt);
    auto hurt = [&]
    {
        if (hurtWait <= 0)
        {
            testHealth -= (std::max)(1, 6 - sharedStats.Defense());
            hurtWait = 1;
        }
    };
    {
        Profiling::Scope monsterProfile("village.monster_ai");
        for (auto &monster : monsters)
        {
            if (monster.health <= 0)
            {
                monster.respawn -= dt;
                if (monster.respawn <= 0 && Dist(player, monster.home) > 5)
                {
                    monster.position = monster.home;
                    monster.health = 3;
                }
                continue;
            }
            P target = {monster.home.x + std::sin(time * .3f + monster.home.x) * 2,
                        monster.home.z + std::cos(time * .3f + monster.home.z) * 2};
            const bool chase = !SafeVillage(player) && Dist(player, monster.position) < 6 &&
                               Dist(player, monster.home) < 9;
            if (chase)
                target = player;
            WalkToward(monster.position,
                       target,
                       chase ? 2.2f : .7f,
                       dt,
                       monster.heading,
                       chase ? 1.05f : .15f);
            if (chase && Dist(player, monster.position) < 1.1f)
                hurt();
        }
    }
    {
        Profiling::Scope residentProfile("village.resident_ai");
        for (std::size_t i = 0; i < residents.size(); ++i)
        {
            auto &resident = residents[i];
            resident.cooldown = (std::max)(0.f, resident.cooldown - dt);
            resident.stun = (std::max)(0.f, resident.stun - dt);
            if (i == 0)
                continue; // Original quest giver stays reachable.
            P target = resident.home;
            const float phase = time * .22f + float(i);
            if (resident.behavior == Behavior::Merchant)
                continue;
            if (resident.behavior == Behavior::Study)
                target = {resident.home.x + std::sin(phase) * 1.5f, resident.home.z};
            else if (resident.behavior == Behavior::Experiment)
                target = {resident.home.x + std::sin(phase) * 2,
                          resident.home.z + std::cos(phase) * 2};
            else if (resident.behavior == Behavior::Investigate)
                target = {resident.home.x + std::sin(phase) * 3,
                          resident.home.z + std::cos(phase) * 1.5f};
            else
                target = {resident.home.x + std::sin(phase) * 2,
                          resident.home.z + std::cos(phase) * 2};

            if (resident.behavior == Behavior::Guard)
            {
                for (auto &monster : monsters)
                    if (monster.health > 0 && Dist(monster.position, resident.home) < 8)
                    {
                        target = monster.position;
                        if (Dist(resident.position, target) < 1.8f && resident.cooldown <= 0)
                        {
                            --monster.health;
                            resident.cooldown = 1;
                            if (monster.health <= 0)
                                monster.respawn = 12;
                        }
                        break;
                    }
            }
            if (resident.behavior == Behavior::Priest && Dist(player, resident.position) < 3 &&
                testHealth < sharedStats.MaxHealth() && resident.cooldown <= 0)
            {
                testHealth = (std::min)(sharedStats.MaxHealth(), testHealth + 10);
                resident.cooldown = 3;
            }
            if (resident.behavior == Behavior::Helpful && Dist(player, resident.position) < 5 &&
                testHealth < 60)
                target = {player.x + 1, player.z + 1};
            if ((resident.behavior == Behavior::Bandit || resident.behavior == Behavior::Bully) &&
                Dist(player, resident.home) < 6 && Dist(player, resident.position) < 5)
            {
                target = player;
                if (resident.behavior == Behavior::Bandit && !SafeVillage(player) &&
                    resident.stun <= 0 && Dist(player, resident.position) < 1.2f)
                    hurt();
                if (resident.behavior == Behavior::Bully && Dist(player, resident.position) < 1.8f)
                    target = resident.home;
            }
            if (resident.stun > 0)
                target = resident.home;
            if (resident.cooldown <= 0 || resident.stun > 0)
                WalkToward(resident.position,
                           target,
                           1.2f,
                           dt,
                           resident.heading,
                           Dist(target, player) < .1f ? 1.1f : .15f);
        }
    }
    if (testHealth <= 0)
    {
        testHealth = sharedStats.MaxHealth();
        player = {0, 3};
        camera = player;
        hurtWait = 3;
        Say(L"마을 구조대", L"안전한 마을로 돌아왔습니다. 경험치와 동전은 잃지 않았어요.");
    }
    Profiling::Count("village.residents", double(residents.size()));
    Profiling::Count("village.monsters_total", double(monsters.size()));
    unsigned alive = 0;
    for (const auto &monster : monsters)
        if (monster.health > 0)
            ++alive;
    Profiling::Count("village.monsters_alive", alive);
}

P Target()
{
    if (stage == 1)
        return seed;
    if (stage == 2 || stage == 3)
        return stone;
    return {0, 0};
}

const wchar_t *Objective()
{
    static const wchar_t *lines[] = {L"01 / 마을의 미라 교수와 대화하기 [E]",
                                     L"02 / 숲의 빛씨앗 채집하기 [E]",
                                     L"03 / 호숫가 돌 옆에서 빛 마법 사용하기 [스페이스]",
                                     L"04 / 깨어난 작은 생물과 교감하기 [E]",
                                     L"05 / 미라 교수에게 돌아가 보고하기 [E]",
                                     L"완료 / 새로운 친구, 새로운 수수께끼"};
    return lines[stage];
}

void Interact()
{
    if (!dialog.empty())
    {
        dialog.clear();
        return;
    }
    if (stage == 1 && Dist(player, seed) < 1.8f)
    {
        stage = 2;
        Say(L"빛씨앗", L"햇살처럼 따뜻하다! 호숫가 돌 옆에서 스페이스로 빛 마법을 사용하자.");
        return;
    }
    if (stage == 3 && Dist(player, stone) < 2)
    {
        stage = 4;
        Say(L"작은 생물", L"뀨! 빛씨앗을 꿀꺽 삼킨 생물이 당신의 뒤를 졸졸 따라온다.");
        return;
    }
    int nearest = -1;
    float best = 1.9f;
    for (int i = 0; i < npcCount; ++i)
    {
        float d = Dist(player, residents[i].position);
        if (d < best)
        {
            best = d;
            nearest = i;
        }
    }
    if (nearest < 0)
        return;
    const NPC &n = villagers[nearest];
    ResidentState &resident = residents[nearest];
    resident.cooldown = 3;
    if (resident.behavior == Behavior::Merchant)
    {
        if (testHealth >= sharedStats.MaxHealth())
            Say(n.name, L"이미 건강해 보이네요. 필요할 때 다시 오세요.");
        else if (testCoins < 2)
            Say(n.name, L"동전이 부족하네요. 숲의 몬스터를 물리쳐 보세요.");
        else
        {
            testCoins -= 2;
            testHealth = (std::min)(sharedStats.MaxHealth(), testHealth + 30);
            Say(n.name, L"회복 물품 거래 완료! 체력 30 회복, 동전 2개 사용.");
        }
        return;
    }
    if (resident.behavior == Behavior::Priest)
    {
        testHealth = sharedStats.MaxHealth();
        Say(n.name, L"편히 쉬세요. 체력을 모두 회복했습니다.");
        return;
    }
    if (nearest == 0 && stage == 0)
    {
        stage = 1;
        Say(n.name, n.line);
    }
    else if (nearest == 0 && stage == 4)
    {
        stage = 5;
        levelOneTransitionPending = true;
        Say(n.name, L"새로운 반응을 관찰했구나. 정체는 아직 알 수 없으니 차근차근 조사해 보자.");
    }
    else
        Say(n.name, n.line);
}

void KeyDown(unsigned char k, int, int)
{
    if (k >= 'A' && k <= 'Z')
        k = static_cast<unsigned char>(k - 'A' + 'a');
    if (k == 27)
    {
        paused = !paused;
        std::fill(keys, keys + 256, false);
        return;
    }
    if (k == 'r')
    {
        Reset();
        return;
    }
    if (paused || keys[k])
        return;
    keys[k] = true;
    if (k == 'q' && dialog.empty())
        sharedStats.DrinkPotion();
    if (k == 'f' && dialog.empty())
        AttackPopulation();
    if (k == 'e')
        Interact();
    if (k == ' ' && dialog.empty())
    {
        spell = 1;
        if (stage == 2 && Dist(player, stone) < 2)
        {
            stage = 3;
            Say(L"호숫가 돌", L"빛에 응답한 문양이 깨어났다. 돌 아래에서 작은 울음소리가 들린다…");
        }
    }
}

bool ConsumeLevelOneTransition()
{
    if (!levelOneTransitionPending || !dialog.empty() || paused)
        return false;

    levelOneTransitionPending = false;
    return true;
}

void KeyUp(unsigned char k, int, int)
{
    if (k >= 'A' && k <= 'Z')
        k = static_cast<unsigned char>(k - 'A' + 'a');
    keys[k] = false;
}

void Visibility(int state)
{
    if (state != GLUT_VISIBLE)
    {
        paused = true;
        std::fill(keys, keys + 256, false);
    }
}

void Activate()
{
    paused = false;
    mouseAttack = false;
    std::fill(keys, keys + 256, false);
    last = glutGet(GLUT_ELAPSED_TIME);
}

void Mouse(int button, int state, int, int)
{
    if (button == GLUT_LEFT_BUTTON)
        mouseAttack = state == GLUT_DOWN && !paused && dialog.empty();
}

void Tick(int)
{
    Profiling::Scope profile("game.update");
    int now = glutGet(GLUT_ELAPSED_TIME);
    float dt = (std::min)((now - last) / 1000.f, .05f);
    last = now;
    if (!paused)
    {
        time += dt;
        sharedSaveTime += dt;
        if (sharedSaveTime >= 15)
        {
            sharedSaveTime = 0;
            LevelOne::SaveSharedProgress();
        }
        if (dialog.empty() && (mouseAttack || keys['f'] || keys[' ']) &&
            !(stage == 2 && Dist(player, stone) < 2))
            AttackPopulation();
        spell = (std::max)(0.f, spell - dt);
        if (stage < 5)
            elapsed += dt;
        moving = false;
        if (dialog.empty())
        {
            float sx = float(keys['d']) - float(keys['a']),
                  sy = float(keys['w']) - float(keys['s']);
            float len = std::sqrt(sx * sx + sy * sy);
            if (len > 0)
            {
                // Inverse of the renderer's +45-degree Y rotation on the ground plane.
                // Screen right = (+X,+Z), screen up = (+X,-Z).
                // Facing is an output of movement, never an input to its direction.
                const float step = 3.2f * dt / len;
                const float dx = (sx + sy) * .70710678f * step;
                const float dz = (sx - sy) * .70710678f * step;
                moving = true;
                facing = std::atan2(dx, dz) * 180.f / 3.14159265f;
                P next = {player.x + dx, player.z};
                if (CanStep(player, next))
                    player = next;
                next = {player.x, player.z + dz};
                if (CanStep(player, next))
                    player = next;
                walk += dt * 10;
            }
        }
        if (dialog.empty())
            UpdatePopulation(dt);
        for (Animal &animal : animals)
        {
            animal.phase += dt * 3;
            P desired = {animal.home.x + std::sin(time * .3f + animal.home.x) * 2,
                         animal.home.z + std::cos(time * .27f + animal.home.z) * 2};
            float ax = desired.x - animal.position.x, az = desired.z - animal.position.z;
            if (Dist(animal.position, player) < 3)
            {
                ax = animal.position.x - player.x;
                az = animal.position.z - player.z;
            }
            float length = std::sqrt(ax * ax + az * az);
            if (length > .05f)
            {
                float speed = animal.kind == 1 ? 1.5f : 1.f;
                P next = {animal.position.x + ax / length * speed * dt,
                          animal.position.z + az / length * speed * dt};
                if (CanStep(animal.position, next))
                {
                    animal.position = next;
                    animal.heading = std::atan2(ax, az) * 180.f / 3.14159265f;
                }
            }
        }
        float b = 1 - std::exp(-5 * dt);
        camera.x += (player.x - camera.x) * b;
        camera.z += (player.z - camera.z) * b;
    }
    glutPostRedisplay();
}

void Resize(int width, int height)
{
    w = (std::max)(width, 1);
    h = (std::max)(height, 1);
    if (renderer)
        renderer->Resize(w, h);
}

void Ground(float x, float z, float sx, float sz, int material)
{
    renderer->MaterialMode(material);
    Color({1, 1, 1});
    Renderer::CountedBegin(GL_QUADS);
    glNormal3f(0, 1, 0);
    float y = material == Renderer::Grass ? -.035f : .025f;
    glTexCoord2f(0, 0);
    glVertex3f(x - sx / 2, y, z - sz / 2);
    glTexCoord2f(0, sz / 2);
    glVertex3f(x - sx / 2, y, z + sz / 2);
    glTexCoord2f(sx / 2, sz / 2);
    glVertex3f(x + sx / 2, y, z + sz / 2);
    glTexCoord2f(sx / 2, 0);
    glVertex3f(x + sx / 2, y, z - sz / 2);
    glEnd();
    renderer->MaterialMode();
}

void Roof(P b)
{
    renderer->MaterialMode(Renderer::Roof);
    Color({1, 1, 1});
    Renderer::CountedBegin(GL_QUADS);
    glNormal3f(-.70f, .71f, 0);
    glTexCoord2f(0, 0);
    glVertex3f(b.x - 1.85f, 1.85f, b.z - 1.65f);
    glTexCoord2f(0, 2);
    glVertex3f(b.x - 1.85f, 1.85f, b.z + 1.65f);
    glTexCoord2f(2, 2);
    glVertex3f(b.x, 3.6f, b.z + 1.65f);
    glTexCoord2f(2, 0);
    glVertex3f(b.x, 3.6f, b.z - 1.65f);
    glNormal3f(.70f, .71f, 0);
    glTexCoord2f(0, 0);
    glVertex3f(b.x, 3.6f, b.z - 1.65f);
    glTexCoord2f(0, 2);
    glVertex3f(b.x, 3.6f, b.z + 1.65f);
    glTexCoord2f(2, 2);
    glVertex3f(b.x + 1.85f, 1.85f, b.z + 1.65f);
    glTexCoord2f(2, 0);
    glVertex3f(b.x + 1.85f, 1.85f, b.z - 1.65f);
    glEnd();
    renderer->MaterialMode();
    Color({.56f, .38f, .19f});
    // The roof reaches y=2.181 at the wall edge, above the original 1.8-high wall.
    // Fill that band on all four sides before closing the gables against the roof.
    Box(b.x, 1.98f, b.z, 3, .40f, 2.6f, {.48f, .39f, .25f});
    Renderer::CountedBegin(GL_TRIANGLES);
    glNormal3f(0, 0, 1);
    glVertex3f(b.x - 1.5f, 2.18f, b.z + 1.3f);
    glVertex3f(b.x + 1.5f, 2.18f, b.z + 1.3f);
    glVertex3f(b.x, 3.6f, b.z + 1.3f);
    glNormal3f(0, 0, -1);
    glVertex3f(b.x + 1.5f, 2.18f, b.z - 1.3f);
    glVertex3f(b.x - 1.5f, 2.18f, b.z - 1.3f);
    glVertex3f(b.x, 3.6f, b.z - 1.3f);
    glEnd();
}

void WoodenDoor(P b)
{
    renderer->MaterialMode(Renderer::Wood);
    Color({1, 1, 1});
    Renderer::CountedBegin(GL_QUADS);
    glNormal3f(0, 0, 1);
    glTexCoord2f(0, 0);
    glVertex3f(b.x - .32f, .03f, b.z + 1.40f);
    glTexCoord2f(1, 0);
    glVertex3f(b.x + .32f, .03f, b.z + 1.40f);
    glTexCoord2f(1, 1);
    glVertex3f(b.x + .32f, 1.17f, b.z + 1.40f);
    glTexCoord2f(0, 1);
    glVertex3f(b.x - .32f, 1.17f, b.z + 1.40f);
    glEnd();
    renderer->MaterialMode();
}

void Person(float x, float z, C coat, bool student = false)
{
    float cycle = student ? (moving ? walk : 0) : std::sin(time + x) * .15f;
    glPushMatrix();
    glTranslatef(x, 0, z);
    for (int side = -1; side <= 1; side += 2)
    {
        glPushMatrix();
        glTranslatef(side * .14f, .6f, 0);
        glRotatef(std::sin(cycle) * 24 * side, 1, 0, 0);
        Box(0, -.23f, 0, .18f, .46f, .20f, {.22f, .22f, .30f});
        Box(0, -.48f, .065f, .22f, .16f, .34f, {.23f, .15f, .11f});
        glPopMatrix();
        glPushMatrix();
        glTranslatef(side * .32f, 1.12f, 0);
        glRotatef(-std::sin(cycle) * 23 * side, 1, 0, 0);
        Box(0, -.17f, 0, .18f, .39f, .22f, coat);
        Ball(0, -.41f, 0, .105f, {.96f, .73f, .51f});
        glPopMatrix();
    }
    Box(0, .88f, 0, .52f, .61f, .34f, coat);
    Box(0, .68f, .02f, .56f, .085f, .38f, {.33f, .21f, .12f});
    Box(0, .7f, .22f, .09f, .10f, .04f, {.95f, .75f, .30f});
    Ball(0, 1.4f, 0, .255f, {.98f, .78f, .56f});
    Ball(0, 1.54f, -.055f, .23f, {.29f, .18f, .13f});
    Ball(-.09f, 1.43f, .22f, .035f, {.13f, .16f, .19f});
    Ball(.09f, 1.43f, .22f, .035f, {.13f, .16f, .19f});
    Ball(0, 1.35f, .24f, .048f, {.89f, .57f, .40f});
    if (student)
    {
        Cone(0, 1.61f, 0, .38f, .61f, {.34f, .27f, .57f});
        Box(0, 1.13f, -.22f, .50f, .62f, .07f, {.30f, .26f, .49f});
        Box(.44f, .77f, .08f, .045f, .82f, .045f, {.43f, .25f, .13f});
        renderer->MaterialMode(-1, .8f);
        Ball(.44f, 1.21f, .08f, .08f, {1, .80f, .30f});
        renderer->MaterialMode();
    }
    else
    {
        Cone(0, 1.59f, 0, .30f, .14f, coat);
        Box(.21f, .8f, -.18f, .18f, .22f, .16f, {.42f, .27f, .16f});
    }
    glPopMatrix();
}

void Creature(float x, float z)
{
    Ball(x, .42f, z, .25f, {.55f, .77f, .85f});
    Ball(x, .66f, z - .15f, .20f, {.69f, .87f, .91f});
    for (int side = -1; side <= 1; side += 2)
    {
        glPushMatrix();
        glTranslatef(x + side * .2f, .5f, z);
        glRotatef(side * (20 + std::sin(time * 6) * 18), 0, 0, 1);
        Cone(0, 0, 0, .17f, .4f, {.67f, .52f, .82f});
        glPopMatrix();
    }
    Ball(x - .08f, .7f, z - .32f, .035f, {.12f, .19f, .25f});
    Ball(x + .08f, .7f, z - .32f, .035f, {.12f, .19f, .25f});
}

void Wildlife(const Animal &a)
{
    glPushMatrix();
    glTranslatef(a.position.x, 0, a.position.z);
    glRotatef(a.heading, 0, 1, 0);
    float scale = a.kind == 0 ? 1.f : (a.kind == 1 ? .5f : .72f);
    glScalef(scale, scale, scale);
    C fur = a.kind == 0 ? C{.59f, .40f, .23f}
                        : (a.kind == 1 ? C{.82f, .79f, .67f} : C{.83f, .39f, .16f});
    glPushMatrix();
    glTranslatef(0, .62f, 0);
    glScalef(.7f, .8f, 1.35f);
    Ball(0, 0, 0, .52f, fur);
    glPopMatrix();
    for (int i = 0; i < 4; ++i)
    {
        float side = i % 2 ? 1.f : -1.f, front = i / 2 ? 1.f : -1.f;
        Box(side * .23f,
            .27f,
            front * .34f + std::sin(a.phase + float(i)) * .06f,
            .105f,
            .5f,
            .12f,
            fur);
    }
    Ball(0, .88f, .54f, .25f, fur);
    Ball(0, .81f, .77f, .13f, a.kind == 1 ? C{.93f, .87f, .79f} : fur);
    for (int side = -1; side <= 1; side += 2)
    {
        Cone(side * .14f, 1.04f, .5f, .08f, a.kind == 1 ? .48f : .22f, fur);
        Ball(side * .16f, .94f, .70f, .035f, {.10f, .12f, .12f});
        if (a.kind == 0)
        {
            Box(side * .19f, 1.32f, .42f, .05f, .5f, .05f, {.39f, .29f, .18f});
            Box(side * .26f, 1.46f, .42f, .17f, .05f, .05f, {.39f, .29f, .18f});
        }
    }
    if (a.kind == 2)
    {
        glPushMatrix();
        glTranslatef(0, .53f, -.5f);
        glRotatef(-45, 1, 0, 0);
        Cone(0, 0, 0, .18f, .7f, fur);
        glPopMatrix();
    }
    glPopMatrix();
}

void DrawHouse()
{
    const P b = {0, 0};
    Box(b.x, .9f, b.z, 3, 1.8f, 2.6f, {.48f, .49f, .40f});
    Roof(b);
    // Facade details stay inside the original house silhouette and collision footprint.
    Box(0, .16f, 0, 3.02f, .32f, 2.62f, {.32f, .35f, .29f});
    Box(0, 3.57f, 0, .19f, .16f, 3.32f, {.36f, .22f, .10f});
    for (int side = -1; side <= 1; side += 2)
    {
        Box(side * 1.48f, 1.02f, 0, .13f, 1.70f, 2.62f, {.28f, .17f, .08f});
        Box(side * 1.84f, 1.87f, 0, .15f, .17f, 3.32f, {.39f, .25f, .12f});
        for (int end = -1; end <= 1; end += 2)
        {
            glPushMatrix();
            glTranslatef(side * .925f, 2.725f, end * 1.65f);
            glRotatef(side * 46.6f, 0, 0, 1);
            Box(0, 0, 0, .16f, 2.55f, .17f, {.42f, .27f, .12f});
            glPopMatrix();
        }
    }
    Box(0, 2.47f, 1.32f, .14f, 1.18f, .12f, {.28f, .17f, .08f});
    for (int side = -1; side <= 1; side += 2)
    {
        Box(b.x + side * 1.43f, .93f, b.z + 1.34f, .13f, 1.86f, .12f, {.31f, .21f, .14f});
        Box(b.x + side * .87f, 1.1f, b.z + 1.34f, .61f, .68f, .08f, {.31f, .21f, .14f});
        renderer->MaterialMode(-1, .8f);
        Box(b.x + side * .87f, 1.1f, b.z + 1.39f, .46f, .53f, .025f, {1, .69f, .26f});
        renderer->MaterialMode();
        Box(b.x + side * .87f, 1.1f, b.z + 1.42f, .05f, .55f, .03f, {.33f, .22f, .14f});
    }
    Box(b.x, 1.76f, b.z + 1.36f, 3, .12f, .13f, {.31f, .21f, .14f});
    Box(b.x, .59f, b.z + 1.34f, .68f, 1.18f, .09f, {.39f, .25f, .13f});
    WoodenDoor(b);
    Ball(b.x + .22f, .62f, b.z + 1.42f, .045f, {.88f, .66f, .25f});
    Box(b.x, 0, b.z + 1.6f, 1.1f, .16f, .7f, {.53f, .53f, .45f});
    Box(b.x + 1, 2.6f, b.z - .5f, .42f, 1.5f, .45f, {.55f, .48f, .40f});
}

void DrawRoads()
{
    // One non-overlapping road mesh avoids flicker at the intersections.
    renderer->MaterialMode(Renderer::Stone);
    Color({1, 1, 1});
    Renderer::CountedBegin(GL_QUADS);
    glNormal3f(0, 1, 0);
    for (P tile : trailTiles)
    {
        const float x = tile.x, z = tile.z;
        glTexCoord2f(x * .5f, z * .5f);
        glVertex3f(x, .025f, z);
        glTexCoord2f(x * .5f, (z + .5f) * .5f);
        glVertex3f(x, .025f, z + .5f);
        glTexCoord2f((x + .5f) * .5f, (z + .5f) * .5f);
        glVertex3f(x + .5f, .025f, z + .5f);
        glTexCoord2f((x + .5f) * .5f, z * .5f);
        glVertex3f(x + .5f, .025f, z);
    }
    glEnd();
    renderer->MaterialMode();
}

void DrawWater()
{
    // Animated water surface with geometric ripples, changing normals and foam.
    renderer->MaterialMode(-1, .18f);
    const int rings = 18, segments = 64;
    auto waterVertex = [](float radius, float angle)
    {
        float x = 0 + std::cos(angle) * radius * 4.25f, z = 0 + std::sin(angle) * radius * 3.85f;
        float wave = std::sin(x * 2 + time * 1.4f) * std::cos(z * 2.3f + time) * .025f;
        glNormal3f(-.05f * std::cos(x * 2 + time * 1.4f), 1, -.05f * std::sin(z * 2.3f + time));
        Color({.08f + radius * .08f, .30f + radius * .10f + wave, .33f + radius * .10f});
        glVertex3f(x, .075f + wave, z);
    };
    for (int ring = 0; ring < rings; ++ring)
    {
        Renderer::CountedBegin(GL_TRIANGLE_STRIP);
        for (int j = 0; j <= segments; ++j)
        {
            float a = j * 6.2831853f / segments;
            waterVertex(float(ring) / rings, a);
            waterVertex(float(ring + 1) / rings, a);
        }
        glEnd();
    }
    renderer->MaterialMode(-1, .5f);
    for (int i = 0; i < 30; ++i)
    {
        float a = i * 2.4f + time * .07f, r = 1.2f + (i % 6) * .44f;
        Disk(0 + std::cos(a) * r, .115f, 0 + std::sin(a) * r, .11f, .025f, {.61f, .86f, .86f});
    }
}

void DrawFire()
{
    const P f = {0, 0};
    renderer->MaterialMode(-1, 2.1f);
    for (int i = 0; i < 9; ++i)
    {
        float life = std::fmod(time * (.65f + i * .015f) + i * .113f, 1.f);
        float x = f.x + std::sin(time * 4 + i) * .12f, z = f.z + std::cos(time * 3 + i) * .12f;
        Ball(x, .25f + life * .95f, z, (1 - life) * .16f + .015f, {1, .28f + life * .3f, .055f});
    }
    for (int i = 0; i < 7; ++i)
    {
        float life = std::fmod(time * .4f + i * .14f, 1.f);
        Ball(f.x + std::sin(i + time) * life * .45f,
             .6f + life * 1.8f,
             f.z,
             .025f * (1 - life),
             {1, .64f, .12f});
    }
}

void HUD();

ActorTransform Placement(
    P p, float y = 0, float heading = 0, float sx = 1, float sy = 1, float sz = 1)
{
    return {{p.x, y, p.z}, {0, heading, 0}, {sx, sy, sz}};
}

Actor &Place(const wchar_t *name,
             ActorId parent,
             const ActorTransform &transform,
             ActorBounds bounds,
             std::function<void()> draw,
             bool shadow = true,
             bool additive = false)
{
    Actor &actor = scene.Create(name, parent);
    actor.SetTransform(transform);
    actor.SetBounds(bounds);
    actor.SetDraw(std::move(draw));
    actor.SetPasses(true, shadow);
    ActorAppearance look;
    look.additive = additive;
    actor.SetAppearance(look);
    return actor;
}

void BuildScene()
{
    Profiling::Scope profile("scene.build");
    scene.Clear();
    animalActors.clear();
    breadcrumbActors.clear();
    const ActorId level = scene.Create(L"튜토리얼").Id();
    const ActorId terrain = scene.Create(L"지형", level).Id();
    const ActorId dynamic = scene.Create(L"주민과 생물", level).Id();
    std::map<std::pair<int, int>, ActorId> chunks;
    auto chunkFor = [&](P p)
    {
        const std::pair<int, int> key = {int(std::floor(p.x / 8)), int(std::floor(p.z / 8))};
        auto found = chunks.find(key);
        if (found != chunks.end())
            return found->second;
        Actor &chunk = scene.Create(L"정적 구역", terrain);
        // Local meshes are placed under named spatial groups for subtree culling.
        chunks.emplace(key, chunk.Id());
        return chunk.Id();
    };
    Place(
        L"지면",
        terrain,
        Placement({0, 0}),
        ActorBounds::Box({-90, -.05f, -86}, {90, 0, 86}),
        []
        {
            Ground(0, 0, 180, 172, Renderer::Grass);
        },
        false);
    Place(
        L"돌길",
        terrain,
        Placement({0, 0}),
        ActorBounds::Box({-56, 0, -52}, {56, .04f, 52}),
        []
        {
            DrawRoads();
        },
        false);

    for (const auto pond : ponds)
    {
        Place(
            L"외곽 연못 둑",
            chunkFor(pond.center),
            Placement(pond.center),
            ActorBounds::Box({-pond.rx - 1, 0, -pond.rz - 1}, {pond.rx + 1, .05f, pond.rz + 1}),
            [pond]
            {
                Disk(0, .02f, 0, pond.rx + 1, pond.rz + 1, {.43f, .49f, .32f});
            },
            false);
        Place(
            L"외곽 연못",
            chunkFor(pond.center),
            Placement(pond.center, 0, 0, pond.rx / 4.25f, 1, pond.rz / 3.85f),
            ActorBounds::Box({-4.3f, 0, -3.9f}, {4.3f, .2f, 3.9f}),
            []
            {
                DrawWater();
            },
            false);
    }
    for (P shrub : shrubs)
        Place(
            L"들풀 군락",
            chunkFor(shrub),
            Placement(shrub, 0, 0, .55f, .24f, .55f),
            villageModels->Bounds(Model::Tree),
            []
            {
                villageModels->Draw(Model::Tree);
            },
            false);
    for (P rock : rocks)
        Place(L"정원 바위",
              chunkFor(rock),
              Placement(rock, 0, rock.x * 17, .65f, .6f, .8f),
              villageModels->Bounds(Model::Rock),
              []
              {
                  villageModels->Draw(Model::Rock);
              });
    for (P clearing : clearings)
        Place(
            L"숲속 공터",
            chunkFor(clearing),
            Placement(clearing),
            ActorBounds::Box({-5, 0, -5}, {5, .04f, 5}),
            []
            {
                Disk(0, .018f, 0, 5, 5, {.49f, .47f, .29f});
            },
            false);
    Place(L"숲길 유적",
          chunkFor({-36, -27}),
          Placement({-36, -27}),
          ActorBounds::Box({-3, 0, -3}, {3, 3, 3}),
          []
          {
              Box(-2, 1.2f, 0, .7f, 2.4f, .7f, {.55f, .57f, .5f});
              Box(2, .9f, 0, .7f, 1.8f, .7f, {.55f, .57f, .5f});
              Box(0, .3f, 2, 3, .6f, .9f, {.48f, .51f, .44f});
          });
    for (std::size_t i = 8; i < sizeof(houses) / sizeof(houses[0]); ++i)
    {
        P house = houses[i];
        Place(
            L"텃밭",
            chunkFor(house),
            Placement({house.x + 3.5f, house.z}),
            ActorBounds::Box({-1, 0, -1.5f}, {1, .5f, 1.5f}),
            []
            {
                Disk(0, .02f, 0, 1, 1.5f, {.37f, .29f, .17f});
                for (int j = 0; j < 7; ++j)
                    Ball(
                        std::sin(j * 2.4f) * .7f, .2f, std::cos(j * 2.4f), .18f, {.38f, .6f, .25f});
            },
            false);
    }
    for (P b : houses)
        Place(L"주택",
              chunkFor(b),
              Placement(b),
              ActorBounds::Box({-1.9f, -.1f, -1.7f}, {1.9f, 3.65f, 2}),
              []
              {
                  DrawHouse();
              });
    for (P t : trees)
        Place(L"나무",
              chunkFor(t),
              Placement(t,
                        0,
                        std::sin(t.x * 1.7f + t.z) * 180,
                        .85f + .2f * std::sin(t.x),
                        1.f + .3f * std::sin(t.z * 2),
                        .85f + .2f * std::sin(t.x)),
              ActorBounds::Box({-1.2f, 0, -1.2f}, {1.2f, 4.1f, 1.2f}),
              []
              {
                  villageModels->Draw(Model::Tree);
              });
    for (int i = 0; i < 180; ++i)
    {
        const P p = {float((i * 17) % 550) / 10 - 27.5f, float((i * 31) % 510) / 10 - 25.5f};
        if (std::fabs(p.x) > 2 && !Blocked(p))
            Place(
                L"야생화",
                chunkFor(p),
                Placement(p),
                ActorBounds::Box({-.1f, 0, -.1f}, {.1f, .4f, .1f}),
                [i]
                {
                    Cone(0, .03f, 0, .09f, .3f, {.37f, .53f, .25f});
                    Ball(0, .3f, 0, .06f, i % 2 ? C{1, .84f, .48f} : C{.79f, .61f, .85f});
                },
                false);
    }
    for (int i = 0; i < npcCount; ++i)
    {
        const NPC &n = villagers[i];
        residents[i].actor = Place(n.name,
                                   dynamic,
                                   Placement({n.x, n.z}, 0, float(int(n.x * 9) % 70)),
                                   ActorBounds::Box({-.65f, 0, -.65f}, {.65f, 2.3f, .65f}),
                                   [i]
                                   {
                                       Person(0, 0, villagers[i].color);
                                   })
                                 .Id();
    }
    for (std::size_t i = 0; i < monsters.size(); ++i)
        monsters[i].actor =
            Place(L"숲 몬스터",
                  dynamic,
                  Placement(monsters[i].position),
                  ActorBounds::Box({-.7f, 0, -.9f}, {.7f, 1.2f, .9f}),
                  [i]
                  {
                      villageModels->Draw(monsters[i].kind == 0 ? Model::Slime : Model::Boar);
                  })
                .Id();
    heroActor = Place(L"플레이어",
                      dynamic,
                      Placement(player),
                      ActorBounds::Box({-.7f, 0, -.7f}, {.7f, 2.3f, .7f}),
                      []
                      {
                          Person(0, 0, {.43f, .34f, .72f}, true);
                      })
                    .Id();
    for (std::size_t i = 0; i < animals.size(); ++i)
        animalActors.push_back(Place(L"야생동물",
                                     dynamic,
                                     Placement(animals[i].position),
                                     ActorBounds::Box({-.8f, 0, -1.4f}, {.8f, 1.7f, 1.4f}),
                                     [i]
                                     {
                                         Animal local = animals[i];
                                         local.position = {0, 0};
                                         local.heading = 0;
                                         Wildlife(local);
                                     })
                                   .Id());

    for (P f : campfires)
    {
        Actor &camp = scene.Create(L"모닥불", terrain);
        camp.SetTransform(Placement(f));
        Place(L"장작과 돌",
              camp.Id(),
              Placement({0, 0}),
              ActorBounds::Box({-.7f, 0, -.7f}, {.7f, .4f, .7f}),
              []
              {
                  for (int i = 0; i < 8; ++i)
                  {
                      const float a = i * 6.2831853f / 8;
                      Ball(std::cos(a) * .47f, .15f, std::sin(a) * .47f, .17f, {.43f, .43f, .40f});
                  }
                  Box(0, .14f, 0, .7f, .14f, .17f, {.25f, .16f, .10f});
                  Box(0, .2f, 0, .16f, .14f, .7f, {.25f, .16f, .10f});
              });
        Place(
            L"불꽃과 불티",
            camp.Id(),
            Placement({0, 0}),
            ActorBounds::Box({-.7f, 0, -.4f}, {.7f, 2.5f, .4f}),
            []
            {
                DrawFire();
            },
            false,
            true);
    }
    Actor &lake = scene.Create(L"호수", terrain);
    lake.SetTransform(Placement({9, -4}));
    Place(
        L"호숫가 모래",
        lake.Id(),
        Placement({0, 0}),
        ActorBounds::Box({-4.6f, 0, -4.2f}, {4.6f, .05f, 4.2f}),
        []
        {
            Disk(0, .035f, 0, 4.6f, 4.2f, {.77f, .72f, .50f});
        },
        false);
    Place(
        L"수면과 물결",
        lake.Id(),
        Placement({0, 0}),
        ActorBounds::Box({-4.3f, .04f, -3.9f}, {4.3f, .15f, 3.9f}),
        []
        {
            DrawWater();
        },
        false);
    Place(L"목재 선착장",
          terrain,
          Placement({5.1f, -1.8f}),
          ActorBounds::Box({-.12f, 0, -.6f}, {1.4f, .2f, .6f}),
          []
          {
              for (int i = 0; i < 6; ++i)
                  Box(i * .24f, .12f, 0, .22f, .12f, 1.2f, {.5f, .34f, .18f});
          });
    Place(L"고대 문양석",
          terrain,
          Placement(stone),
          ActorBounds::Box({-.4f, 0, -.4f}, {.4f, 1.1f, .4f}),
          []
          {
              Box(0, .38f, 0, .75f, .76f, .65f, {.57f, .64f, .63f});
              renderer->MaterialMode(-1, stage >= 3 ? 1.5f : 0);
              Ball(0, .88f, 0, .18f, stage >= 3 ? C{.61f, .95f, .96f} : C{.54f, .48f, .74f});
          });
    Actor &seedNode = Place(L"빛씨앗",
                            dynamic,
                            Placement(seed),
                            ActorBounds::Box({-.25f, -.25f, -.25f}, {.25f, .25f, .25f}),
                            []
                            {
                                Ball(0, 0, 0, .25f, {1, .83f, .35f});
                            });
    ActorAppearance glow;
    glow.emission = 1.6f;
    seedNode.SetAppearance(glow);
    seedActor = seedNode.Id();
    creatureActor = Place(L"작은 생물",
                          dynamic,
                          Placement({0, 0}),
                          ActorBounds::Box({-.8f, 0, -.6f}, {.8f, 1, .6f}),
                          []
                          {
                              Creature(0, 0);
                          })
                        .Id();
    Actor &marker = Place(
        L"목적지 표시",
        dynamic,
        Placement({0, 0}),
        ActorBounds::Box({-.13f, -.13f, -.13f}, {.13f, .13f, .13f}),
        []
        {
            Ball(0, 0, 0, .12f, {1, .85f, .3f});
        },
        false,
        true);
    targetActor = marker.Id();
    glow.additive = true;
    glow.emission = 1.4f;
    marker.SetAppearance(glow);
    for (int i = 0; i < 6; ++i)
    {
        Actor &dot = Place(
            L"길 안내",
            dynamic,
            Placement({0, 0}),
            ActorBounds::Box({-.07f, -.01f, -.07f}, {.07f, .01f, .07f}),
            []
            {
                Disk(0, 0, 0, .065f, .065f, {1, .8f, .37f});
            },
            false,
            true);
        dot.SetAppearance(glow);
        breadcrumbActors.push_back(dot.Id());
    }
    Actor &magic = Place(
        L"빛 마법",
        dynamic,
        Placement(player),
        ActorBounds::Box({-2.6f, 0, -2.6f}, {2.6f, 1.5f, 2.6f}),
        []
        {
            for (int i = 0; i < 36; ++i)
            {
                const float a = i * 6.2831853f / 36, r = (1 - spell) * 2.5f;
                Ball(std::cos(a) * r, .35f + spell, std::sin(a) * r, .055f, {1, .8f, .4f});
            }
        },
        false,
        true);
    glow.emission = 2.3f;
    magic.SetAppearance(glow);
    spellActor = magic.Id();
    Actor &hud = scene.Create(L"튜토리얼 HUD");
    hud.SetPasses(false, false, true);
    hud.SetBounds(ActorBounds::Box({0, 0, -.01f}, {1280, 800, .01f}));
    hud.SetDraw(
        []
        {
            HUD();
        });
    SyncScene();
}

void SyncScene()
{
    Profiling::Scope profile("scene.sync");
    for (std::size_t i = 0; i < residents.size(); ++i)
        scene.Find(residents[i].actor)
            ->SetTransform(Placement(residents[i].position, 0, residents[i].heading));
    for (const auto &monster : monsters)
    {
        Actor *actor = scene.Find(monster.actor);
        actor->SetEnabled(monster.health > 0);
        actor->SetTransform(Placement(monster.position, 0, monster.heading));
    }
    scene.Find(heroActor)->SetTransform(Placement(player, 0, facing));
    for (std::size_t i = 0; i < animals.size(); ++i)
        scene.Find(animalActors[i])
            ->SetTransform(Placement(animals[i].position, 0, animals[i].heading));
    Actor *seedNode = scene.Find(seedActor);
    seedNode->SetTransform(Placement(seed, .6f + std::sin(time * 3) * .12f));
    seedNode->SetEnabled(stage <= 1);
    Actor *creature = scene.Find(creatureActor);
    creature->SetEnabled(stage >= 3);
    creature->SetTransform(
        Placement(stage == 3 ? P{stone.x - .7f, stone.z} : P{player.x - .65f, player.z + .5f}));
    const P target = Target();
    Actor *marker = scene.Find(targetActor);
    marker->SetEnabled(stage < 5);
    marker->SetTransform(Placement(target, 2.4f + std::sin(time * 3) * .12f));
    for (std::size_t i = 0; i < breadcrumbActors.size(); ++i)
    {
        const float fraction = float(i + 1) / 7;
        Actor *dot = scene.Find(breadcrumbActors[i]);
        dot->SetEnabled(stage < 5 && Dist(player, target) > 2);
        dot->SetTransform(Placement({player.x + (target.x - player.x) * fraction,
                                     player.z + (target.z - player.z) * fraction},
                                    .1f));
    }
    scene.Find(spellActor)->SetEnabled(spell > 0);
    scene.Find(spellActor)->SetTransform(Placement(player));
    scene.UpdateTransforms();
}

void HUD()
{
    Profiling::Scope profile("render.hud");
    Panel(24, 22, 775, 116);
    Text(44, 52, L"윌로미어 · 작은 시작", {1, .86f, .55f});
    Text(44, 86, Objective());
    int nearbyResident = -1;
    float nearbyDistance = 2.5f;
    for (int i = 0; i < npcCount; ++i)
    {
        const float distance = Dist(player, residents[i].position);
        if (distance < nearbyDistance)
        {
            nearbyResident = i;
            nearbyDistance = distance;
        }
    }
    if (nearbyResident >= 0)
        Text(44, 207, std::wstring(L"주변 인물: ") + villagers[nearbyResident].name + L" [E]");
    Text(44,
         119,
         L"경과 " + std::to_wstring(int(elapsed)) + L"초  ·  목표 3~5분  ·  시간 제한 없음",
         {.67f, .80f, .80f});
    Text(44,
         175,
         L"마을 | 레벨 " + std::to_wstring(sharedStats.level) + L" | 체력 " +
             std::to_wstring(testHealth) + L" | 동전 " + std::to_wstring(testCoins) +
             L" | 경험치 " + std::to_wstring(testExperience));
    Panel(24, 744, 1232, 38);
    Text(42, 772, L"WASD 이동  E 상호작용  좌클릭·스페이스·F 공격  Q 회복약  Tab 사냥터  ESC 정지");
    Panel(1040, 22, 216, 195);
    Text(1055, 50, L"주변 지도", {1, .86f, .55f});
    Text(1055, 78, L"숲길 · 마을 · 야영지");
    glPointSize(2);
    Renderer::CountedBegin(GL_POINTS);
    Color({.25f, .45f, .25f});
    for (P p : trees)
        glVertex2f(1147 + p.x * 1.65f, 131 + p.z * 1.1f);
    Color({.7f, .6f, .4f});
    for (P p : trailTiles)
        glVertex2f(1147 + p.x * 1.65f, 131 + p.z * 1.1f);
    Color({.75f, .64f, .5f});
    for (P house : houses)
        glVertex2f(1147 + house.x * 1.65f, 131 + house.z * 1.1f);
    Color({.3f, .6f, .85f});
    for (const auto &pond : ponds)
        for (int x = -6; x <= 6; ++x)
            for (int z = -6; z <= 6; ++z)
                if (x * x / (pond.rx * pond.rx) + z * z / (pond.rz * pond.rz) < 1)
                    glVertex2f(1147 + (pond.center.x + x) * 1.65f,
                               131 + (pond.center.z + z) * 1.1f);
    for (int x = -4; x <= 4; ++x)
        for (int z = -3; z <= 3; ++z)
            if (x * x / 20.f + z * z / 16.f < 1)
                glVertex2f(1147 + (9 + x) * 1.65f, 131 + (-4 + z) * 1.1f);
    glEnd();
    auto dot = [](P p, C c)
    {
        Color(c);
        glPointSize(3);
        Renderer::CountedBegin(GL_POINTS);
        glVertex2f(1147 + p.x * 1.65f, 131 + p.z * 1.1f);
        glEnd();
    };
    for (const auto &resident : residents)
        dot(resident.position, {.85f, .65f, .4f});
    for (const auto &monster : monsters)
        if (monster.health > 0)
            dot(monster.position, {.95f, .2f, .2f});
    dot(seed, {.53f, .85f, .48f});
    dot(stone, {.4f, .79f, 1});
    dot(player, {1, 1, 1});
    Text(1055, 204, L"흰 점: 내 위치");
    bool isNearby = Dist(player, Target()) < 1.9f;
    for (const auto &resident : residents)
        isNearby = isNearby || Dist(player, resident.position) < 1.9f;
    if (isNearby && dialog.empty())
    {
        Panel(410, 677, 460, 42);
        Text(430,
             706,
             stage == 2 && Dist(player, stone) < 2 ? L"[스페이스] 돌의 문양 깨우기"
                                                   : L"[E] 가까운 대상과 상호작용");
    }
    if (!dialog.empty())
    {
        Panel(24, 602, 1232, 127);
        Text(44, 635, speaker, {1, .86f, .55f});
        Text(44, 675, dialog);
        Text(44, 713, L"[E] 계속");
    }
    if (levelOneTransitionPending && dialog.empty())
    {
        Panel(290, 265, 700, 180);
        Text(330, 312, L"튜토리얼 완료", {1, .86f, .55f});
        Text(330, 350, L"역사책이 잊어버린 작은 친구를 만났습니다.");
        Text(330, 387, L"보상: 마법학교 초대장과 배고픈 동행자");
        Text(330, 424, L"레벨 1 사냥터로 이동합니다.");
    }
    if (paused)
    {
        Panel(430, 300, 420, 130);
        Text(465, 350, L"일시정지");
        Text(465, 396, L"ESC 계속하기    R 다시 시작");
    }
}

bool Initialize(int width, int height)
{
    renderer.reset(new Renderer(width, height));
    if (!renderer->IsInitialized())
        OutputDebugStringA(renderer->Error().c_str());
    villageModels.reset(new ModelCache());
    return renderer->IsInitialized() && villageModels->Initialize();
}

void Shutdown()
{
    scene.Clear();
    villageModels.reset();
    renderer.reset();
}

int MonitorRegion(P p)
{
    if (p.x > -17 && p.x < 18 && p.z > -10 && p.z < 28)
        return 0;
    return p.z < 0 ? (p.x < 0 ? 1 : 2) : (p.x < 0 ? 3 : 4);
}

void PublishMonitoring()
{
    if (!Monitoring::Due())
        return;
    Profiling::Scope profile("monitoring.snapshot");
    static unsigned long long sequence = 0;
    static std::vector<P> previous;
    static const wchar_t *roles[] = {L"작업",
                                     L"학습",
                                     L"실험",
                                     L"조사",
                                     L"경비",
                                     L"상인",
                                     L"성직자",
                                     L"친절한 주민",
                                     L"산적",
                                     L"심술궂은 주민"};
    static const wchar_t *regions[] = {
        L"중앙 마을", L"북서 외곽", L"북동 외곽", L"남서 외곽", L"남동 외곽"};
    int npcCounts[5] = {}, mobCounts[5] = {}, deadCounts[5] = {}, hostileCounts[5] = {};
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << "{\"schema_version\":1,\"mode\":\"village\",\"session_id\":" << GetCurrentProcessId()
        << ",\"sequence\":" << ++sequence << ",\"paused\":" << (paused ? "true" : "false")
        << ",\"dialogue_open\":" << (!dialog.empty() ? "true" : "false")
        << ",\"player\":{\"health\":" << testHealth << ",\"coins\":" << testCoins
        << ",\"experience\":" << testExperience << "},\"npcs\":[";
    for (std::size_t i = 0; i < residents.size(); ++i)
    {
        const auto &r = residents[i];
        int region = MonitorRegion(r.position);
        ++npcCounts[region];
        if (r.behavior == Behavior::Bandit)
            ++hostileCounts[region];
        bool moved = previous.size() == residents.size() && Dist(previous[i], r.position) > .05f;
        const wchar_t *state = paused            ? L"일시정지"
                               : !dialog.empty() ? L"대화로 대기"
                               : r.stun > 0      ? L"후퇴 중"
                               : moved           ? L"이동 중"
                               : r.cooldown > 0  ? L"행동 대기"
                                                 : L"대기";
        if (i)
            out << ',';
        out << "{\"id\":" << i << ",\"actor_id\":" << r.actor
            << ",\"name\":" << Monitoring::Quote(villagers[i].name)
            << ",\"role\":" << Monitoring::Quote(roles[int(r.behavior)])
            << ",\"state\":" << Monitoring::Quote(state) << ",\"region\":" << region
            << ",\"x\":" << r.position.x << ",\"z\":" << r.position.z
            << ",\"cooldown_seconds\":" << r.cooldown << ",\"stun_seconds\":" << r.stun << '}';
    }
    previous.clear();
    for (const auto &r : residents)
        previous.push_back(r.position);
    out << "],\"monsters\":[";
    for (std::size_t i = 0; i < monsters.size(); ++i)
    {
        const auto &m = monsters[i];
        int region = MonitorRegion(m.health > 0 ? m.position : m.home);
        if (m.health > 0)
            ++mobCounts[region];
        else
            ++deadCounts[region];
        if (i)
            out << ',';
        out << "{\"id\":" << i
            << ",\"kind\":" << Monitoring::Quote(m.kind == 0 ? L"슬라임" : L"멧돼지")
            << ",\"region\":" << region << ",\"health\":" << m.health
            << ",\"respawn_seconds\":" << (std::max)(0.f, m.respawn) << ",\"x\":" << m.position.x
            << ",\"z\":" << m.position.z << '}';
    }
    out << "],\"regions\":[";
    for (int i = 0; i < 5; ++i)
    {
        if (i)
            out << ',';
        out << "{\"id\":" << i << ",\"name\":" << Monitoring::Quote(regions[i])
            << ",\"npc_count\":" << npcCounts[i] << ",\"hostile_npc_count\":" << hostileCounts[i]
            << ",\"monsters_alive\":" << mobCounts[i]
            << ",\"monsters_respawning\":" << deadCounts[i] << '}';
    }
    out << "],\"writer\":{\"queued\":" << Monitoring::Writer().queued.load()
        << ",\"dropped_total\":" << Monitoring::Writer().dropped.load()
        << ",\"errors_total\":" << Monitoring::Writer().failures.load() << "}}";
    Monitoring::Publish(out.str());
}

void Render()
{
    if (!renderer || !renderer->IsInitialized())
        return;
    Renderer::BeginFrame();
    SyncScene();
    {
        Profiling::Scope profile("render.shadow", true);
        renderer->BeginShadow(camera.x, camera.z);
        scene.Draw(*renderer, ScenePass::Shadow, time);
    }
    {
        Profiling::Scope profile("render.world", true);
        renderer->BeginScene(camera.x, camera.z);
        scene.Draw(*renderer, ScenePass::World, time);
    }
    {
        Profiling::Scope profile("render.postprocess", true);
        renderer->EndScene();
    }
    {
        Profiling::Scope profile("render.ui", true);
        renderer->BeginUI();
        scene.Draw(*renderer, ScenePass::UI, time);
    }
    {
        Profiling::Scope profile("present.swap_buffers", false);
        glutSwapBuffers();
    }
    PublishMonitoring();
    Renderer::EndFrame();
}
}
