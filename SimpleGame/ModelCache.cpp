#include "stdafx.h"
#include "Profiler.h"
#include "ModelCache.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fstream>

namespace
{
constexpr std::uint32_t CacheMagic = 0x314D5052;
constexpr std::uint32_t CacheVersion = 1;

std::uint32_t Checksum(const void *data, std::size_t length)
{
    const auto *bytes = static_cast<const unsigned char *>(data);
    std::uint32_t hash = 2166136261u;
    for (std::size_t i = 0; i < length; ++i)
    {
        hash = (hash ^ bytes[i]) * 16777619u;
    }
    return hash;
}
}

ModelCache::~ModelCache()
{
    for (auto &batch : batches)
        if (batch.mesh.buffer)
            glDeleteBuffers(1, &batch.mesh.buffer);
    for (Mesh &mesh : meshes)
    {
        if (mesh.buffer)
        {
            glDeleteBuffers(1, &mesh.buffer);
        }
    }
}

bool ModelCache::Initialize()
{
    Profiling::Scope profile("assets.initialize");
    wchar_t executable[32768] = {};
    const DWORD count = GetModuleFileNameW(nullptr, executable, 32768);
    if (!count || count >= 32768)
    {
        status = L"모델 경로를 확인하지 못했습니다.";
        return false;
    }
    const std::wstring fullPath(executable);
    const std::wstring directory = fullPath.substr(0, fullPath.find_last_of(L"\\/")) + L"\\Cache";
    const std::wstring path = directory + L"\\level1-models.bin";
    if (Load(path))
    {
        status = L"모델 캐시 불러오기 완료";
    }
    else
    {
        Generate();
        CreateDirectoryW(directory.c_str(), nullptr);
        status = Save(path) ? L"모델 생성·캐시 저장 완료" : L"모델 생성 완료 / 캐시 저장 실패";
    }
    for (Mesh &mesh : meshes)
    {
        mesh.count = static_cast<GLsizei>(mesh.vertices.size());
        mesh.bounds = {};
        for (const Vertex &vertex : mesh.vertices)
        {
            mesh.bounds.Include(
                Vector3{vertex.position[0], vertex.position[1], vertex.position[2]});
        }
        glGenBuffers(1, &mesh.buffer);
        glBindBuffer(GL_ARRAY_BUFFER, mesh.buffer);
        glBufferData(GL_ARRAY_BUFFER,
                     mesh.vertices.size() * sizeof(Vertex),
                     mesh.vertices.data(),
                     GL_STATIC_DRAW);
        GLint uploadedBytes = 0;
        glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &uploadedBytes);
        if (uploadedBytes != static_cast<GLint>(mesh.vertices.size() * sizeof(Vertex)))
        {
            status = L"모델을 그래픽 메모리에 올리지 못했습니다.";
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            return false;
        }
        // The disk file is the CPU source for subsequent executions.
        if (&mesh != &meshes[static_cast<int>(Model::Tree)] &&
            &mesh != &meshes[static_cast<int>(Model::Rock)])
            mesh.vertices.clear();
        mesh.vertices.shrink_to_fit();
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return true;
}

bool ModelCache::Load(const std::wstring &path)
{
    Profiling::Scope profile("io.model_cache_load");
    std::ifstream input(path.c_str(), std::ios::binary);
    std::uint32_t header[4] = {};
    if (!input.read(reinterpret_cast<char *>(header), sizeof(header)) || header[0] != CacheMagic ||
        header[1] != CacheVersion || header[2] != meshes.size() || header[3] != sizeof(Vertex))
    {
        return false;
    }
    for (Mesh &mesh : meshes)
    {
        std::uint32_t record[2] = {};
        if (!input.read(reinterpret_cast<char *>(record), sizeof(record)) || record[0] == 0 ||
            record[0] > 150000 || record[0] % 3 != 0)
        {
            return false;
        }
        mesh.vertices.resize(record[0]);
        const std::size_t bytes = mesh.vertices.size() * sizeof(Vertex);
        if (!input.read(reinterpret_cast<char *>(mesh.vertices.data()), bytes) ||
            Checksum(mesh.vertices.data(), bytes) != record[1])
        {
            return false;
        }
        for (const Vertex &vertex : mesh.vertices)
        {
            for (float component : vertex.position)
            {
                if (!std::isfinite(component) || std::fabs(component) > 10)
                {
                    return false;
                }
            }
        }
    }
    return input.peek() == std::char_traits<char>::eof();
}

bool ModelCache::Save(const std::wstring &path) const
{
    Profiling::Scope profile("io.model_cache_save");
    const std::wstring temporary = path + L".tmp";
    std::ofstream output(temporary.c_str(), std::ios::binary | std::ios::trunc);
    const std::uint32_t header[] = {
        CacheMagic, CacheVersion, static_cast<std::uint32_t>(meshes.size()), sizeof(Vertex)};
    output.write(reinterpret_cast<const char *>(header), sizeof(header));
    for (const Mesh &mesh : meshes)
    {
        const std::size_t bytes = mesh.vertices.size() * sizeof(Vertex);
        const std::uint32_t record[] = {static_cast<std::uint32_t>(mesh.vertices.size()),
                                        Checksum(mesh.vertices.data(), bytes)};
        output.write(reinterpret_cast<const char *>(record), sizeof(record));
        output.write(reinterpret_cast<const char *>(mesh.vertices.data()), bytes);
    }
    output.flush();
    const bool success = output.good();
    output.close();
    return success && MoveFileExW(temporary.c_str(),
                                  path.c_str(),
                                  MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
}

void ModelCache::Draw(Model model) const
{
    DrawMesh(meshes[static_cast<std::size_t>(model)]);
}

void ModelCache::DrawBatch(Model model, const std::vector<Vector3> &positions, ScenePass pass)
{
    Profiling::Scope profile("render.static_batch");
    Batch &batch = batches[(model == Model::Tree ? 0 : 2) + (pass == ScenePass::Shadow ? 1 : 0)];
    bool changed = positions.size() != batch.positions.size();
    if (!changed)
        for (std::size_t i = 0; i < positions.size(); ++i)
            if (positions[i].x != batch.positions[i].x || positions[i].y != batch.positions[i].y ||
                positions[i].z != batch.positions[i].z)
            {
                changed = true;
                break;
            }
    if (changed)
    {
        Profiling::Scope rebuild("render.static_batch_rebuild");
        const Mesh &source = meshes[static_cast<std::size_t>(model)];
        std::vector<Vertex> vertices;
        vertices.reserve(source.vertices.size() * positions.size());
        for (const auto &position : positions)
            for (Vertex vertex : source.vertices)
            {
                vertex.position[0] += position.x;
                vertex.position[1] += position.y;
                vertex.position[2] += position.z;
                vertices.push_back(vertex);
            }
        if (!batch.mesh.buffer)
            glGenBuffers(1, &batch.mesh.buffer);
        glBindBuffer(GL_ARRAY_BUFFER, batch.mesh.buffer);
        glBufferData(
            GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_DYNAMIC_DRAW);
        Profiling::Count("geometry.batch_upload_bytes", double(vertices.size() * sizeof(Vertex)));
        batch.mesh.count = static_cast<GLsizei>(vertices.size());
        batch.positions = positions;
        Profiling::Count("render.batch_cache_misses");
    }
    else
        Profiling::Count("render.batch_cache_hits");
    Profiling::Count("render.batched_actors", double(positions.size()));
    DrawMesh(batch.mesh);
}

void ModelCache::DrawMesh(const Mesh &mesh) const
{
    glBindBuffer(GL_ARRAY_BUFFER, mesh.buffer);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glClientActiveTexture(GL_TEXTURE0);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glVertexPointer(
        3, GL_FLOAT, sizeof(Vertex), reinterpret_cast<void *>(offsetof(Vertex, position)));
    glNormalPointer(GL_FLOAT, sizeof(Vertex), reinterpret_cast<void *>(offsetof(Vertex, normal)));
    glColorPointer(4, GL_FLOAT, sizeof(Vertex), reinterpret_cast<void *>(offsetof(Vertex, color)));
    glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex), reinterpret_cast<void *>(offsetof(Vertex, uv)));
    Renderer::CountedDrawArrays(GL_TRIANGLES, 0, mesh.count);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

const std::wstring &ModelCache::Status() const
{
    return status;
}

const ActorBounds &ModelCache::Bounds(Model model) const
{
    return meshes[static_cast<std::size_t>(model)].bounds;
}

void ModelCache::Box(
    Mesh &mesh, float x, float y, float z, float sx, float sy, float sz, float r, float g, float b)
{
    const float corners[8][3] = {{-1, -1, -1},
                                 {1, -1, -1},
                                 {1, 1, -1},
                                 {-1, 1, -1},
                                 {-1, -1, 1},
                                 {1, -1, 1},
                                 {1, 1, 1},
                                 {-1, 1, 1}};
    const int faces[6][4] = {
        {0, 3, 2, 1}, {4, 5, 6, 7}, {0, 4, 7, 3}, {1, 2, 6, 5}, {3, 7, 6, 2}, {0, 1, 5, 4}};
    const float normals[6][3] = {
        {0, 0, -1}, {0, 0, 1}, {-1, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, -1, 0}};
    const int triangles[6] = {0, 1, 2, 0, 2, 3};
    for (int face = 0; face < 6; ++face)
    {
        for (int index : triangles)
        {
            const auto &p = corners[faces[face][index]];
            Vertex vertex = {{x + p[0] * sx * .5f, y + p[1] * sy * .5f, z + p[2] * sz * .5f},
                             {normals[face][0], normals[face][1], normals[face][2]},
                             {r, g, b, 1},
                             {index == 1 || index == 2 ? 1.f : 0.f, index >= 2 ? 1.f : 0.f}};
            mesh.vertices.push_back(vertex);
        }
    }
}

void ModelCache::Ellipsoid(
    Mesh &mesh, float x, float y, float z, float sx, float sy, float sz, float r, float g, float b)
{
    auto vertex = [&](int u, int v)
    {
        const float longitude = u * 6.2831853f / 12;
        const float latitude = v * 3.14159265f / 8;
        const float px = std::sin(latitude) * std::cos(longitude);
        const float py = std::cos(latitude);
        const float pz = std::sin(latitude) * std::sin(longitude);
        const float nx = px / sx, ny = py / sy, nz = pz / sz;
        const float length = std::sqrt(nx * nx + ny * ny + nz * nz);
        return Vertex{{x + px * sx, y + py * sy, z + pz * sz},
                      {nx / length, ny / length, nz / length},
                      {r, g, b, 1},
                      {u / 12.f, v / 8.f}};
    };
    for (int v = 0; v < 8; ++v)
    {
        for (int u = 0; u < 12; ++u)
        {
            const Vertex a = vertex(u, v), b0 = vertex(u + 1, v), c = vertex(u + 1, v + 1),
                         d = vertex(u, v + 1);
            mesh.vertices.insert(mesh.vertices.end(), {a, b0, c, a, c, d});
        }
    }
}

void ModelCache::Grid(Mesh &mesh, int divisions, bool vertical)
{
    auto vertex = [&](int x, int z)
    {
        const float u = float(x) / divisions, v = float(z) / divisions;
        return Vertex{{u - .5f, vertical ? v : 0, vertical ? 0 : v - .5f},
                      {0, vertical ? 0.f : 1.f, vertical ? 1.f : 0.f},
                      {1, 1, 1, 1},
                      {u, v}};
    };
    for (int z = 0; z < divisions; ++z)
    {
        for (int x = 0; x < divisions; ++x)
        {
            const Vertex a = vertex(x, z), b = vertex(x + 1, z), c = vertex(x + 1, z + 1),
                         d = vertex(x, z + 1);
            mesh.vertices.insert(mesh.vertices.end(), {a, b, c, a, c, d});
        }
    }
}

void ModelCache::Generate()
{
    Profiling::Scope profile("assets.generate_meshes");
    for (Mesh &mesh : meshes)
    {
        mesh.vertices.clear();
    }
    for (int frame = 0; frame < 8; ++frame)
    {
        Mesh &hero = meshes[frame];
        const float swing = std::sin(frame * 6.2831853f / 8) * .15f;
        Box(hero, 0, .85f, 0, .52f, .62f, .34f, .43f, .32f, .65f);
        for (int side = -1; side <= 1; side += 2)
        {
            Box(hero, side * .15f, .31f, side * swing, .18f, .48f, .22f, .24f, .19f, .29f);
            Box(hero, side * .15f, .09f, .07f + side * swing, .22f, .16f, .32f, .25f, .15f, .10f);
            Box(hero, side * .34f, .91f, -side * swing, .18f, .4f, .20f, .50f, .38f, .70f);
            Ellipsoid(hero, side * .34f, .66f, -side * swing, .10f, .10f, .1f, .96f, .76f, .52f);
        }
        Ellipsoid(hero, 0, 1.37f, 0, .25f, .27f, .23f, .96f, .76f, .52f);
        Ellipsoid(hero, 0, 1.57f, 0, .37f, .07f, .32f, .32f, .25f, .54f);
        Ellipsoid(hero, 0, 1.77f, 0, .20f, .30f, .18f, .40f, .29f, .63f);
        for (int side = -1; side <= 1; side += 2)
        {
            Ellipsoid(hero, side * .09f, 1.40f, .21f, .033f, .04f, .03f, .1f, .12f, .17f);
        }
        Box(hero, .44f, .85f, .1f, .05f, .9f, .05f, .42f, .25f, .12f);
        Ellipsoid(hero, .44f, 1.34f, .1f, .10f, .1f, .1f, 1, .8f, .3f);
    }
    Mesh &slime = meshes[static_cast<int>(Model::Slime)];
    Ellipsoid(slime, 0, .40f, 0, .48f, .39f, .45f, .36f, .73f, .51f);
    for (int side = -1; side <= 1; side += 2)
    {
        Ellipsoid(slime, side * .14f, .51f, .38f, .06f, .08f, .035f, .12f, .21f, .17f);
    }
    Mesh &boar = meshes[static_cast<int>(Model::Boar)];
    Ellipsoid(boar, 0, .52f, 0, .42f, .36f, .61f, .57f, .37f, .23f);
    Ellipsoid(boar, 0, .57f, .55f, .27f, .25f, .25f, .48f, .29f, .19f);
    for (int side = -1; side <= 1; side += 2)
    {
        Box(boar, side * .26f, .20f, .3f, .13f, .35f, .13f, .31f, .21f, .15f);
        Box(boar, side * .26f, .20f, -.3f, .13f, .35f, .13f, .31f, .21f, .15f);
        Ellipsoid(boar, side * .21f, .47f, .71f, .05f, .12f, .06f, .94f, .87f, .65f);
    }
    Mesh &tree = meshes[static_cast<int>(Model::Tree)];
    Box(tree, 0, .7f, 0, .25f, 1.4f, .25f, .39f, .25f, .13f);
    Ellipsoid(tree, 0, 1.6f, 0, .65f, .85f, .65f, .25f, .49f, .28f);
    Ellipsoid(tree, 0, 2.25f, 0, .45f, .6f, .45f, .37f, .60f, .31f);
    Ellipsoid(
        meshes[static_cast<int>(Model::Rock)], 0, .35f, 0, .62f, .43f, .57f, .53f, .56f, .49f);
    Ellipsoid(
        meshes[static_cast<int>(Model::Crystal)], 0, .24f, 0, .14f, .27f, .14f, .50f, .84f, .96f);
    Mesh &potion = meshes[static_cast<int>(Model::Potion)];
    Ellipsoid(potion, 0, .20f, 0, .16f, .2f, .16f, .88f, .3f, .36f);
    Box(potion, 0, .4f, 0, .11f, .12f, .11f, .71f, .56f, .28f);
    Ellipsoid(meshes[static_cast<int>(Model::Orb)], 0, 0, 0, .13f, .13f, .13f, 1, .79f, .34f);
    Grid(meshes[static_cast<int>(Model::Ground)], 1, false);
    Grid(meshes[static_cast<int>(Model::Water)], 12, false);
    Grid(meshes[static_cast<int>(Model::Flame)], 8, true);
    Mesh &camp = meshes[static_cast<int>(Model::Camp)];
    Box(camp, 0, .1f, 0, .8f, .15f, .2f, .31f, .18f, .09f);
    Box(camp, 0, .2f, 0, .2f, .15f, .8f, .31f, .18f, .09f);
    for (int i = 0; i < 8; ++i)
    {
        const float a = i * 6.2831853f / 8;
        Ellipsoid(
            camp, std::cos(a) * .5f, .12f, std::sin(a) * .5f, .16f, .14f, .16f, .52f, .5f, .43f);
    }
}
