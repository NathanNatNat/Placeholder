#include "core/job_system.h"
#include "core/logging.h"

#include <algorithm>

namespace placeholder::core
{

// -- WorkStealingQueue --

void WorkStealingQueue::push(Job job)
{
    std::lock_guard lock(m_mutex);
    m_jobs.push_back(std::move(job));
}

bool WorkStealingQueue::pop(Job& job)
{
    std::lock_guard lock(m_mutex);
    if (m_jobs.empty())
    {
        return false;
    }
    job = std::move(m_jobs.back());
    m_jobs.pop_back();
    return true;
}

bool WorkStealingQueue::steal(Job& job)
{
    std::lock_guard lock(m_mutex);
    if (m_jobs.empty())
    {
        return false;
    }
    job = std::move(m_jobs.front());
    m_jobs.pop_front();
    return true;
}

bool WorkStealingQueue::empty() const
{
    std::lock_guard lock(m_mutex);
    return m_jobs.empty();
}

// -- JobSystem --

JobSystem::JobSystem(uint32_t threadCount)
{
    auto log = getLogger("core");

    if (threadCount == 0)
    {
        uint32_t hw = std::thread::hardware_concurrency();
        threadCount = (hw > 1) ? hw - 1 : 1;
    }

    for (uint32_t i = 0; i < threadCount; ++i)
    {
        m_queues.push_back(std::make_unique<WorkStealingQueue>());
    }

    for (uint32_t i = 0; i < threadCount; ++i)
    {
        m_workers.emplace_back(&JobSystem::workerLoop, this, i);
    }

    log->info("Job system started with {} worker threads", threadCount);
}

JobSystem::~JobSystem()
{
    m_running.store(false);
    m_wakeCondition.notify_all();

    for (auto& worker : m_workers)
    {
        if (worker.joinable())
        {
            worker.join();
        }
    }
}

void JobSystem::schedule(Job job)
{
    uint32_t index = m_nextQueue.fetch_add(1) % static_cast<uint32_t>(m_queues.size());
    m_pendingJobs.fetch_add(1);
    m_queues[index]->push(std::move(job));
    m_wakeCondition.notify_one();
}

void JobSystem::waitAll()
{
    std::unique_lock lock(m_completeMutex);
    m_completeCondition.wait(lock, [this]
    {
        return m_pendingJobs.load() <= 0;
    });
}

std::thread JobSystem::spawnDedicated(Job job)
{
    return std::thread(std::move(job));
}

void JobSystem::workerLoop(uint32_t workerIndex)
{
    while (m_running.load())
    {
        if (!tryRunJob(workerIndex))
        {
            std::unique_lock lock(m_wakeMutex);
            m_wakeCondition.wait_for(lock, std::chrono::milliseconds(1), [this, workerIndex]
            {
                return !m_running.load() || !m_queues[workerIndex]->empty();
            });
        }
    }

    while (tryRunJob(workerIndex)) {}
}

bool JobSystem::tryRunJob(uint32_t workerIndex)
{
    Job job;

    if (m_queues[workerIndex]->pop(job))
    {
        job();
        if (m_pendingJobs.fetch_sub(1) == 1)
        {
            m_completeCondition.notify_all();
        }
        return true;
    }

    uint32_t count = static_cast<uint32_t>(m_queues.size());
    for (uint32_t i = 1; i < count; ++i)
    {
        uint32_t victimIndex = (workerIndex + i) % count;
        if (m_queues[victimIndex]->steal(job))
        {
            job();
            if (m_pendingJobs.fetch_sub(1) == 1)
            {
                m_completeCondition.notify_all();
            }
            return true;
        }
    }

    return false;
}

} // namespace placeholder::core
