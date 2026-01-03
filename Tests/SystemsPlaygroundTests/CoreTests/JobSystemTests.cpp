
#include "pch.h"

#include "../../../Engine/src/Core/JobSystem.h"

#include <gtest/gtest.h>

TEST(JobSystem, JobRunsWhenSubmitted) {
    JobSystem jobs;

    bool ran = false;
    auto h = jobs.Submit([&] { ran = true; });

    jobs.Wait(h);
    EXPECT_TRUE(ran);
}
