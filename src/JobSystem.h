
#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <queue>
#include <vector>

class JobSystem {
public:
    JobSystem();
    ~JobSystem();

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

