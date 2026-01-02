
#include "JobSystem.h"

#include <iostream>

JobSystem::JobSystem() 
{
    unsigned int threadCount = std::thread::hardware_concurrency();
    if (threadCount == 0) threadCount = 4; // fallback

    unsigned int workerCount = threadCount - 1;

    std::cout << "Starting JobSystem with " << workerCount << " worker threads\n";

    m_workers.reserve(workerCount);

    for (unsigned int i = 0; i < workerCount; ++i) {
        m_workers.emplace_back(&JobSystem::WorkerLoop, this);
    }
}

JobSystem::~JobSystem() 
{
    m_running = false;

    // Wake all threads (we’ll add the condition variable later)
    // For now, just join them.
    for (auto& worker : m_workers) {
        if (worker.joinable())
            worker.join();
    }
}

void JobSystem::Submit(std::function<void()> job) 
{
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_jobQueue.push(std::move(job));
        m_jobsInFlight++;
    }

    m_condition.notify_one();
}

void JobSystem::Wait() {
    while (m_jobsInFlight > 0) {
        std::this_thread::yield();
    }
}

void JobSystem::WorkerLoop() 
{
    while (m_running) 
    {
        
        std::function<void()> job;

        {
            std::unique_lock<std::mutex> lock(m_queueMutex);

            // Sleep until there is a job or shutdown
            m_condition.wait(lock, [&] {
                return !m_jobQueue.empty() || !m_running;
            });

            if (!m_running)
                return;

            job = std::move(m_jobQueue.front());
            m_jobQueue.pop();
        }

        // Execute job outside the lock
        job();

        m_jobsInFlight--;
    }
}

