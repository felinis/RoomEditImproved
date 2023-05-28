#pragma once
#include "internal/Array.h"
#include <assert.h>
#include <new>

/// <summary>
/// A linear allocator (fast and simple, but prone to heavy memory fragmentation).
/// </summary>
class MemoryPool
{
	static constexpr uint32_t ONE_MIBIBYTE = 1024 * 1024;

	void* data;
	uint32_t currentOffset;
	uint32_t size;

public:
	constexpr MemoryPool() : data(nullptr), currentOffset(0), size(0)
	{
	}

	bool Create(uint32_t size);
	void Destroy();

	/// <summary>
	/// Allocates memory.
	/// </summary>
	/// <param name="size">is the size to allocate</param>
	/// <param name="alignment">is the memory alignment</param>
	/// <returns>pointer to data</returns>
	template <typename T>
	inline T* Allocate(uint32_t numElements, uint32_t alignment = 4)
	{
		assert(alignment > 0 && ((alignment & (alignment - 1)) == 0)); //make sure alignment is a power of 2

		uint32_t size = sizeof(T) * numElements; // calculate size based on number of elements

		size_t currentAddress = reinterpret_cast<size_t>(data) + currentOffset;

		uint32_t misalignment = (alignment - (currentAddress & (alignment - 1))) & (alignment - 1);

		//make sure we're in budget
		assert(currentOffset + size + misalignment <= this->size && "Out of budget!");

		currentOffset += size + misalignment;

		auto dataStart = reinterpret_cast<T*>(currentAddress + misalignment);
		return dataStart;
	}


	/// <summary>
	/// Creates a new array of a given type T and a given number of elements.
	/// </summary>
	template <typename T>
	inline Array<T> CreateArray(uint32_t numElements)
	{
		assert(numElements <= 4000); //sanity check
		auto data = Allocate<T>(numElements);
		for (uint32_t i = 0; i < numElements; ++i)
		{
			new(data + i) T(); //using "placement new" to call the constructor on already allocated memory
		}
		auto arr = Array<T>(data, numElements);
		return arr;
	}

	/// <summary>
	/// Returns currently used space in the allocator.
	/// </summary>
	const uint32_t GetOffset() const;

	/// <summary>
	/// Returns the capacity of the allocator.
	/// </summary>
	const uint32_t GetSize() const;

	/// <summary>
	/// Discards memory starting from @offset, so we can use that space again for next allocations.
	/// </summary>
	void FlushFrom(uint32_t offset);
};
