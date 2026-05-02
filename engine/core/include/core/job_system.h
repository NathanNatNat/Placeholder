#pragma once

#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace placeholder::core
{

/// A job is a callable unit of work with no arguments.
using Job = std::function<void()>;

/// Thread-safe work-stealing deque.
/// The owning thread pushes/pops from the back; thieves steal from the front.
class WorkStealingQueue
{
public:
    void push(Job job);
    bool pop(Job& job);
    bool steal(Job& job);
    bool empty() const;

private:
    mutable std::mutex m_mutex;
    std::deque<Job> m_jobs;
};

/// Thread pool with work-stealing for parallel task execution.
class JobSystem
{
public:
    /// Initialize with the given number of worker threads.
    /// Pass 0 to use (hardware_concurrency - 1) threads.
    explicit JobSystem(uint32_t threadCount = 0);
    ~JobSystem();

    JobSystem(const JobSystem&) = delete;
    JobSystem& operator=(const JobSystem&) = delete;

    /// Schedule a job for execution on any worker thread.
    void schedule(Job job);

    /// Schedule a job and return a future for its result.
    template<typename F>
    auto scheduleWithResult(F&& func) -> std::future<decltype(func())>
    {
        using ReturnType = decltype(func());
        auto task = std::make_shared<std::packaged_task<ReturnType()>>(std::forward<F>(func));
        auto future = task->get_future();
        schedule([task]() { (*task)(); });
        return future;
    }

    /// Block until all currently scheduled jobs have completed.
    void waitAll();

    /// Get the number of worker threads.
    uint32_t threadCount() const { return static_cast<uint32_t>(m_workers.size()); }

    /// Spawn a dedicated thread that is not part of the pool.
    /// Returns the thread — caller is responsible for joining.
    static std::thread spawnDedicated(Job job);

private:
    void workerLoop(uint32_t workerIndex);
    bool tryRunJob(uint32_t workerIndex);

    std::vector<std::thread> m_workers;
    std::vector<std::unique_ptr<WorkStealingQueue>> m_queues;

    std::atomic<uint32_t> m_nextQueue{0};
    std::atomic<int64_t> m_pendingJobs{0};

    std::mutex m_wakeMutex;
    std::condition_variable m_wakeCondition;

    std::mutex m_completeMutex;
    std::condition_variable m_completeCondition;

    std::atomic<bool> m_running{true};
};

} // namespace placeholder::core
