
#include "LinearAllocator.h"

#include <cstdlib>
#include <stdexcept>
#include <iostream>

LinearAllocator::LinearAllocator(size_t size)
{
	m_start = static_cast<std::byte*>(std::malloc(size));
	if (!m_start) {
		throw std::bad_alloc{};
	}

	m_current = m_start;
	m_end = m_start + size;
}

LinearAllocator::~LinearAllocator()
{
	std::free(m_start);
	m_start = m_current = m_end = nullptr;
}

void* LinearAllocator::Allocate(size_t size, size_t alignment)
{
	std::uintptr_t current = reinterpret_cast<std::uintptr_t>(m_current);

	std::uintptr_t aligned = (current + (alignment - 1)) & ~(alignment - 1);
	std::byte* alignedPtr = reinterpret_cast<std::byte*>(aligned);

	if (alignedPtr + size > m_end)
	{
		std::exception_ptr ex = std::make_exception_ptr(std::bad_alloc{});
		return nullptr;
	}

	m_current = alignedPtr + size;

	return alignedPtr;
}


void LinearAllocator::Reset()
{
	m_current = m_start;
}

