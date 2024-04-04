#pragma once

#include "heap/UploadHeap.hh"
#include "heap/DynamicHeap.hh"

/*
*	Manual video memory management.
*/
class HeapManager
{
	//default, fastest heap
	ID3D12Heap *heap = nullptr;
	UINT64 currentOffset = 0; //resource offset in the heap, the current VRAM pointer
	UINT64 safeOffset = 0;

	//upload heap
	UploadHeap uploadHeap;

	//dynamic heap
	DynamicHeap dynamicHeap;

	//align the current offset to 64 KiB (D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT)
	UINT64 AlignOffset(UINT64 offset, UINT64 alignment)
	{
		UINT64 a = alignment - 1;
		return ((offset + a) & ~a);
	}

public:
	bool Create(ID3D12Device *device);
	void Destroy();

	bool AllocateAndFillBuffer(
		ID3D12Device *device,
		const D3D12_RESOURCE_DESC *desc,
		const void *data,
		D3D12_RESOURCE_STATES initialStates,
		ID3D12Resource **resource
	);

	bool AllocateAndFillTexture(
		ID3D12Device *device,
		const D3D12_RESOURCE_DESC *desc,
		const void *data,
		UINT32 dataSize,
		D3D12_RESOURCE_STATES initialStates,
		ID3D12Resource **resource
	);

	bool AllocateImage(
		ID3D12Device *device,
		DXGI_FORMAT format,
		UINT width, UINT height,
		D3D12_RESOURCE_STATES initialState,
		D3D12_RESOURCE_FLAGS flags,
		D3D12_CLEAR_VALUE *clearValue,
		ID3D12Resource **resource
	);

	void SetSafeResetCheckpoint();
	void Reset();

	uint64_t UploadToDynamicBuffer(void *data, uint32_t size) const;
};
