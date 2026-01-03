
#include "Core/JobSystem.h"
#include "Core/LinearAllocator.h"

#include <iostream>
#include <vector>

int main() {
	//   LinearAllocator allocator(1024); // 1 KB

	//   int* numbers = static_cast<int*>(
	//       allocator.Allocate(sizeof(int) * 100, alignof(int))
	//   );

	//   for (int i = 0; i < 100; ++i) {
	//       numbers[i] = i * 3;
	//       std::cout << numbers[i] << " " << std::endl;
	//   }

	   //allocator.Reset(); // numbers is now invalid

	   /*JobSystem jobSystem;

	   for (int i = 0; i < 10; ++i) {
		   jobSystem.Submit([i] {
			   std::cout << "Job " << i << " running on thread " << std::this_thread::get_id() << std::endl;
		   });
	   }

	   const size_t count = 1000;
	   std::vector<int> data(count);

	   for (int i = 0; i < data.size(); ++i) { data[i] = i; }*/

	   /*jobSystem.ParallelFor(count, [&](size_t i) {
		   data[i] = int(i * 2);
	   }, 64);*/

	   /*jobSystem.ParallelForEach(data, [](int& value) {
		   value = value * 3;
		   }, 64); */

	JobSystem jobs;

	/* JobHandle load = jobs.Submit([] {
		std::cout << "Loading file\n";
	});

	JobHandle parse = jobs.SubmitAfter(load, [] {
		std::cout << "Parsing file\n";
	});

	JobHandle upload = jobs.SubmitAfter(parse, [] {
		std::cout << "Uploading to GPU\n";
	});

	jobs.Wait(upload);*/

	jobs.Submit([&] {
		auto& scratch = jobs.GetScratchAllocator();

		float* temp = static_cast<float*>(scratch.Allocate(sizeof(float) * 1024));

		for (int i = 0; i < 1024; ++i)
			temp[i] = i * 0.5f;

		std::cout << "Job finished with scratch memory\n";
		});

	std::cin >> std::ws;

	return 0;
}
