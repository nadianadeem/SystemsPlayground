
#include "JobSystem.h"
#include "LinearAllocator.h"

#include <iostream>

int main() {
    LinearAllocator allocator(1024); // 1 KB

    int* numbers = static_cast<int*>(
        allocator.Allocate(sizeof(int) * 100, alignof(int))
    );

    for (int i = 0; i < 100; ++i) {
        numbers[i] = i * 3;
        std::cout << numbers[i] << " " << std::endl;
    }

	allocator.Reset(); // numbers is now invalid

	JobSystem jobSystem;

    for (int i = 0; i < 10; ++i) {
        jobSystem.Submit([i] {
            std::cout << "Job " << i << " running on thread " << std::this_thread::get_id() << std::endl;
        });
    }

    jobSystem.Wait();

	std::cin >> std::ws;

    return 0;
}
