////////////////////////////////////////////////////////////////////////////////////////////////
//	Sabre Engine Graphics - Direct3D 12 implementation
//	(C) Moczulski Alan, 2023.
////////////////////////////////////////////////////////////////////////////////////////////////

#include "HeapManager.hh"
#include <assert.h>

////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef NDEBUG
#define DebugPrint(x) {OutputDebugString(x); DebugBreak();}
#else
#define DebugPrint(x)
#endif

static constexpr UINT64 ONE_MEGABYTE = 1024 * 1024;
static constexpr UINT64 VRAM_BUDGET = ONE_MEGABYTE * 128;

////////////////////////////////////////////////////////////////////////////////////////////////

bool HeapManager::Create(ID3D12Device *device)
{
	D3D12_HEAP_PROPERTIES heapProperties = {};
	//we are only using a single node/GPU
	heapProperties.CreationNodeMask = 1;
	heapProperties.VisibleNodeMask = 1;

	//create default heap
	{
		heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

		D3D12_HEAP_DESC heapDesc = {};
		heapDesc.SizeInBytes = VRAM_BUDGET;
		heapDesc.Properties = heapProperties;
		heapDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
		if FAILED(device->CreateHeap(&heapDesc, IID_PPV_ARGS(&heap)))
		{
			DebugPrint("Failed to create the default heap.");
			return false;
		}
	}

	//create upload heap
	if (!uploadHeap.Create(device))
		return false;

	//create dynamic heap
	if (!dynamicHeap.Create(device))
		return false;

	return true;
}

void HeapManager::Destroy()
{
	dynamicHeap.Destroy();
	uploadHeap.Destroy();
	if (heap)
		heap->Release();
}

/*
*	Allocates space in static VRAM to store a given buffer (for example: vertex, index buffers).
*/
bool HeapManager::AllocateAndFillBuffer(
	ID3D12Device *device,
	const D3D12_RESOURCE_DESC *desc,
	const void *data,
	D3D12_RESOURCE_STATES initialStates,
	ID3D12Resource **resource
)
{
	//TODO: aren't all resources' initial states set to D3D12_RESOURCE_STATE_COPY_DEST?
	//because we need to initialise them by copying from the upload heap...

	assert(heap);

	//get the size we need to use on the heap as well as the alignment
	D3D12_RESOURCE_ALLOCATION_INFO ai = device->GetResourceAllocationInfo(0, 1, desc);
	UINT64 newStartOffsetForResource = AlignOffset(currentOffset, ai.Alignment);

	//check if it will fit in our budget
	if (newStartOffsetForResource + ai.SizeInBytes > VRAM_BUDGET)
	{
		DebugPrint("Out of GPU memory budget!");
		return false;
	}

	if FAILED(device->CreatePlacedResource(
		heap,
		newStartOffsetForResource,
		desc,
		initialStates,
		nullptr,
		IID_PPV_ARGS(resource)
	))
	{
		DebugPrint("Failed to create the resource on the GPU's default heap.");
		return false;
	}

	currentOffset = newStartOffsetForResource + ai.SizeInBytes;

	return uploadHeap.ImmediatelyUploadToHeap(device, desc, &ai, data, initialStates, resource);
}

bool HeapManager::AllocateAndFillTexture(
	ID3D12Device *device,
	const D3D12_RESOURCE_DESC *desc,
	const void *data,
	UINT32 dataSize,
	D3D12_RESOURCE_STATES initialStates,
	ID3D12Resource **resource
)
{
	//TODO: aren't all resources' initial states set to D3D12_RESOURCE_STATE_COPY_DEST?
	//because we need to initialise them by copying from the upload heap...

	assert(heap);

	//get the size we need to use on the heap as well as the alignment
	D3D12_RESOURCE_ALLOCATION_INFO ai = device->GetResourceAllocationInfo(0, 1, desc);
	UINT64 newStartOffsetForResource = AlignOffset(currentOffset, ai.Alignment);

	//check if it will fit in our budget
	if (newStartOffsetForResource + ai.SizeInBytes > VRAM_BUDGET)
	{
		DebugPrint("Out of GPU memory budget!");
		return false;
	}

	if FAILED(device->CreatePlacedResource(
		heap,
		newStartOffsetForResource,
		desc,
		initialStates,
		nullptr,
		IID_PPV_ARGS(resource)
	))
	{
		DebugPrint("Failed to create the resource on the GPU's default heap.");
		return false;
	}

	currentOffset = newStartOffsetForResource + ai.SizeInBytes;

	return uploadHeap.ImmediatelyUploadTextureToHeap(device, desc, &ai, data, dataSize, initialStates, *resource);
}

/*
*	Allocates space in static VRAM to store a given image (for example: a texture or a depth buffer).
*/
bool HeapManager::AllocateImage(ID3D12Device *device, DXGI_FORMAT format, UINT width, UINT height, D3D12_RESOURCE_STATES initialState, D3D12_RESOURCE_FLAGS flags, D3D12_CLEAR_VALUE *clearValue, ID3D12Resource **imageResource)
{
	assert(heap);

	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Width = width;
	resourceDesc.Height = height;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.Format = format;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Flags = flags;

	//get the size we need to use on the heap as well as the alignment
	D3D12_RESOURCE_ALLOCATION_INFO ai = device->GetResourceAllocationInfo(0, 1, &resourceDesc);
	UINT64 newStartOffsetForResource = AlignOffset(currentOffset, ai.Alignment);

	//check if it will fit in our budget
	if (newStartOffsetForResource + ai.SizeInBytes > VRAM_BUDGET)
	{
		DebugPrint("Out of GPU memory budget!");
		return false;
	}

	if FAILED(device->CreatePlacedResource(
		heap,
		newStartOffsetForResource,
		&resourceDesc,
		initialState,
		clearValue, //should be nullptr on non-depth-stencil images
		IID_PPV_ARGS(imageResource)
	))
	{
		DebugPrint("Failed to create image.");
		return false;
	}

	currentOffset = newStartOffsetForResource + ai.SizeInBytes;
	return true;
}

void HeapManager::SetSafeResetCheckpoint()
{
	assert(heap);

	safeOffset = currentOffset;
}

void HeapManager::Reset()
{
	assert(heap);
	assert(safeOffset > 0); //there needs to be something inside the heap, and we must have already called SetSafeResetCheckpoint

	//WARNING: make sure all created heap resources are released before resetting
	//if this is not the case, D3D12 debug layers will inform us of memory leaks

	currentOffset = safeOffset;
}

uint64_t HeapManager::UploadToDynamicBuffer(void *data, uint32_t size) const
{
	dynamicHeap.UpdateData(data, size);
	return dynamicHeap.GetGPUAddress();
}
