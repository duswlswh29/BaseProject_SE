#pragma once
#include "Renderer.h"
#include "BackgroundWriter.h"
#include <locale>
#include <array>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>
#include <string>

namespace Profiling
{
using Clock = std::chrono::steady_clock;

struct Metric
{
    double total = 0;
    double maximum = 0;
    unsigned calls = 0;
};

struct Query
{
    GLuint ids[2] = {};
    bool pending = false;
    unsigned long long frame = 0;
    std::string name;
};

struct State
{
    std::map<std::string, Metric> cpu;
    std::map<std::string, double> counters;
    std::array<Query, 64> queries;
    unsigned long long frame = 0;
    Clock::time_point previous = Clock::now();
    unsigned long long drawCalls = 0;
    Clock::time_point report = previous;
    double intervalTotal = 0;
    double intervalMax = 0;
    unsigned intervalCount = 0;
    std::string buffer;
    std::wstring prefix;
    unsigned part = 0;
    std::size_t bytes = 0;
    std::ofstream file;
    std::atomic<bool> failed{false};
    BackgroundWriter writer;
    bool gpuSupported = false;
    bool initialized = false;
};

inline State &Get()
{
    static State state;
    return state;
}

inline void Count(const std::string &name, double value = 1)
{
    Get().counters[name] += value;
}

inline void Initialize()
{
    auto &s = Get();
    if (s.initialized)
        return;
    s.initialized = true;
    s.gpuSupported = GLEW_VERSION_3_3 || GLEW_ARB_timer_query;
    wchar_t path[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    std::wstring directory(path);
    directory = directory.substr(0, directory.find_last_of(L"\\/") + 1);
    s.prefix = directory + L"profile-" + std::to_wstring(GetCurrentProcessId()) + L"-" +
               std::to_wstring(GetTickCount64()) + L"-";
    auto jsonString = [](const GLubyte *value)
    {
        std::string result;
        if (value)
            for (const unsigned char *p = value; *p; ++p)
            {
                if (*p == '"' || *p == '\\')
                    result += '\\';
                if (*p >= 32)
                    result += static_cast<char>(*p);
            }
        return result;
    };
#ifdef _DEBUG
    const char *configuration = "debug";
#else
    const char *configuration = "release";
#endif
    s.buffer += std::string("{\"schema\":1,\"event\":\"session\",\"build\":\"") + configuration +
                "\",\"gpu_renderer\":\"" + jsonString(glGetString(GL_RENDERER)) +
                "\",\"gl_version\":\"" + jsonString(glGetString(GL_VERSION)) +
                "\",\"render_scheduler\":\"idle\",\"update_timer_ms\":16,\"gpu_timer_supported\":" +
                (s.gpuSupported ? "true" : "false") + "}\n";
    std::printf("[Profiler] JSONL files are saved beside the executable. GPU timers: %s\n",
                s.gpuSupported ? "enabled" : "unavailable");
}

inline void Flush()
{
    auto &s = Get();
    if (s.buffer.empty())
        return;
    if (s.failed)
    {
        s.buffer.clear();
        return;
    }
    std::string batch;
    batch.swap(s.buffer);
    s.writer.Submit(
        [&s, batch = std::move(batch)]
        {
            if (s.failed)
                return;
            if (!s.file.is_open() || s.bytes >= 32 * 1024 * 1024)
            {
                s.file.close();
                const std::wstring path = s.prefix + std::to_wstring(s.part++) + L".jsonl";
                s.file.open(path.c_str(), std::ios::out | std::ios::binary);
                s.bytes = 0;
            }
            s.file << batch;
            s.file.flush();
            s.bytes += batch.size();
            if (!s.file)
            {
                s.failed = true;
                std::fprintf(stderr, "[Profiler] JSONL write failed.\n");
            }
        });
}

inline void PollGpu()
{
    auto &s = Get();
    for (auto &q : s.queries)
    {
        if (!q.pending)
            continue;
        GLint available = GL_FALSE;
        glGetQueryObjectiv(q.ids[1], GL_QUERY_RESULT_AVAILABLE, &available);
        if (!available)
            continue;
        GLuint64 start = 0, end = 0;
        glGetQueryObjectui64v(q.ids[0], GL_QUERY_RESULT, &start);
        glGetQueryObjectui64v(q.ids[1], GL_QUERY_RESULT, &end);
        std::ostringstream row;
        row.imbue(std::locale::classic());
        row << std::fixed << std::setprecision(4)
            << "{\"schema\":1,\"event\":\"gpu_pass\",\"frame\":" << q.frame << ",\"stage\":\""
            << q.name << "\",\"gpu_ms\":" << double(end - start) / 1000000.0 << "}\n";
        if (!s.failed)
            s.buffer += row.str();
        q.pending = false;
    }
}

class Scope
{
  public:
    explicit Scope(const char *label, bool gpu = false) : name(label), start(Clock::now())
    {
        auto &s = Get();
        trackDraws = gpu;
        firstDraw = s.drawCalls;
        if (gpu && s.gpuSupported)
        {
            for (auto &candidate : s.queries)
            {
                if (!candidate.pending)
                {
                    query = &candidate;
                    if (!query->ids[0])
                        glGenQueries(2, query->ids);
                    query->pending = true;
                    query->frame = s.frame;
                    query->name = label;
                    glQueryCounter(query->ids[0], GL_TIMESTAMP);
                    break;
                }
            }
            if (!query)
                Count("gpu_query_samples_skipped");
        }
    }

    ~Scope()
    {
        if (trackDraws)
            Count(std::string(name) + ".draw_calls", double(Get().drawCalls - firstDraw));
        if (query)
            glQueryCounter(query->ids[1], GL_TIMESTAMP);
        const double ms = std::chrono::duration<double, std::milli>(Clock::now() - start).count();
        auto &metric = Get().cpu[name];
        metric.total += ms;
        metric.maximum = (std::max)(metric.maximum, ms);
        ++metric.calls;
    }

    Scope(const Scope &) = delete;
    Scope &operator=(const Scope &) = delete;

  private:
    const char *name;
    Clock::time_point start;
    Query *query = nullptr;
    bool trackDraws = false;
    unsigned long long firstDraw = 0;
};

inline void BeginFrame()
{
    Initialize();
    ++Get().frame;
    Scope scope("profiler.gpu_poll");
    PollGpu();
}

inline void EndFrame()
{
    Scope serialization("profiler.frame_serialization");
    auto &s = Get();
    const auto now = Clock::now();
    const double ms = std::chrono::duration<double, std::milli>(now - s.previous).count();
    s.previous = now;
    const bool valid = s.frame > 1;
    Count("profiler.writer_queue_depth", s.writer.queued.load());
    Count("profiler.dropped_batches_total", s.writer.dropped.load());
    Count("profiler.writer_failures_total", s.writer.failures.load());
    Count("profiler.writer_last_ms", s.writer.lastWriteMs.load());
    Count("profiler.file_failed", s.failed ? 1 : 0);
    std::ostringstream row;
    row << std::fixed << std::setprecision(4)
        << "{\"schema\":1,\"event\":\"frame\",\"frame\":" << s.frame
        << ",\"frame_interval_ms\":" << (valid ? std::to_string(ms) : "null")
        << ",\"fps\":" << (valid && ms > 0 ? std::to_string(1000.0 / ms) : "null")
        << ",\"cpu_scopes\":{";
    bool first = true;
    for (const auto &entry : s.cpu)
    {
        if (!first)
            row << ',';
        first = false;
        row << '"' << entry.first << "\":{\"total_ms\":" << entry.second.total
            << ",\"max_ms\":" << entry.second.maximum << ",\"calls\":" << entry.second.calls << '}';
    }
    row << "},\"counters\":{";
    first = true;
    for (const auto &entry : s.counters)
    {
        if (!first)
            row << ',';
        first = false;
        row << '"' << entry.first << "\":" << entry.second;
    }
    row << "}}\n";
    if (!s.failed)
        s.buffer += row.str();
    s.cpu.clear();
    s.counters.clear();
    if (valid)
    {
        s.intervalTotal += ms;
        s.intervalMax = (std::max)(s.intervalMax, ms);
        ++s.intervalCount;
    }
    if (std::chrono::duration<double>(now - s.report).count() >= 1.0)
    {
        Scope logging("profiler.log_flush");
        const auto frame = s.frame;
        const double fps = s.intervalTotal > 0 ? s.intervalCount * 1000.0 / s.intervalTotal : 0;
        const double maximum = s.intervalMax;
        s.writer.Submit(
            [frame, fps, maximum]
            {
                std::printf(
                    "[Profile] frame=%llu fps_avg=%.1f frame_ms_max=%.2f\n", frame, fps, maximum);
            });
        Flush();
        s.report = now;
        s.intervalTotal = s.intervalMax = 0;
        s.intervalCount = 0;
    }
}

inline void Shutdown()
{
    auto &s = Get();
    if (!s.initialized)
        return;
    PollGpu();
    for (auto &q : s.queries)
    {
        if (q.pending)
            s.buffer += "{\"schema\":1,\"event\":\"gpu_sample_discarded\",\"frame\":" +
                        std::to_string(q.frame) + "}\n";
        if (q.ids[0])
            glDeleteQueries(2, q.ids);
        q = {};
    }
    Flush();
    s.writer.Stop();
    s.file.close();
}
}
