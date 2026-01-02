
#pragma once

#include <cstddef>
#include <cstdint>

class LinearAllocator {
public:
    LinearAllocator(size_t size);
    ~LinearAllocator();

    void* Allocate(size_t size, size_t alignment = alignof(std::max_align_t));
    void Reset();

private:
    std::byte* m_start = nullptr;
    std::byte* m_current = nullptr;
    std::byte* m_end = nullptr;
};
