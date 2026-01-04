
#include "pch.h"

#include "../../../Engine/src/Core/LinearAllocator.h"

#include <gtest/gtest.h>

TEST(LinearAllocator, ResetsMemoryCorrectly) {
    LinearAllocator alloc(128);

    void* p1 = alloc.Allocate(64);
    alloc.Reset();
    void* p2 = alloc.Allocate(64);

    EXPECT_EQ(p1, p2);
}
