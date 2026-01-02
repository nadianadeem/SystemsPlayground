
#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <type_traits>
#include <mutex>
#include <thread>
#include <queue>
#include <vector>

class JobSystem {
public:
    JobSystem();
    ~JobSystem();

    void ParallelFor(size_t count, std::function<void(size_t)> func, size_t batchSize = 1);

    template<typename T, typename Func>
    void ParallelForEach(std::vector<T>& vec, Func func, size_t batchSize = 1) 
    {
        static_assert(std::is_invocable_v<decltype(func), T&>, "Func must be callable with a T& parameter");
        ParallelFor(vec.size(), [&](size_t i) 
        {
            func(vec[i]);
        }, batchSize);
    }



    void Submit(std::function<void()> job);
    void Wait();

private:
    void WorkerLoop();

    std::vector<std::thread> m_workers;
    std::atomic<bool> m_running = true;

    std::queue<std::function<void()>> m_jobQueue;
    std::mutex m_queueMutex;
    std::condition_variable m_condition;

    std::atomic<int> m_jobsInFlight = 0;
};

