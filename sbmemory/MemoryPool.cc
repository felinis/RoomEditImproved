/*
*	Memory Module - Sabre Engine
*	(C) Moczulski Alan, 2023.
*/

#include "MemoryPool.hh"
#include <stdlib.h>
#include <memory.h>
#include <assert.h>

bool MemoryPool::Create(uint32_t size)
{
	data = malloc(size);
	if (!data)
		return false;

	memset(data, 0, size);

	currentOffset = 0;
	this->size = size;
	return true;
}

void MemoryPool::Destroy()
{
	if (data)
		free(data);
#ifndef NDEBUG
	data = nullptr;
	currentOffset = 0;
	size = 0;
#endif
}

const uint32_t MemoryPool::GetOffset() const
{
	return currentOffset;
}

const uint32_t MemoryPool::GetSize() const
{
	return size;
}

void MemoryPool::FlushFrom(uint32_t offset)
{
	memset((void*)((size_t)data + (size_t)offset), 0, size - offset);

	currentOffset = offset;
}
