#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <d3d12.h>
#include <stdint.h>

//CPU-visible heap, used exclusively for uploading/modifying data, slowest heap
class DynamicHeap
{
	ID3D12Heap *heap;
	ID3D12Resource *dynamicBuffer;

public:
	constexpr DynamicHeap(): heap(nullptr), dynamicBuffer(nullptr) { }

	bool Create(ID3D12Device *device);
	void Destroy();

	void UpdateData(void *data, uint32_t size) const;
	uint64_t GetGPUAddress() const;
};
