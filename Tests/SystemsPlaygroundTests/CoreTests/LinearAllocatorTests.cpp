
#include "pch.h"

#include "../../../Engine/src/Core/LinearAllocator.h"

#include <gtest/gtest.h>

struct TestStruct
{
    int a;
    TestStruct() : a(7) {}
};

TEST(LinearAllocator, ResetsMemoryCorrectly) {
    LinearAllocator alloc(128);

    void* p1 = alloc.Allocate(64);
    alloc.Reset();
    void* p2 = alloc.Allocate(64);

    EXPECT_EQ(p1, p2);
}

TEST(LinearAllocator, AllocatesAlignedMemory) 
{
    LinearAllocator alloc(128);
    void* p1 = alloc.Allocate(16, 16);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(p1) % 16, 0);
    void* p2 = alloc.Allocate(32, 32);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(p2) % 32, 0);
}

TEST(LinearAllocator, NewOperatorWorks) 
{
    LinearAllocator alloc(256);

    TestStruct* obj = alloc.New<TestStruct>();
    EXPECT_EQ(obj->a, 7);
}

TEST(LinearAllocator, NewArrayOperatorWorks) 
{
    LinearAllocator alloc(512);

    size_t count = 5;
    TestStruct* arr = alloc.NewArray<TestStruct>(count);
    for (size_t i = 0; i < count; ++i) {
        EXPECT_EQ(arr[i].a, 7);
    }
}
