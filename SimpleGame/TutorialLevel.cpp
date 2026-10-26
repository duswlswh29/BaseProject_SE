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

static const NPC npcTemplates[] = {
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
constexpr int VillageCount = 5;
constexpr int ResidentsPerVillage = 20;
constexpr int npcCount = VillageCount * ResidentsPerVillage;
constexpr float PlayerSpeed = 3.2f;
constexpr float VillageSpacing = 40.f;
constexpr float MapMinX = -30, MapMaxX = 190, MapMinZ = -40, MapMaxZ = 40;
static const wchar_t *villageNames[] = {L"윌로미어", L"솔바람", L"호수빛", L"돌나루", L"새벽숲"};
static std::vector<NPC> villagers;
static std::wstring residentNames[npcCount];
static std::vector<P> houses;

struct StreetLamp
{
    P position;
    float heading;
};

static std::vector<StreetLamp> streetLamps;
static std::vector<ActorId> groundChunks;
static float groundChunkSize = 64.f;
static float rumorClock = 0;
static unsigned rumorTransfers = 0;

P VillageCenter(int id)
{
    return {id * VillageSpacing, 0};
}

int NearestVillage(P p)
{
    return (std::max)(0, (std::min)(VillageCount - 1, int(std::floor(p.x / VillageSpacing + .5f))));
}

struct Pond
{
    P center;
    float rx, rz;
};

static const Pond ponds[] = {
    {{22, 30}, 5, 3.2f}, {{62, -30}, 5.8f, 3.8f}, {{102, 32}, 4.5f, 5}, {{146, -28}, 5.5f, 3.6f}};
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
    int homeVillage = 0, destination = 0, routePhase = 0, direction = 1;
    bool courier = false;
    bool walking = false;
    float visitWait = 5;
    unsigned rumors = 0;
    P lastExchange = {0, 0};
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
    glPushMatrix();
    glTranslatef(x, y, z);
    glScalef(a, b, c);
    villageModels->DrawTinted(Model::WoodBlock, color.r, color.g, color.b);
    glPopMatrix();
}

void Ball(float x, float y, float z, float r, C c)
{
    glPushMatrix();
    glTranslatef(x, y, z);
    glScalef(r * 2, r * 2, r * 2);
    villageModels->DrawTinted(Model::LeafCluster, c.r, c.g, c.b);
    glPopMatrix();
}

void Cone(float x, float y, float z, float r, float height, C c)
{
    glPushMatrix();
    glTranslatef(x, y + height * .5f, z);
    glScalef(r * 2, height, r * 2);
    villageModels->DrawTinted(Model::LeafCluster, c.r, c.g, c.b);
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

static const Trail localTrails[] = {
    {{0, 3}, {0, -3}, 2.4f},        {{0, -3}, {-8, -3}, 2.2f},   {{-8, -3}, {-8, -6}, 2.2f},
    {{-8, -6}, {-17, -12}, 2.8f},   {{-17, -12}, {-24, -18}, 3}, {{-24, -18}, {-36, -27}, 3},
    {{-36, -27}, {-42, -12}, 2.5f}, {{-42, -12}, {-32, 5}, 3},   {{-32, 5}, {-12, 12}, 3},
    {{-12, 12}, {0, 12}, 2.5f},     {{0, 3}, {0, 22}, 3},        {{0, 12}, {12, 12}, 2.5f},
    {{0, 22}, {8, 29}, 3},          {{8, 29}, {24, 32}, 3},      {{24, 32}, {36, 16}, 3},
    {{36, 16}, {32, 1}, 3},         {{32, 1}, {25, -19}, 3},     {{25, -19}, {15, -12}, 2.8f},
    {{15, -12}, {4, -10}, 2.5f},    {{4, -10}, {4, -4}, 2.2f},   {{4, -4}, {0, -3}, 2.2f}};
static std::vector<Trail> trails;

void ConfigureVillages()
{
    villagers.clear();
    houses.clear();
    trails.clear();
    rumorClock = 0;
    rumorTransfers = 0;
    const P houseOffsets[] = {
        {-5, 3}, {5, 4}, {-5, 8}, {5, 9}, {-6, 15}, {6, 19}, {-13, 8}, {13, 11}};
    for (const Trail &trail : localTrails)
        if (trail.a.x >= -17 && trail.a.x <= 17 && trail.b.x >= -17 && trail.b.x <= 17)
            trails.push_back(trail);
    for (int v = 0; v < VillageCount; ++v)
    {
        P c = VillageCenter(v);
        trails.push_back({{c.x, 0}, {c.x, -12}, 3});
        trails.push_back({{c.x, -12}, {c.x + (v < 4 ? VillageSpacing : -VillageSpacing), -12}, 3});
        trails.push_back({{c.x, 0}, {c.x, 23}, 3});
        for (P h : houseOffsets)
        {
            houses.push_back({c.x + h.x, h.z});
            trails.push_back({{c.x + h.x, h.z + 2.2f}, {c.x, h.z + 2.2f}, 2});
        }
        for (int j = 0; j < ResidentsPerVillage; ++j)
        {
            int i = v * ResidentsPerVillage + j;
            NPC n = npcTemplates[j];
            if (v == 0)
            {
                n.x = npcTemplates[j].x;
                n.z = npcTemplates[j].z;
            }
            else
            {
                float a = j * 2.399963f, radius = 2.5f + (j % 4) * 1.3f;
                n.x = c.x + std::cos(a) * radius;
                n.z = std::sin(a) * radius;
            }
            if (j == 1 || j == 2)
            {
                n.x = c.x + (j == 1 ? -1.f : 1.f);
                n.z = -2;
            }
            residentNames[i] =
                v == 0 ? std::wstring(n.name) : std::wstring(villageNames[v]) + L" " + n.name;
            n.name = residentNames[i].c_str();
            villagers.push_back(n);
        }
    }
}

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
    return false;
}

void BuildLandscape()
{
    Profiling::Scope profile("village.landscape_generate");
    trees.clear();
    trailTiles.clear();
    // Bake path coverage once; render and map share this layout.
    for (int x = int(MapMinX * 2); x < int(MapMaxX * 2); ++x)
        for (int z = int(MapMinZ * 2); z < int(MapMaxZ * 2); ++z)
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
    std::vector<P> groves;
    for (int v = 0; v < VillageCount; ++v)
        for (float z : {-30.f, 30.f})
            for (float x : {-16.f, 16.f})
                groves.push_back({v * VillageSpacing + x, z});
    for (P grove : groves)
        for (int attempt = 0, placed = 0; attempt < 300 && placed < 24; ++attempt)
        {
            float angle = unit(random) * 6.2831853f;
            float radius = std::sqrt(unit(random)) * 10;
            P p = {grove.x + std::cos(angle) * radius, grove.z + std::sin(angle) * radius * .85f};
            if (p.x < MapMinX + 2 || p.x > MapMaxX - 2 || p.z < MapMinZ + 2 || p.z > MapMaxZ - 2 ||
                OnTrail(p, 1.6f))
                continue;
            if (std::fabs(p.x - VillageCenter(NearestVillage(p)).x) < 17 && p.z > -10 && p.z < 25)
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
    for (int attempt = 0; attempt < 10000 && shrubs.size() < 1000; ++attempt)
    {
        P p = {MapMinX + unit(random) * (MapMaxX - MapMinX),
               MapMinZ + unit(random) * (MapMaxZ - MapMinZ)};
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
    ConfigureVillages();
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
    return std::fabs(p.x - VillageCenter(NearestVillage(p)).x) < 16 && p.z > -8 && p.z < 27;
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

P FindMonsterSpawn(P oldPosition)
{
    static std::mt19937 rng(20261022);
    std::uniform_real_distribution<float> unit(0, 1);
    for (int attempt = 0; attempt < 2048; ++attempt)
    {
        int gap = int(rng() % 4);
        P p = {gap * VillageSpacing + 17 + unit(rng) * 6,
               MapMinZ + 3 + unit(rng) * (MapMaxZ - MapMinZ - 6)};
        if (Blocked(p) || SafeVillage(p) || OnTrail(p, 1.5f) || Dist(p, player) < 12 ||
            Dist(p, oldPosition) < 10)
            continue;
        bool free = true;
        for (const auto &r : residents)
            if (Dist(r.position, p) < 2)
            {
                free = false;
                break;
            }
        for (const auto &m : monsters)
            if (m.health > 0 && Dist(m.position, p) < 1.5f)
            {
                free = false;
                break;
            }
        if (free)
            return p;
    }
    return oldPosition; // Retry later rather than spawning inside a character or obstacle.
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

    for (int i = 0; i < npcCount; ++i)
    {
        ResidentState state;
        state.home = state.position = FindFree({villagers[i].x, villagers[i].z});
        state.behavior = patterns[i % ResidentsPerVillage];
        state.homeVillage = i / ResidentsPerVillage;
        state.courier = i % ResidentsPerVillage == 1 || i % ResidentsPerVillage == 2;
        state.direction = state.homeVillage == 4 ? -1 : 1;
        state.destination = state.homeVillage;
        state.visitWait = 5 + float(i % 3);
        state.rumors = i % ResidentsPerVillage == 0 ? (1u << state.homeVillage) : 0;
        state.lastExchange = state.position;
        residents.push_back(state);
    }
    for (int i = 0; i < 100; ++i)
    {
        VillageMonster monster;
        monster.kind = i % 2;
        P spawn = FindMonsterSpawn({-1000, -1000});
        if (spawn.x < MapMinX)
        {
            // Keep a dormant slot and retry; never place a live monster out of bounds.
            monster.health = 0;
            monster.respawn = .5f;
            spawn = {20, -20};
        }
        monster.home = monster.position = spawn;
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
                monster.respawn = 2;
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
                if (monster.respawn <= 0)
                {
                    P spawn = FindMonsterSpawn(monster.position);
                    if (Dist(spawn, monster.position) >= 10)
                    {
                        monster.home = monster.position = spawn;
                        monster.health = 3;
                    }
                    else
                        monster.respawn = .5f;
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
            const P previousPosition = resident.position;
            resident.walking = false;
            resident.cooldown = (std::max)(0.f, resident.cooldown - dt);
            resident.stun = (std::max)(0.f, resident.stun - dt);
            if (i == 0)
                continue; // Original quest giver stays reachable.
            if (resident.courier)
            {
                if (resident.visitWait > 0)
                {
                    resident.visitWait -= dt;
                    if (resident.visitWait <= 0)
                    {
                        if (resident.destination == 4)
                            resident.direction = -1;
                        if (resident.destination == 0)
                            resident.direction = 1;
                        resident.routePhase = 0;
                    }
                }
                else
                {
                    const float lane = -12.f + (i % 2 ? -.4f : .4f);
                    P destination = VillageCenter(resident.destination);
                    destination.x += i % 2 ? -.9f : .9f;
                    P next = resident.routePhase == 0   ? P{destination.x, lane}
                             : resident.routePhase == 1 ? P{destination.x, lane}
                                                        : P{destination.x, -2};
                    P before = resident.position;
                    WalkToward(resident.position, next, 2.4f, dt, resident.heading, .35f);
                    if (Dist(before, resident.position) < .001f && Dist(before, next) > .65f)
                    {
                        P detour = {before.x + (i % 2 ? -dt : dt), before.z};
                        if (CanStep(resident.position, detour))
                            resident.position = detour;
                    }
                    if (Dist(resident.position, next) < .65f)
                    {
                        if (resident.routePhase == 0)
                        {
                            resident.destination += resident.direction;
                            resident.routePhase = 1;
                        }
                        else if (resident.routePhase == 1)
                            resident.routePhase = 2;
                        else
                        {
                            resident.visitWait = 12;
                            resident.routePhase = 0;
                        }
                    }
                }
                resident.walking = Dist(previousPosition, resident.position) > .0001f;
                continue;
            }
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
                                monster.respawn = 2;
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
            resident.walking = Dist(previousPosition, resident.position) > .0001f;
        }
    }
    // Only a physical encounter can transfer knowledge. Snapshot masks prevent
    // a rumor from crossing multiple NPCs in one simulation tick.
    rumorClock += dt;
    if (rumorClock >= 1)
    {
        rumorClock = 0;
        std::vector<unsigned> next;
        for (const auto &r : residents)
            next.push_back(r.rumors);
        for (std::size_t a = 0; a < residents.size(); ++a)
            for (std::size_t b = a + 1; b < residents.size(); ++b)
            {
                auto &ra = residents[a];
                auto &rb = residents[b];
                if (Dist(ra.position, rb.position) > 3 ||
                    (Dist(ra.position, ra.lastExchange) < .25f &&
                     Dist(rb.position, rb.lastExchange) < .25f))
                    continue;
                unsigned combined = ra.rumors | rb.rumors;
                if (combined != ra.rumors || combined != rb.rumors)
                    ++rumorTransfers;
                next[a] |= combined;
                next[b] |= combined;
            }
        for (std::size_t i = 0; i < residents.size(); ++i)
        {
            if (residents[i].rumors != next[i])
                residents[i].lastExchange = residents[i].position;
            residents[i].rumors = next[i];
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
    {
        Say(n.name, n.line);
        for (int v = 0; v < VillageCount; ++v)
            if (resident.rumors & (1u << v))
            {
                dialog += std::wstring(L" 전해 들은 이야기: ") + villageNames[v] +
                          L" 숲에서 정체 모를 빛이 보였대요.";
                break;
            }
    }
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
                const float step = PlayerSpeed * dt / len;
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
    glPushMatrix();
    glTranslatef(x, 0, z);
    villageModels->DrawAnimated(Model::HeroIdle, Model::Hero0, walk / 6.2831853f, moving);
    glPopMatrix();
}

void Creature(float x, float z)
{
    glPushMatrix();
    glTranslatef(x, 0, z);
    glScalef(.45f, .45f, .45f);
    villageModels->DrawAnimated(Model::CatIdle, Model::Cat0, time, moving && stage >= 4);
    glPopMatrix();
}

void Wildlife(const Animal &a)
{
    glPushMatrix();
    glTranslatef(a.position.x, 0, a.position.z);
    glRotatef(a.heading, 0, 1, 0);
    if (a.kind == 0)
        villageModels->DrawAnimated(Model::DeerIdle, Model::Deer0, a.phase / 6.2831853f, true);
    else if (a.kind == 1)
    {
        glScalef(.65f, .65f, .65f);
        villageModels->DrawAnimated(Model::CatIdle, Model::Cat0, a.phase / 6.2831853f, true);
    }
    else
        villageModels->DrawAnimated(Model::WolfIdle, Model::Wolf0, a.phase / 6.2831853f, true);
    glPopMatrix();
}

void DrawStreetLamp()
{
    renderer->MaterialMode();
    const C wood = {.25f, .13f, .055f};
    Box(0, .15f, 0, .38f, .30f, .38f, {.29f, .28f, .23f});
    Box(0, 1.40f, 0, .18f, 2.50f, .18f, wood);
    Box(.34f, 2.56f, 0, .90f, .16f, .18f, wood);
    Box(.22f, 2.28f, 0, .55f, .10f, .13f, wood);
    Box(.64f, 2.30f, 0, .045f, .42f, .045f, {.12f, .11f, .09f});
    glPushMatrix();
    glTranslatef(.64f, 1.75f, 0);
    glScalef(1.25f, 1.25f, 1.25f);
    villageModels->Draw(Model::Lantern);
    glPopMatrix();
}

void DrawHouse()
{
    villageModels->Draw(Model::House);
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
    renderer->MaterialMode();
    renderer->EffectMode(1, time);
    Color({1, 1, 1});
    Renderer::CountedBegin(GL_TRIANGLE_FAN);
    glNormal3f(0, 1, 0);
    glTexCoord2f(.5f, .5f);
    glVertex3f(0, .10f, 0);
    for (int i = 0; i <= 64; ++i)
    {
        const float angle = -i * 6.2831853f / 64;
        const float x = std::cos(angle), z = std::sin(angle);
        glTexCoord2f(x * .5f + .5f, z * .5f + .5f);
        glVertex3f(x * 4.25f, .10f, z * 3.85f);
    }
    glEnd();
    renderer->EffectMode(0, time);
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

// One shared height field drives the terrace mesh and all grounded actor transforms.
constexpr int TerraceWidth = int(MapMaxX - MapMinX);
constexpr int TerraceDepth = int(MapMaxZ - MapMinZ);
constexpr float TerraceStepHeight = .50f;
static std::vector<float> terraceHeights;

float TerraceHeight(P p)
{
    const int x = int(std::floor(p.x - MapMinX));
    const int z = int(std::floor(p.z - MapMinZ));
    if (x < 0 || z < 0 || x >= TerraceWidth || z >= TerraceDepth || terraceHeights.empty())
        return 0;
    return terraceHeights[z * TerraceWidth + x];
}

void BuildTerraceHeights()
{
    Profiling::Scope profile("village.terrace_generate");
    terraceHeights.assign(TerraceWidth * TerraceDepth, 0);
    for (int z = 0; z < TerraceDepth; ++z)
        for (int x = 0; x < TerraceWidth; ++x)
        {
            P p = {MapMinX + x + .5f, MapMinZ + z + .5f};
            float height = 0;
            for (int v = 0; v < VillageCount; ++v)
            {
                const float cx = VillageCenter(v).x;
                const P hills[] = {{cx - 13, -2}, {cx + 13, 19}, {cx - 8, 27}, {cx + 8, -25}};
                for (P hill : hills)
                {
                    const float dx = (p.x - hill.x) / 12.f;
                    const float dz = (p.z - hill.z) / 10.f;
                    const float radius = std::sqrt(dx * dx + dz * dz);
                    height = (std::max)(height, 2.2f * (1.f - radius));
                }
            }
            float clearance = 100;
            for (const Trail &trail : trails)
                clearance =
                    (std::min)(clearance, SegmentDistance(p, trail) - trail.width * .5f - 1.f);
            for (P house : houses)
                clearance = (std::min)(clearance, Dist(p, house) - 3.8f);
            for (P fire : campfires)
                clearance = (std::min)(clearance, Dist(p, fire) - 2.f);
            clearance = (std::min)(clearance, Dist(p, stone) - 3.f);
            clearance = (std::min)(clearance, Dist(p, seed) - 3.f);
            if (InPond(p, 2.f) || Dist(p, {9, -4}) < 6.f)
                clearance = 0;
            height = (std::min)(height, clearance * .18f);
            terraceHeights[z * TerraceWidth + x] =
                std::floor((std::max)(0.f, height) / TerraceStepHeight) * TerraceStepHeight;
        }
    // Preserve broad half-meter terraces without introducing extra thin steps.
    for (int pass = 0; pass < 16; ++pass)
        for (int z = 0; z < TerraceDepth; ++z)
            for (int x = 0; x < TerraceWidth; ++x)
            {
                float &h = terraceHeights[z * TerraceWidth + x];
                for (P d : {P{-1, 0}, P{1, 0}, P{0, -1}, P{0, 1}})
                    h = (std::min)(h,
                                   TerraceHeight(
                                       {MapMinX + x + d.x + .5f, MapMinZ + z + d.z + .5f}) +
                                       TerraceStepHeight);
            }
}

struct TerraceMesh
{
    struct Vertex
    {
        float x, y, z, nx, ny, nz, r, g, b, a, u, v;
    };

    GLuint buffer = 0;
    GLsizei count = 0;

    ~TerraceMesh()
    {
        if (buffer)
            glDeleteBuffers(1, &buffer);
    }

    void Draw() const
    {
        renderer->MaterialMode();
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_NORMAL_ARRAY);
        glEnableClientState(GL_COLOR_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glVertexPointer(3, GL_FLOAT, sizeof(Vertex), nullptr);
        glNormalPointer(GL_FLOAT, sizeof(Vertex), reinterpret_cast<void *>(12));
        glColorPointer(4, GL_FLOAT, sizeof(Vertex), reinterpret_cast<void *>(24));
        glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex), reinterpret_cast<void *>(40));
        Renderer::CountedDrawArrays(GL_TRIANGLES, 0, count);
        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_NORMAL_ARRAY);
        glDisableClientState(GL_COLOR_ARRAY);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
};

ActorTransform Placement(
    P p, float y = 0, float heading = 0, float sx = 1, float sy = 1, float sz = 1)
{
    return {{p.x, y + TerraceHeight(p), p.z}, {0, heading, 0}, {sx, sy, sz}};
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
    BuildTerraceHeights();
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

    // Upload each 16m patch once; individual steps do not issue separate draw calls.
    for (int cz = 0; cz < TerraceDepth; cz += 16)
        for (int cx = 0; cx < TerraceWidth; cx += 16)
        {
            std::vector<TerraceMesh::Vertex> vertices;
            auto quad = [&](Vector3 a, Vector3 b, Vector3 c, Vector3 d, Vector3 n, bool top)
            {
                for (Vector3 p : {a, b, c, a, c, d})
                    vertices.push_back({p.x,
                                        p.y,
                                        p.z,
                                        n.x,
                                        n.y,
                                        n.z,
                                        top ? .29f : .32f,
                                        top ? .39f : .29f,
                                        top ? .12f : .22f,
                                        1,
                                        p.x,
                                        top ? -108.f : -103.f});
            };
            for (int z = cz; z < (std::min)(cz + 16, TerraceDepth); ++z)
                for (int x = cx; x < (std::min)(cx + 16, TerraceWidth); ++x)
                {
                    float px = MapMinX + x, pz = MapMinZ + z;
                    float h = TerraceHeight({px + .5f, pz + .5f});
                    if (h < .01f)
                        continue;
                    quad({px, h, pz},
                         {px, h, pz + 1},
                         {px + 1, h, pz + 1},
                         {px + 1, h, pz},
                         {0, 1, 0},
                         true);
                    float low = TerraceHeight({px - .5f, pz + .5f});
                    if (low < h)
                        quad({px, low, pz},
                             {px, low, pz + 1},
                             {px, h, pz + 1},
                             {px, h, pz},
                             {-1, 0, 0},
                             false);
                    low = TerraceHeight({px + 1.5f, pz + .5f});
                    if (low < h)
                        quad({px + 1, low, pz + 1},
                             {px + 1, low, pz},
                             {px + 1, h, pz},
                             {px + 1, h, pz + 1},
                             {1, 0, 0},
                             false);
                    low = TerraceHeight({px + .5f, pz - .5f});
                    if (low < h)
                        quad({px + 1, low, pz},
                             {px, low, pz},
                             {px, h, pz},
                             {px + 1, h, pz},
                             {0, 0, -1},
                             false);
                    low = TerraceHeight({px + .5f, pz + 1.5f});
                    if (low < h)
                        quad({px, low, pz + 1},
                             {px + 1, low, pz + 1},
                             {px + 1, h, pz + 1},
                             {px, h, pz + 1},
                             {0, 0, 1},
                             false);
                }
            if (vertices.empty())
                continue;
            auto mesh = std::make_shared<TerraceMesh>();
            mesh->count = static_cast<GLsizei>(vertices.size());
            glGenBuffers(1, &mesh->buffer);
            glBindBuffer(GL_ARRAY_BUFFER, mesh->buffer);
            glBufferData(GL_ARRAY_BUFFER,
                         vertices.size() * sizeof(TerraceMesh::Vertex),
                         vertices.data(),
                         GL_STATIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            Place(L"넓은 잔디 계단 지형",
                  terrain,
                  ActorTransform{},
                  ActorBounds::Box({MapMinX + cx, 0, MapMinZ + cz},
                                   {MapMinX + cx + 16, 2.5f, MapMinZ + cz + 16}),
                  [mesh]
                  {
                      mesh->Draw();
                  });
        }
    // A fixed pool covers the camera; crossing a cell reuses patches rather than growing memory.
    groundChunks.clear();
    for (int i = 0; i < 9; ++i)
    {
        Actor &ground = Place(
            L"연속 평지 구역",
            terrain,
            ActorTransform{},
            ActorBounds::Box({-32, -.05f, -32}, {32, 0, 32}),
            []
            {
                Ground(0, 0, groundChunkSize, groundChunkSize, Renderer::Grass);
            },
            false);
        groundChunks.push_back(ground.Id());
    }
    Place(
        L"돌길",
        terrain,
        Placement({0, 0}),
        ActorBounds::Box({MapMinX, 0, MapMinZ}, {MapMaxX, .04f, MapMaxZ}),
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
            Placement(shrub, 0, shrub.x * 13.f, 1, 1, 1),
            villageModels->Bounds(Model::GrassTuft),
            []
            {
                villageModels->Draw(Model::GrassTuft);
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
    for (std::size_t i = 8; i < houses.size(); ++i)
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
    for (std::size_t i = 0; i < houses.size(); ++i)
    {
        const P h = houses[i];
        const Model prop = i % 3 == 0 ? Model::Table : i % 3 == 1 ? Model::Barrel : Model::Chest;
        Place(L"주택 생활 소품",
              chunkFor(h),
              Placement({h.x - .7f, h.z - 2.2f}),
              villageModels->Bounds(prop),
              [prop]
              {
                  villageModels->Draw(prop);
              });
        Place(L"주택 등불",
              chunkFor(h),
              Placement({h.x + 1.1f, h.z + 1.5f}, 1.1f, 0, .55f, .55f, .55f),
              villageModels->Bounds(Model::Lantern),
              []
              {
                  villageModels->Draw(Model::Lantern);
              });
    }
    // Three hand-spaced lamps per village, along the outside of the main street.
    streetLamps.clear();
    for (int village = 0; village < VillageCount; ++village)
    {
        const P center = VillageCenter(village);
        const StreetLamp locations[] = {{{center.x - 2.25f, -7.f}, 0},
                                        {{center.x + 2.25f, 14.f}, 180},
                                        {{center.x - 2.25f, 22.5f}, 0}};
        for (const StreetLamp &lamp : locations)
        {
            streetLamps.push_back(lamp);
            Place(L"목재 가로등",
                  chunkFor(lamp.position),
                  Placement(lamp.position, 0, lamp.heading),
                  ActorBounds::Box({-.22f, 0, -.22f}, {.85f, 2.70f, .22f}),
                  DrawStreetLamp);
        }
    }
    for (P b : houses)
        Place(L"주택",
              chunkFor(b),
              Placement(b),
              villageModels->Bounds(Model::House),
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
              [t]
              {
                  villageModels->Draw(std::sin(t.x + t.z) > .45f ? Model::Broadleaf : Model::Tree);
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
                                       const auto &r = residents[i];
                                       const bool merchant = r.behavior == Behavior::Merchant;
                                       const bool healer = r.behavior == Behavior::Priest ||
                                                           r.behavior == Behavior::Helpful;
                                       villageModels->DrawAnimated(merchant ? Model::MerchantIdle
                                                                   : healer ? Model::HealerIdle
                                                                            : Model::HeroIdle,
                                                                   merchant ? Model::Merchant0
                                                                   : healer ? Model::Healer0
                                                                            : Model::Hero0,
                                                                   time * 1.1f + float(i) * .13f,
                                                                   r.walking);
                                   })
                                 .Id();
    }
    for (std::size_t i = 0; i < monsters.size(); ++i)
        monsters[i].actor = Place(L"숲 몬스터",
                                  dynamic,
                                  Placement(monsters[i].position),
                                  ActorBounds::Box({-1, -.2f, -1.6f}, {1, 1.9f, 1.6f}),
                                  [i]
                                  {
                                      villageModels->DrawAnimated(
                                          monsters[i].kind == 0 ? Model::CatIdle : Model::WolfIdle,
                                          monsters[i].kind == 0 ? Model::Cat0 : Model::Wolf0,
                                          time + float(i) * .17f,
                                          true);
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
                                     ActorBounds::Box({-1, -.2f, -1.6f}, {1, 2.2f, 1.6f}),
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
    // Account for wide windows as well as the rotated orthographic view.
    const float aspect =
        float(glutGet(GLUT_WINDOW_WIDTH)) / (std::max)(1, glutGet(GLUT_WINDOW_HEIGHT));
    groundChunkSize = (std::max)(64.f, std::ceil((22.f * aspect + 44.f) / 32.f) * 32.f);
    const float centerX = std::floor(camera.x / groundChunkSize) * groundChunkSize;
    const float centerZ = std::floor(camera.z / groundChunkSize) * groundChunkSize;
    for (std::size_t i = 0; i < groundChunks.size(); ++i)
    {
        Actor *ground = scene.Find(groundChunks[i]);
        ActorTransform transform;
        transform.position = {centerX + (int(i % 3) - 1) * groundChunkSize,
                              0,
                              centerZ + (int(i / 3) - 1) * groundChunkSize};
        ground->SetTransform(transform);
        const float half = groundChunkSize * .5f;
        ground->SetBounds(ActorBounds::Box({-half, -.05f, -half}, {half, 0, half}));
    }
    Profiling::Count("world.active_ground_chunks", double(groundChunks.size()));

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
        glVertex2f(1050 + ((p.x) - MapMinX) * (194.f / (MapMaxX - MapMinX)),
                   87 + ((p.z) - MapMinZ) * (96.f / (MapMaxZ - MapMinZ)));
    Color({.7f, .6f, .4f});
    for (P p : trailTiles)
        glVertex2f(1050 + ((p.x) - MapMinX) * (194.f / (MapMaxX - MapMinX)),
                   87 + ((p.z) - MapMinZ) * (96.f / (MapMaxZ - MapMinZ)));
    Color({.75f, .64f, .5f});
    for (P house : houses)
        glVertex2f(1050 + ((house.x) - MapMinX) * (194.f / (MapMaxX - MapMinX)),
                   87 + ((house.z) - MapMinZ) * (96.f / (MapMaxZ - MapMinZ)));
    Color({.3f, .6f, .85f});
    for (const auto &pond : ponds)
        for (int x = -6; x <= 6; ++x)
            for (int z = -6; z <= 6; ++z)
                if (x * x / (pond.rx * pond.rx) + z * z / (pond.rz * pond.rz) < 1)
                    glVertex2f(
                        1050 + (((pond.center.x + x)) - MapMinX) * (194.f / (MapMaxX - MapMinX)),
                        87 + (((pond.center.z + z)) - MapMinZ) * (96.f / (MapMaxZ - MapMinZ)));
    for (int x = -4; x <= 4; ++x)
        for (int z = -3; z <= 3; ++z)
            if (x * x / 20.f + z * z / 16.f < 1)
                glVertex2f(1050 + (((9 + x)) - MapMinX) * (194.f / (MapMaxX - MapMinX)),
                           87 + (((-4 + z)) - MapMinZ) * (96.f / (MapMaxZ - MapMinZ)));
    glEnd();
    auto dot = [](P p, C c)
    {
        // The minimap still represents the settled area; off-map actors stay at its edge.
        p.x = (std::max)(MapMinX, (std::min)(MapMaxX, p.x));
        p.z = (std::max)(MapMinZ, (std::min)(MapMaxZ, p.z));
        Color(c);
        glPointSize(3);
        Renderer::CountedBegin(GL_POINTS);
        glVertex2f(1050 + ((p.x) - MapMinX) * (194.f / (MapMaxX - MapMinX)),
                   87 + ((p.z) - MapMinZ) * (96.f / (MapMaxZ - MapMinZ)));
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
    Text(1055,
         204,
         (player.x < MapMinX || player.x > MapMaxX || player.z < MapMinZ || player.z > MapMaxZ)
             ? L"마을 지도 밖 탐험 중"
             : L"흰 점: 내 위치");
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
    return NearestVillage(p);
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
    int npcCounts[5] = {}, mobCounts[5] = {}, deadCounts[5] = {}, hostileCounts[5] = {};
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << "{\"schema_version\":2,\"mode\":\"village\",\"session_id\":" << GetCurrentProcessId()
        << ",\"map_bounds\":{\"min_x\":" << MapMinX << ",\"max_x\":" << MapMaxX
        << ",\"min_z\":" << MapMinZ << ",\"max_z\":" << MapMaxZ << "}"
        << ",\"rumor_transfers_total\":" << rumorTransfers << ",\"sequence\":" << ++sequence
        << ",\"paused\":" << (paused ? "true" : "false")
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
            << ",\"name\":" << Monitoring::Quote(villagers[i].name) << ",\"role\":"
            << Monitoring::Quote(r.courier ? L"마을 간 전달자" : roles[int(r.behavior)])
            << ",\"state\":" << Monitoring::Quote(state) << ",\"region\":" << region
            << ",\"home_village\":" << r.homeVillage << ",\"rumor_mask\":" << r.rumors
            << ",\"courier\":" << (r.courier ? "true" : "false")
            << ",\"destination_village\":" << r.destination << ",\"x\":" << r.position.x
            << ",\"z\":" << r.position.z << ",\"cooldown_seconds\":" << r.cooldown
            << ",\"stun_seconds\":" << r.stun << '}';
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
            << ",\"kind\":" << Monitoring::Quote(m.kind == 0 ? L"야생 고양이" : L"늑대")
            << ",\"region\":" << region << ",\"health\":" << m.health
            << ",\"respawn_seconds\":" << (std::max)(0.f, m.respawn) << ",\"x\":" << m.position.x
            << ",\"z\":" << m.position.z << '}';
    }
    out << "],\"regions\":[";
    for (int i = 0; i < 5; ++i)
    {
        if (i)
            out << ',';
        int rumorKnown = 0, knowledge = 0, nearbyThreats = 0;
        for (const auto &r : residents)
            if (r.homeVillage == i)
            {
                if (r.rumors)
                    ++rumorKnown;
                for (int bit = 0; bit < VillageCount; ++bit)
                    if (r.rumors & (1u << bit))
                        ++knowledge;
            }
        for (const auto &m : monsters)
            if (m.health > 0 && Dist(m.position, VillageCenter(i)) < 22)
                ++nearbyThreats;
        const int safety = (std::max)(0, 100 - nearbyThreats * 6);
        const int curiosity = knowledge * 100 / (ResidentsPerVillage * VillageCount);
        const wchar_t *mood = safety < 55 ? L"불안" : curiosity >= 40 ? L"호기심" : L"평온";
        out << "{\"id\":" << i << ",\"name\":" << Monitoring::Quote(villageNames[i])
            << ",\"resident_count\":" << ResidentsPerVillage
            << ",\"center_x\":" << VillageCenter(i).x << ",\"center_z\":0"
            << ",\"mood\":" << Monitoring::Quote(mood) << ",\"safety_score\":" << safety
            << ",\"curiosity_score\":" << curiosity
            << ",\"rumor_informed_residents\":" << rumorKnown
            << ",\"nearby_threat_count\":" << nearbyThreats << ",\"npc_count\":" << npcCounts[i]
            << ",\"hostile_npc_count\":" << hostileCounts[i]
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
    std::vector<Renderer::PointLight> lights;
    lights.reserve(houses.size() + 5 + streetLamps.size());
    for (P house : houses)
        lights.push_back({house.x + 1.1f, 1.3f, house.z + 1.5f, 4.2f});
    for (P fire : campfires)
        lights.push_back({fire.x, 1.0f, fire.z, 5.f, 1.2f, .46f, .11f});
    for (const StreetLamp &lamp : streetLamps)
    {
        const float side = lamp.heading == 0 ? 1.f : -1.f;
        lights.push_back({lamp.position.x + side * .64f,
                          TerraceHeight(lamp.position) + 2.05f,
                          lamp.position.z,
                          3.6f,
                          .85f,
                          .46f,
                          .16f});
    }
    renderer->SetPointLights(lights);

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
