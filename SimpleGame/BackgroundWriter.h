#pragma once
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>

// Single bounded worker. Jobs must own their data and must never call OpenGL.
class BackgroundWriter
{
  public:
    BackgroundWriter()
        : worker(
              [this]
              {
                  Run();
              })
    {
    }

    ~BackgroundWriter()
    {
        Stop();
    }

    bool Submit(std::function<void()> job)
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (stopping || jobs.size() >= 4)
        {
            ++dropped;
            return false;
        }
        jobs.push_back(std::move(job));
        queued.store(static_cast<unsigned>(jobs.size()));
        ready.notify_one();
        return true;
    }

    void Stop()
    {
        {
            std::lock_guard<std::mutex> lock(mutex);
            stopping = true;
        }
        ready.notify_one();
        if (worker.joinable())
            worker.join();
    }

    std::atomic<unsigned> queued{0}, dropped{0}, failures{0};
    std::atomic<double> lastWriteMs{0};

  private:
    void Run()
    {
        for (;;)
        {
            std::function<void()> job;
            {
                std::unique_lock<std::mutex> lock(mutex);
                ready.wait(lock,
                           [this]
                           {
                               return stopping || !jobs.empty();
                           });
                if (jobs.empty())
                    return;
                job = std::move(jobs.front());
                jobs.pop_front();
                queued.store(static_cast<unsigned>(jobs.size()));
            }
            const auto start = std::chrono::steady_clock::now();
            try
            {
                job();
            }
            catch (...)
            {
                ++failures;
            }
            lastWriteMs.store(
                std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start)
                    .count());
        }
    }

    std::mutex mutex;
    std::condition_variable ready;
    std::deque<std::function<void()>> jobs;
    bool stopping = false;
    std::thread worker;
};
