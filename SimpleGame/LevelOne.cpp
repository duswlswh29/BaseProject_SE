#include "stdafx.h"
#include "LevelOne.h"
#include "ModelCache.h"
#include "RpgWorld.h"
#include "Dependencies/freeglut.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

namespace LevelOne
{
namespace
{
using Rpg::Point;

struct Enemy
{
    Point position;
    int kind = 0;
    int health = 0;
    int maximumHealth = 0;
    float heading = 0;
    float cooldown = 0;
    float respawn = 0;
    float hitFlash = 0;
};

struct Projectile
{
    Point position;
    Point direction;
    float life = 1;
    int damage = 0;
};

struct Loot
{
    Point position;
    int kind = 0;
};

struct FloatingText
{
    Point position;
    std::wstring text;
    float remaining = 1.2f;
};

std::unique_ptr<Renderer> renderer;
std::unique_ptr<ModelCache> models;
Rpg::World world;
Rpg::Stats stats;
std::mt19937 random;
std::vector<Enemy> enemies;
std::vector<Projectile> projectiles;
std::vector<Loot> loot;
std::vector<FloatingText> floating;
std::vector<int> navigation;
Point player;
Point camera;
bool keys[256] = {};
bool paused = false;
bool mouseAttack = false;
bool running = false;
bool moving = false;
bool inventoryOpen = false;
bool confirmNewMap = false;
bool saveDirty = false;
float time = 0;
float walkTime = 0;
float attackCooldown = 0;
float facing = 0;
float invulnerable = 0;
float pathTimer = 0;
float saveTimer = 0;
float messageTimer = 0;
int lastTime = 0;
std::wstring message;
std::wstring profilePath;

void Notice(const std::wstring &text)
{
    message = text;
    messageTimer = 4;
}

bool SaveProfile()
{
    const int record[] = {0x31504752,
                          1,
                          stats.level,
                          stats.experience,
                          stats.totalExperience,
                          stats.health,
                          stats.potions,
                          stats.crystals,
                          stats.coins,
                          stats.kills};
    const std::wstring temporary = profilePath + L".tmp";
    std::ofstream output(temporary.c_str(), std::ios::binary | std::ios::trunc);
    output.write(reinterpret_cast<const char *>(record), sizeof(record));
    output.flush();
    const bool okay = output.good();
    output.close();
    const bool saved = okay && MoveFileExW(temporary.c_str(),
                                           profilePath.c_str(),
                                           MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
    if (saved)
    {
        saveDirty = false;
    }
    return saved;
}

void LoadProfile()
{
    wchar_t executable[32768] = {};
    GetModuleFileNameW(nullptr, executable, 32768);
    const std::wstring path(executable);
    profilePath = path.substr(0, path.find_last_of(L"\\/")) + L"\\level1-progress.dat";
    int record[10] = {};
    std::ifstream input(profilePath.c_str(), std::ios::binary);
    if (!input)
    {
        return;
    }
    if (!input.read(reinterpret_cast<char *>(record), sizeof(record)) || record[0] != 0x31504752 ||
        record[1] != 1 || record[2] < 1 || record[2] > 20)
    {
        Notice(L"저장 자료를 읽지 못해 새 성장 기록으로 시작합니다.");
        return;
    }
    Rpg::Stats loaded;
    loaded.level = record[2];
    if (record[3] < 0 || record[3] >= loaded.RequiredExperience() || record[4] < 0 ||
        record[4] > 100000000 || record[5] < 0 || record[5] > loaded.MaxHealth())
    {
        Notice(L"저장 자료의 능력치가 올바르지 않습니다.");
        return;
    }
    for (int i = 6; i < 10; ++i)
    {
        if (record[i] < 0 || record[i] > 1000000)
        {
            Notice(L"저장 자료의 아이템 수량이 올바르지 않습니다.");
            return;
        }
    }
    loaded.experience = record[3];
    loaded.totalExperience = record[4];
    loaded.health = (std::max)(1, record[5]);
    loaded.potions = record[6];
    loaded.crystals = record[7];
    loaded.coins = record[8];
    loaded.kills = record[9];
    stats = loaded;
}

void Spawn(Enemy &enemy)
{
    for (int attempt = 0; attempt < 100; ++attempt)
    {
        enemy.position = world.RandomGround(random, 7);
        if (Rpg::Distance(enemy.position, player) > 6)
        {
            break;
        }
    }
    enemy.maximumHealth = enemy.kind == 0 ? 45 : 80;
    enemy.health = enemy.maximumHealth;
    enemy.cooldown = 1;
    enemy.respawn = 0;
    enemy.hitFlash = 0;
}

void NewMap()
{
    const auto seed = static_cast<std::uint32_t>(
        std::chrono::high_resolution_clock::now().time_since_epoch().count());
    random.seed(seed);
    world.Generate(seed);
    player = {0, 0};
    camera = player;
    projectiles.clear();
    loot.clear();
    floating.clear();
    enemies.clear();
    for (int i = 0; i < 20; ++i)
    {
        Enemy enemy;
        enemy.kind = i % 3 == 0 ? 1 : 0;
        Spawn(enemy);
        enemies.push_back(enemy);
    }
    navigation = world.DistancesFrom(player);
    attackCooldown = 0;
    invulnerable = 2;
    confirmNewMap = false;
}

void Move(Point &position, Point direction, float speed, float dt)
{
    const float length = std::hypot(direction.x, direction.z);
    if (length < .001f)
    {
        return;
    }
    const float step = (std::min)(speed * dt, length);
    Point next = {position.x + direction.x / length * step, position.z};
    if (world.Walkable(next))
    {
        position = next;
    }
    next = {position.x, position.z + direction.z / length * step};
    if (world.Walkable(next))
    {
        position = next;
    }
}

bool ClearLine(Point from, Point target)
{
    const int steps = (std::max)(1, static_cast<int>(Rpg::Distance(from, target) / .2f));
    for (int step = 1; step <= steps; ++step)
    {
        const float fraction = float(step) / steps;
        if (!world.Walkable(
                {from.x + (target.x - from.x) * fraction, from.z + (target.z - from.z) * fraction},
                .05f))
        {
            return false;
        }
    }
    return true;
}

void Attack()
{
    if (attackCooldown > 0)
    {
        return;
    }
    Point direction = {std::sin(facing * .0174532925f), std::cos(facing * .0174532925f)};
    float nearest = 9;
    for (const Enemy &enemy : enemies)
    {
        const float distance = Rpg::Distance(player, enemy.position);
        if (enemy.health > 0 && distance < nearest && ClearLine(player, enemy.position))
        {
            nearest = distance;
            direction = {enemy.position.x - player.x, enemy.position.z - player.z};
        }
    }
    const float length = std::hypot(direction.x, direction.z);
    if (length < .001f)
    {
        direction = {0, 1};
    }
    else
    {
        direction.x /= length;
        direction.z /= length;
    }
    facing = std::atan2(direction.x, direction.z) * 57.2957795f;
    projectiles.push_back({player, direction, 1, stats.Attack()});
    attackCooldown = .38f;
}

void Defeat(Enemy &enemy)
{
    const int reward = enemy.kind == 0 ? 15 : 25;
    enemy.health = 0;
    enemy.respawn = 10;
    ++stats.kills;
    const int levels = stats.GainExperience(reward);
    floating.push_back({enemy.position, L"경험치 +" + std::to_wstring(reward), 1.5f});
    if (levels > 0)
    {
        Notice(L"레벨 " + std::to_wstring(stats.level) + L" 달성! 체력 +20 · 공격 +5 · 방어 +2");
    }
    if (loot.size() >= 120)
    {
        loot.erase(loot.begin());
    }
    loot.push_back({enemy.position, stats.kills % 3 == 0 ? 1 : 0});
    saveDirty = true;
}

void Collect()
{
    int collected = 0;
    for (auto item = loot.begin(); item != loot.end();)
    {
        if (Rpg::Distance(item->position, player) < 1.7f)
        {
            if (item->kind == 1)
            {
                ++stats.potions;
            }
            else
            {
                ++stats.crystals;
                stats.coins += 5;
            }
            ++collected;
            item = loot.erase(item);
        }
        else
        {
            ++item;
        }
    }
    if (collected)
    {
        Notice(L"전리품 " + std::to_wstring(collected) + L"개 획득! [I]로 가방 확인");
        saveDirty = true;
    }
    else if (Rpg::Distance(player, {0, 0}) < 3)
    {
        stats.health = stats.MaxHealth();
        Notice(L"야영지에서 체력을 회복했습니다.");
        saveDirty = true;
    }
}

void UpdateProjectiles(float dt)
{
    for (Projectile &shot : projectiles)
    {
        const int steps = (std::max)(1, static_cast<int>(std::ceil(dt * 11 / .15f)));
        for (int step = 0; step < steps && shot.life > 0; ++step)
        {
            shot.position.x += shot.direction.x * 11 * dt / steps;
            shot.position.z += shot.direction.z * 11 * dt / steps;
            if (!world.Walkable(shot.position, .06f))
            {
                shot.life = 0;
                break;
            }
            for (Enemy &enemy : enemies)
            {
                if (enemy.health > 0 && Rpg::Distance(shot.position, enemy.position) < .55f)
                {
                    enemy.health -= shot.damage;
                    enemy.hitFlash = .16f;
                    floating.push_back({enemy.position, std::to_wstring(shot.damage), .8f});
                    shot.life = 0;
                    if (enemy.health <= 0)
                    {
                        Defeat(enemy);
                    }
                    break;
                }
            }
        }
        shot.life -= dt;
    }
    projectiles.erase(std::remove_if(projectiles.begin(),
                                     projectiles.end(),
                                     [](const Projectile &shot)
                                     {
                                         return shot.life <= 0;
                                     }),
                      projectiles.end());
}

void UpdateEnemies(float dt)
{
    pathTimer -= dt;
    if (pathTimer <= 0)
    {
        navigation = world.DistancesFrom(player);
        pathTimer = .25f;
    }
    for (Enemy &enemy : enemies)
    {
        enemy.hitFlash = (std::max)(0.f, enemy.hitFlash - dt);
        if (enemy.health <= 0)
        {
            enemy.respawn -= dt;
            if (enemy.respawn <= 0)
            {
                Spawn(enemy);
            }
            continue;
        }
        enemy.cooldown -= dt;
        const float distance = Rpg::Distance(player, enemy.position);
        if (distance > 10 || Rpg::Distance(player, {0, 0}) < 3)
        {
            continue;
        }
        if (distance > 1)
        {
            Point target = world.StepToward(enemy.position, navigation);
            if (world.Index(enemy.position) == world.Index(player))
            {
                target = player;
            }
            Point direction = {target.x - enemy.position.x, target.z - enemy.position.z};
            enemy.heading = std::atan2(direction.x, direction.z) * 57.2957795f;
            Move(enemy.position, direction, enemy.kind == 0 ? 1.35f : 1.9f, dt);
        }
        else if (enemy.cooldown <= 0 && invulnerable <= 0)
        {
            const int damage = (std::max)(1, (enemy.kind == 0 ? 8 : 13) - stats.Defense());
            stats.health -= damage;
            invulnerable = .65f;
            enemy.cooldown = 1.2f;
            saveDirty = true;
            if (stats.health <= 0)
            {
                player = {0, 0};
                camera = player;
                stats.health = stats.MaxHealth();
                invulnerable = 3;
                projectiles.clear();
                Notice(L"야영지로 돌아왔습니다. 경험치와 아이템은 유지됩니다.");
                break;
            }
        }
    }
}

void DrawModel(Model model,
               Point position,
               float y = 0,
               float heading = 0,
               float sx = 1,
               float sy = 1,
               float sz = 1)
{
    glPushMatrix();
    glTranslatef(position.x, y, position.z);
    glRotatef(heading, 0, 1, 0);
    glScalef(sx, sy, sz);
    models->Draw(model);
    glPopMatrix();
}

bool Visible(Point position)
{
    return std::fabs(position.x - camera.x) < 28 && std::fabs(position.z - camera.z) < 28;
}

void Actors()
{
    for (int z = 0; z < Rpg::World::Size; ++z)
    {
        for (int x = 0; x < Rpg::World::Size; ++x)
        {
            const Point position = world.Center(x, z);
            if (!Visible(position))
            {
                continue;
            }
            if (world.At(x, z) == Rpg::Tile::Tree)
            {
                DrawModel(Model::Tree, position);
            }
            else if (world.At(x, z) == Rpg::Tile::Rock)
            {
                DrawModel(Model::Rock, position);
            }
        }
    }
    DrawModel(Model::Camp, {0, 0});
    const int frame = moving ? static_cast<int>(walkTime * 10) % 8 : 0;
    renderer->MaterialMode(-1, invulnerable > 0 ? .35f : 0);
    DrawModel(static_cast<Model>(frame), player, 0, facing);
    renderer->MaterialMode();
    for (const Enemy &enemy : enemies)
    {
        if (enemy.health <= 0 || !Visible(enemy.position))
        {
            continue;
        }
        renderer->MaterialMode(-1, enemy.hitFlash > 0 ? 1.f : 0);
        // Transform only: the model vertices themselves stay cached.
        const float squash = enemy.kind == 0 ? 1 + std::sin(time * 5) * .08f : 1;
        DrawModel(enemy.kind == 0 ? Model::Slime : Model::Boar,
                  enemy.position,
                  0,
                  enemy.heading,
                  1,
                  squash,
                  1);
    }
    renderer->MaterialMode();
}

void TerrainAndEffects()
{
    // A repeated material covers the whole map with one cached ground mesh.
    renderer->MaterialMode(Renderer::Grass, 0, 30);
    DrawModel(Model::Ground,
              {-.75f, -.75f},
              -.06f,
              0,
              Rpg::World::Size * Rpg::World::CellSize,
              1,
              Rpg::World::Size * Rpg::World::CellSize);
    renderer->MaterialMode(Renderer::Stone);
    DrawModel(Model::Ground, {0, 0}, -.03f, 0, 5, 1, 5);
    renderer->MaterialMode();
    renderer->EffectMode(1, time);
    for (int z = 0; z < Rpg::World::Size; ++z)
    {
        for (int x = 0; x < Rpg::World::Size; ++x)
        {
            const Point position = world.Center(x, z);
            if (world.At(x, z) == Rpg::Tile::Water && Visible(position))
            {
                DrawModel(Model::Water,
                          position,
                          .025f,
                          0,
                          Rpg::World::CellSize,
                          1,
                          Rpg::World::CellSize);
            }
        }
    }
    renderer->EffectMode();
    renderer->MaterialMode(-1, .55f);
    for (const Loot &item : loot)
    {
        if (Visible(item.position))
        {
            DrawModel(item.kind == 0 ? Model::Crystal : Model::Potion,
                      item.position,
                      .12f + std::sin(time * 3) * .05f,
                      time * 45);
        }
    }
    renderer->MaterialMode(-1, 1.8f);
    for (const Projectile &shot : projectiles)
    {
        DrawModel(Model::Orb, shot.position, .7f);
    }
    renderer->MaterialMode();
    renderer->EffectMode(2, time);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);
    DrawModel(Model::Flame, {0, 0}, .18f, -45, .9f, 1.4f, 1);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    renderer->EffectMode();
}

void Panel(
    float x, float y, float width, float height, float r = .075f, float g = .13f, float b = .18f)
{
    glColor4f(r, g, b, .94f);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + width, y);
    glVertex2f(x + width, y + height);
    glVertex2f(x, y + height);
    glEnd();
}

void ScreenPosition(Point position, float &x, float &y)
{
    const float dx = position.x - camera.x, dz = position.z - camera.z;
    // Matches the orthographic camera; HUD uses a logical 1280x800 canvas.
    GLint viewport[4] = {};
    glGetIntegerv(GL_VIEWPORT, viewport);
    const float aspect = float(viewport[2]) / (std::max)(1, viewport[3]);
    x = 640 + (dx + dz) * .70710678f * 1280 / (22 * aspect);
    y = 400 - (dx - dz) * .408244f * 800 / 22;
}

void HUD()
{
    renderer->BeginUI();
    Panel(20, 18, 720, 172);
    renderer->Text(38, 48, L"레벨 1 · 반딧불 사냥터", 1, .86f, .55f);
    renderer->Text(38,
                   82,
                   L"캐릭터 레벨 " + std::to_wstring(stats.level) + L"   체력 " +
                       std::to_wstring(stats.health) + L" / " + std::to_wstring(stats.MaxHealth()));
    Panel(38, 94, 360, 12, .28f, .18f, .20f);
    Panel(38, 94, 360.f * stats.health / stats.MaxHealth(), 12, .72f, .3f, .32f);
    renderer->Text(38,
                   137,
                   stats.level == Rpg::Stats::MaximumLevel
                       ? L"최고 레벨 달성"
                       : L"경험치 " + std::to_wstring(stats.experience) + L" / " +
                             std::to_wstring(stats.RequiredExperience()));
    renderer->Text(38,
                   172,
                   L"공격 " + std::to_wstring(stats.Attack()) + L"   방어 " +
                       std::to_wstring(stats.Defense()) + L"   처치 " +
                       std::to_wstring(stats.kills) + L"   회복약 " +
                       std::to_wstring(stats.potions));
    Panel(20, 203, 720, 48);
    renderer->Text(38,
                   235,
                   stats.level >= 5
                       ? L"성장 목표 달성! 계속 사냥하며 더 강해질 수 있습니다."
                       : L"성장 목표: 캐릭터 레벨 5 달성 · 적을 처치하고 전리품을 모으세요.");
    Panel(1040, 18, 220, 240);
    renderer->Text(1054, 47, L"주변 지도", 1, .86f, .55f);
    for (int z = 0; z < Rpg::World::Size; ++z)
    {
        for (int x = 0; x < Rpg::World::Size; ++x)
        {
            const Rpg::Tile tile = world.At(x, z);
            glColor3f(tile == Rpg::Tile::Water ? .2f : .35f,
                      tile == Rpg::Tile::Ground ? .55f : .35f,
                      tile == Rpg::Tile::Water ? .75f : .25f);
            glBegin(GL_POINTS);
            glVertex2f(1052 + x * 5, 57 + z * 4.5f);
            glEnd();
        }
    }
    auto dot = [](Point p, float r, float g, float b)
    {
        glColor3f(r, g, b);
        glPointSize(5);
        glBegin(GL_POINTS);
        glVertex2f(1052 + (p.x / Rpg::World::CellSize + 20) * 5,
                   57 + (p.z / Rpg::World::CellSize + 20) * 4.5f);
        glEnd();
    };
    for (const Enemy &enemy : enemies)
        if (enemy.health > 0)
            dot(enemy.position, 1, .35f, .3f);
    dot({0, 0}, 1, .8f, .3f);
    dot(player, 1, 1, 1);
    renderer->Text(1054, 251, L"흰색: 나 / 빨강: 적");
    for (const Enemy &enemy : enemies)
    {
        if (enemy.health <= 0 || !Visible(enemy.position))
            continue;
        float x = 0, y = 0;
        ScreenPosition(enemy.position, x, y);
        if (x < 20 || x > 1260 || y < 270 || y > 695)
            continue;
        Panel(x - 20, y - 37, 40, 5, .3f, .15f, .17f);
        Panel(x - 20, y - 37, 40.f * enemy.health / enemy.maximumHealth, 5, .8f, .34f, .28f);
    }
    for (const FloatingText &text : floating)
    {
        float x = 0, y = 0;
        ScreenPosition(text.position, x, y);
        if (x > 0 && x < 1100 && y > 260 && y < 730)
            renderer->Text(x - 15, y - 55 - (1.5f - text.remaining) * 15, text.text, 1, .88f, .42f);
    }
    if (messageTimer > 0)
    {
        Panel(20, 670, 1000, 44);
        renderer->Text(38, 700, message, 1, .85f, .52f);
    }
    Panel(20, 738, 1240, 44);
    renderer->Text(
        35,
        769,
        L"WASD 이동 | 스페이스·좌클릭 공격 | E 줍기·휴식 | Q 회복약 | I 가방 | N 새 맵 | ESC 정지");
    if (inventoryOpen)
    {
        Panel(350, 280, 580, 260);
        renderer->Text(380, 320, L"가방과 성장 기록", 1, .85f, .5f);
        renderer->Text(380,
                       360,
                       L"마력 결정 " + std::to_wstring(stats.crystals) + L"개 · 금화 " +
                           std::to_wstring(stats.coins));
        renderer->Text(
            380, 400, L"회복약 " + std::to_wstring(stats.potions) + L"개 · Q 사용 시 체력 50 회복");
        renderer->Text(380, 440, L"누적 경험치 " + std::to_wstring(stats.totalExperience));
        renderer->Text(380, 485, L"레벨당 최대 체력 +20 / 공격 +5 / 방어 +2");
        renderer->Text(380, 525, L"[I] 닫기   [P] 성장 기록 저장");
    }
    if (paused)
    {
        Panel(350, 300, 580, 150);
        renderer->Text(385, 345, L"일시정지");
        renderer->Text(385, 390, L"ESC 계속하기 · P 성장 기록 저장");
    }
    if (confirmNewMap)
    {
        Panel(250, 300, 780, 180);
        renderer->Text(280, 345, L"새 사냥터를 생성할까요?", 1, .86f, .5f);
        renderer->Text(
            280, 390, L"성장·가방은 유지됩니다. 바닥의 전리품과 적 배치는 초기화됩니다.");
        renderer->Text(280, 440, L"[N] 생성하기   [ESC] 취소");
    }
}
}

bool Initialize(int width, int height)
{
    renderer.reset(new Renderer(width, height));
    if (!renderer->IsInitialized())
        return false;
    models.reset(new ModelCache());
    if (!models->Initialize())
        return false;
    LoadProfile();
    NewMap();
    Notice(models->Status());
    running = true;
    lastTime = glutGet(GLUT_ELAPSED_TIME);
    return true;
}

void Shutdown()
{
    running = false;
    if (!profilePath.empty() && saveDirty)
        SaveProfile();
    models.reset();
    renderer.reset();
}

void Render()
{
    if (!running || !renderer || !renderer->IsInitialized())
        return;
    renderer->BeginShadow(camera.x, camera.z);
    Actors();
    renderer->BeginScene(camera.x, camera.z);
    renderer->EffectMode();
    Actors();
    TerrainAndEffects();
    renderer->EndScene();
    HUD();
    glutSwapBuffers();
}

void Resize(int width, int height)
{
    if (renderer)
        renderer->Resize(width, height);
}

void KeyDown(unsigned char key, int, int)
{
    if (key >= 'A' && key <= 'Z')
        key = static_cast<unsigned char>(key - 'A' + 'a');
    if (key == 27)
    {
        if (confirmNewMap)
            confirmNewMap = false;
        else
            paused = !paused;
        std::fill(keys, keys + 256, false);
        mouseAttack = false;
        return;
    }
    if (key == 'p')
    {
        Notice(SaveProfile() ? L"성장 기록을 저장했습니다."
                             : L"저장에 실패했습니다. 폴더 권한을 확인하세요.");
        return;
    }
    if (paused)
        return;
    if (key == 'n')
    {
        if (confirmNewMap)
        {
            NewMap();
            Notice(L"새 사냥터가 생성되었습니다. 성장 기록은 유지됩니다.");
        }
        else
            confirmNewMap = true;
        std::fill(keys, keys + 256, false);
        mouseAttack = false;
        return;
    }
    if (confirmNewMap)
        return;
    if (key == 'i')
    {
        inventoryOpen = !inventoryOpen;
        std::fill(keys, keys + 256, false);
        mouseAttack = false;
        return;
    }
    if (inventoryOpen || keys[key])
        return;
    keys[key] = true;
    if (key == 'e')
        Collect();
    if (key == 'q')
    {
        if (stats.DrinkPotion())
        {
            Notice(L"회복약을 사용했습니다.");
            saveDirty = true;
        }
        else
            Notice(L"회복약이 없거나 체력이 가득 찼습니다.");
    }
}

void KeyUp(unsigned char key, int, int)
{
    if (key >= 'A' && key <= 'Z')
        key = static_cast<unsigned char>(key - 'A' + 'a');
    keys[key] = false;
}

void Mouse(int button, int state, int, int)
{
    if (button == GLUT_LEFT_BUTTON)
        mouseAttack = state == GLUT_DOWN && !paused && !inventoryOpen && !confirmNewMap;
}

void Visibility(int state)
{
    if (state != GLUT_VISIBLE)
    {
        paused = true;
        std::fill(keys, keys + 256, false);
        mouseAttack = false;
    }
}

void Tick(int)
{
    if (!running || glutGetWindow() == 0)
        return;
    const int now = glutGet(GLUT_ELAPSED_TIME);
    const float dt = (std::min)((now - lastTime) / 1000.f, .05f);
    lastTime = now;
    if (!paused && !inventoryOpen && !confirmNewMap)
    {
        time += dt;
        messageTimer -= dt;
        invulnerable = (std::max)(0.f, invulnerable - dt);
        attackCooldown = (std::max)(0.f, attackCooldown - dt);
        saveTimer += dt;
        const float sx = float(keys['d']) - float(keys['a']),
                    sy = float(keys['w']) - float(keys['s']);
        Point direction = {(sx + sy) * .70710678f, (sx - sy) * .70710678f};
        moving = std::hypot(direction.x, direction.z) > .01f;
        if (moving)
        {
            Move(player, direction, 3.6f, dt);
            walkTime += dt;
            facing = std::atan2(direction.x, direction.z) * 57.2957795f;
        }
        if (keys[' '] || mouseAttack)
            Attack();
        UpdateProjectiles(dt);
        UpdateEnemies(dt);
        const float blend = 1 - std::exp(-6 * dt);
        camera.x += (player.x - camera.x) * blend;
        camera.z += (player.z - camera.z) * blend;
        for (FloatingText &text : floating)
            text.remaining -= dt;
        floating.erase(std::remove_if(floating.begin(),
                                      floating.end(),
                                      [](const FloatingText &text)
                                      {
                                          return text.remaining <= 0;
                                      }),
                       floating.end());
        if (saveTimer >= 15)
        {
            saveTimer = 0;
            if (saveDirty && !SaveProfile())
                Notice(L"자동 저장 실패: 실행 폴더의 쓰기 권한을 확인하세요.");
        }
    }
    glutPostRedisplay();
    glutTimerFunc(16, Tick, 0);
}
}
