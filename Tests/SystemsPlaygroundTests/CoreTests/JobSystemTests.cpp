
#include "pch.h"

#include "../../../Engine/src/Core/JobSystem.h"

#include <gtest/gtest.h>

TEST(JobSystem, JobRunsWhenSubmitted) 
{
    JobSystem jobs;
    bool ran = false;

    auto h = jobs.Submit([&] { ran = true; });

    jobs.Wait(h);
    EXPECT_TRUE(ran);
}

TEST(JobSystem, JobDependenciesWork) 
{
    JobSystem jobs;
    bool firstRan = false;
    bool secondRan = false;
    auto first = jobs.Submit([&] { firstRan = true; });
    auto second = jobs.SubmitAfter(first, [&] { secondRan = true; });
    jobs.Wait(second);
    EXPECT_TRUE(firstRan);
    EXPECT_TRUE(secondRan);
}

TEST(JobSystem, ParallelForWorks) 
{
    JobSystem jobs;
    const size_t count = 1000;
    std::vector<int> data(count, 0);
    jobs.ParallelFor(count, [&](size_t i) {
        data[i] = int(i * 2);
    }, 64);
    for (size_t i = 0; i < count; ++i) {
        EXPECT_EQ(data[i], int(i * 2));
    }
}

TEST(JobSystem, ParallelForEachWorks) 
{
    JobSystem jobs;
    const size_t count = 1000;
    std::vector<int> data(count);
    for (size_t i = 0; i < count; ++i) {
        data[i] = int(i);
    }
    jobs.ParallelForEach(data, [](int& value) {
        value = value * 3;
    }, 64);
    for (size_t i = 0; i < count; ++i) {
        EXPECT_EQ(data[i], int(i * 3));
    }
}

TEST(JobSystem, WaitOnMultipleHandlesWorks) 
{
    JobSystem jobs;
    const size_t jobCount = 10;
    std::vector<bool> ran(jobCount, false);
    std::vector<JobHandle> handles;
    for (size_t i = 0; i < jobCount; ++i) 
    {
        handles.push_back(jobs.Submit([&, i] { ran[i] = true; }));
    }

    jobs.Wait(handles);

    for (size_t i = 0; i < jobCount; ++i)
    {
        EXPECT_TRUE(ran[i]);
    }
}