#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <d3d12.h>

class UploadHeap
{
	ID3D12Heap *heap;
	ID3D12CommandQueue *copyCommandQueue;
	ID3D12CommandAllocator *copyCommandAllocator;
	ID3D12GraphicsCommandList *copyCommandList;

	bool PrepareCommandList();

public:
	constexpr UploadHeap():
		heap(nullptr),
		copyCommandQueue(nullptr),
		copyCommandAllocator(nullptr),
		copyCommandList(nullptr)
	{ }

	bool Create(ID3D12Device *device);
	void Destroy();



	bool ImmediatelyUploadToHeap(
		ID3D12Device *device,
		const D3D12_RESOURCE_DESC *desc,
		const D3D12_RESOURCE_ALLOCATION_INFO *ai,
		const void *data,
		D3D12_RESOURCE_STATES initialStates,
		ID3D12Resource **resource
	);

	bool ImmediatelyUploadTextureToHeap(
		ID3D12Device *device,
		const D3D12_RESOURCE_DESC *desc,
		const D3D12_RESOURCE_ALLOCATION_INFO *ai,
		const void *data,
		UINT dataSize,
		D3D12_RESOURCE_STATES initialStates,
		ID3D12Resource *texture
	);
};
