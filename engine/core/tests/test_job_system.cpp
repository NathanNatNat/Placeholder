#include <gtest/gtest.h>
#include "core/job_system.h"
#include "core/logging.h"

#include <atomic>
#include <chrono>
#include <vector>

using namespace placeholder::core;

class JobSystemTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        initLogging();
    }
};

TEST_F(JobSystemTest, ConstructionWithExplicitThreadCount)
{
    JobSystem jobs(2);
    EXPECT_EQ(jobs.threadCount(), 2u);
}

TEST_F(JobSystemTest, ConstructionWithDefaultThreadCount)
{
    JobSystem jobs(0);
    EXPECT_GT(jobs.threadCount(), 0u);
}

TEST_F(JobSystemTest, SingleJobExecutes)
{
    JobSystem jobs(2);
    std::atomic<bool> executed{false};

    jobs.schedule([&]() { executed.store(true); });
    jobs.waitAll();

    EXPECT_TRUE(executed.load());
}

TEST_F(JobSystemTest, ManyJobsExecute)
{
    JobSystem jobs(4);
    constexpr int count = 1000;
    std::atomic<int> counter{0};

    for (int i = 0; i < count; ++i)
    {
        jobs.schedule([&]() { counter.fetch_add(1); });
    }
    jobs.waitAll();

    EXPECT_EQ(counter.load(), count);
}

TEST_F(JobSystemTest, ScheduleWithResult)
{
    JobSystem jobs(2);

    auto future = jobs.scheduleWithResult([]() -> int { return 42; });
    jobs.waitAll();

    EXPECT_EQ(future.get(), 42);
}

TEST_F(JobSystemTest, ScheduleWithResultString)
{
    JobSystem jobs(2);

    auto future = jobs.scheduleWithResult([]() -> std::string
    {
        return "hello from job";
    });
    jobs.waitAll();

    EXPECT_EQ(future.get(), "hello from job");
}

TEST_F(JobSystemTest, JobsRunOnMultipleThreads)
{
    JobSystem jobs(4);
    std::mutex mutex;
    std::set<std::thread::id> threadIds;

    for (int i = 0; i < 100; ++i)
    {
        jobs.schedule([&]()
        {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
            std::lock_guard lock(mutex);
            threadIds.insert(std::this_thread::get_id());
        });
    }
    jobs.waitAll();

    EXPECT_GT(threadIds.size(), 1u);
}

TEST_F(JobSystemTest, SpawnDedicatedThread)
{
    std::atomic<bool> executed{false};
    auto thread = JobSystem::spawnDedicated([&]() { executed.store(true); });
    thread.join();

    EXPECT_TRUE(executed.load());
}

TEST_F(JobSystemTest, WaitAllWithNoJobs)
{
    JobSystem jobs(2);
    jobs.waitAll();
}

TEST_F(JobSystemTest, SequentialBatches)
{
    JobSystem jobs(4);

    std::atomic<int> counter{0};
    for (int i = 0; i < 50; ++i)
    {
        jobs.schedule([&]() { counter.fetch_add(1); });
    }
    jobs.waitAll();
    EXPECT_EQ(counter.load(), 50);

    for (int i = 0; i < 50; ++i)
    {
        jobs.schedule([&]() { counter.fetch_add(1); });
    }
    jobs.waitAll();
    EXPECT_EQ(counter.load(), 100);
}
