
#include "JobSystem.h"

#include "LinearAllocator.h"

#include <cassert>
#include <iostream>

// Define the thread-local static member declared in JobSystem.h
thread_local int JobSystem::s_workerIndex = -1;

constexpr size_t SCRATCH_SIZE = 16 * 1024 * 1024; // 16MB

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
	{
		std::unique_lock lock(m_queueMutex);
		m_shutdown = true;
	}

	m_condition.notify_all();

	for (auto& t : m_workers)
		t.join();
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

JobHandle JobSystem::Submit(std::function<void()> job) 
{
	JobHandle handle;
	handle.counter = std::make_shared<std::atomic<int>>(1);

	{
		std::lock_guard<std::mutex> lock(m_queueMutex);

		m_jobQueue.push([job, handle] {
			job();
			handle.counter->fetch_sub(1, std::memory_order_release);
			});

		m_jobsInFlight.fetch_add(1, std::memory_order_relaxed);
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
	while (handle.counter->load(std::memory_order_acquire) > 0) 
	{
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
	while (true)
	{
		std::function<void()> job;

		{
			std::unique_lock<std::mutex> lock(m_queueMutex);

			m_condition.wait(lock, [&] {
				return m_shutdown || !m_jobQueue.empty();
				});

			if (m_shutdown && m_jobQueue.empty())
				break;

			job = std::move(m_jobQueue.front());
			m_jobQueue.pop();
		}

		job();

		m_scratchAllocators[s_workerIndex]->Reset();
		m_jobsInFlight.fetch_sub(1, std::memory_order_release);
	}
}


