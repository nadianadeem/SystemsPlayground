
#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <type_traits>
#include <mutex>
#include <thread>
#include <queue>
#include <vector>

class LinearAllocator;

//----------------- Job System ----------------//

struct JobHandle {
	std::shared_ptr<std::atomic<int>> counter;
};

class JobSystem {
public:
	JobSystem();
	~JobSystem();

	LinearAllocator& GetScratchAllocator();

	bool IsWorkerThread() const;

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

	JobHandle Submit(std::function<void()> job);
	JobHandle SubmitAfter(const JobHandle& dependency, std::function<void()> job);

	void Wait();
	void Wait(const JobHandle& handle);
	void Wait(const std::vector<JobHandle>& handles);

private:
	void WorkerLoop();

	std::vector<std::thread> m_workers;

	std::queue<std::function<void()>> m_jobQueue;
	std::mutex m_queueMutex;
	std::condition_variable m_condition;

	std::atomic<int> m_jobsInFlight = 0;
	thread_local static int s_workerIndex;
	std::atomic<bool> m_shutdown = false;

	std::vector<std::unique_ptr<LinearAllocator>> m_scratchAllocators;
};

