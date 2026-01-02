
#include "JobSystem.h"

#include "LinearAllocator.h"

#include <cassert>
#include <iostream>

// Define the thread-local static member declared in JobSystem.h
thread_local int JobSystem::s_workerIndex = -1;

constexpr size_t SCRATCH_SIZE = 16 * 1024 * 1024; // 16MB
std::vector<std::unique_ptr<LinearAllocator>> m_scratchAllocators;

JobSystem::JobSystem()
{
	unsigned int threadCount = std::thread::hardware_concurrency();
	if (threadCount == 0) threadCount = 4;

	unsigned int workerCount = threadCount - 1;
	m_workers.reserve(workerCount);
	m_scratchAllocators.reserve(workerCount);

	size_t perThreadScratch = SCRATCH_SIZE / workerCount;

	for (unsigned int i = 0; i < workerCount; ++i) {
		m_scratchAllocators.emplace_back(
			std::make_unique<LinearAllocator>(perThreadScratch)
		);

		m_workers.emplace_back([this, i] {
			JobSystem::s_workerIndex = i;
			WorkerLoop();
			});
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

LinearAllocator& JobSystem::GetScratchAllocator()
{
	assert(s_workerIndex >= 0 && "GetScratchAllocator() called from a non-worker thread");
	return *m_scratchAllocators[JobSystem::s_workerIndex];
}

bool JobSystem::IsWorkerThread() const
{
	return s_workerIndex >= 0;
}

void JobSystem::ParallelFor(size_t count, std::function<void(size_t)> func, size_t batchSize) {
	if (count == 0)
		return;

	size_t jobCount = (count + batchSize - 1) / batchSize;

	for (size_t jobIndex = 0; jobIndex < jobCount; ++jobIndex) {
		Submit([=] {
			size_t start = jobIndex * batchSize;
			size_t end = std::min(start + batchSize, count);

			for (size_t i = start; i < end; ++i) {
				func(i);
			}
			});
	}

	Wait();
}

JobHandle JobSystem::Submit(std::function<void()> job) {
	JobHandle handle;
	handle.counter = std::make_shared<std::atomic<int>>(1);

	{
		std::lock_guard<std::mutex> lock(m_queueMutex);
		m_jobQueue.push([job, handle] {
			job();
			(*handle.counter)--;
			});
		m_jobsInFlight++;
	}

	m_condition.notify_one();
	return handle;
}

JobHandle JobSystem::SubmitAfter(const JobHandle& dependency, std::function<void()> job)
{
	return Submit([=]
		{
			Wait(dependency);
			job();
		});
}

void JobSystem::Wait() {
	while (m_jobsInFlight > 0) {
		std::this_thread::yield();
	}
}

void JobSystem::Wait(const JobHandle& handle)
{
	while (handle.counter->load() > 0) {
		std::this_thread::yield();
	}
}

void JobSystem::Wait(const std::vector<JobHandle>& handles)
{
	for (const auto& handle : handles) {
		Wait(handle);
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

		m_scratchAllocators[s_workerIndex]->Reset();
		m_jobsInFlight--;
	}
}

