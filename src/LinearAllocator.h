
#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>
#include <new>

class LinearAllocator {
public:
    LinearAllocator(size_t size);
    ~LinearAllocator();

    template<typename T>
    T* New() {
        void* mem = Allocate(sizeof(T), alignof(T));
        return new (mem) T();
    }

    template<typename T, typename... Args>
    T* New(Args&&... args) {
        void* mem = Allocate(sizeof(T), alignof(T));
        return new (mem) T(std::forward<Args>(args)...);
    }

    template<typename T>
    T* NewArray(size_t count) {
        void* mem = Allocate(sizeof(T) * count, alignof(T));
        T* arr = static_cast<T*>(mem);
        for (size_t i = 0; i < count; ++i)
            new (&arr[i]) T();
        return arr;
    }

    template<typename T>
    T* NewArray(size_t count, const T& value) {
        void* mem = Allocate(sizeof(T) * count, alignof(T));
        T* arr = static_cast<T*>(mem);
        for (size_t i = 0; i < count; ++i)
            new (&arr[i]) T(value);
        return arr;
    }

    void* Allocate(size_t size, size_t alignment = alignof(std::max_align_t));
    void Reset();

private:
    std::byte* m_start = nullptr;
    std::byte* m_current = nullptr;
    std::byte* m_end = nullptr;
};
