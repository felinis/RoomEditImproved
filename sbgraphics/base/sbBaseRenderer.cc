/*
*	Sabre Engine Graphics - Direct3D 12 implementation
*	(C) Moczulski Alan, 2023.
*/

#include "sbBaseRenderer.h"
#include "engineFeatures.h"
#include <assert.h>

bool sbHeap::Create(ID3D12Device *device)
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
	{
		heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

		D3D12_HEAP_DESC heapDesc = {};
		heapDesc.SizeInBytes = UPLOAD_VRAM_BUDGET;
		heapDesc.Properties = heapProperties;
		heapDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
		heapDesc.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
		if FAILED(device->CreateHeap(&heapDesc, IID_PPV_ARGS(&uploadHeap)))
		{
			DebugPrint("Failed to create the upload heap.");
			return false;
		}
	}

	return true;
}

void sbHeap::Destroy()
{
	if (uploadHeap)
		uploadHeap->Release();
	if (heap)
		heap->Release();
}

bool sbHeap::ImmediatelyUploadToHeap(
	ID3D12Device *device,
	const D3D12_RESOURCE_DESC *desc,
	const D3D12_RESOURCE_ALLOCATION_INFO *ai,
	const void *data,
	D3D12_RESOURCE_STATES initialStates,
	ID3D12Resource **bufferResource
)
{
	//check if it will fit in our upload budget
	if (ai->SizeInBytes > UPLOAD_VRAM_BUDGET)
	{
		DebugPrint("Out of GPU upload memory budget!");
		return false;
	}

	//create a buffer on the upload heap
	ID3D12Resource *uploadBuffer;
	if FAILED(device->CreatePlacedResource(
		uploadHeap,
		0,
		desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&uploadBuffer)
	))
	{
		DebugPrint("Failed to create the resource on the GPU's default heap.");
		return false;
	}

	//copy data to the upload buffer
	UINT8 *pData;
	CD3DX12_RANGE readRange(0, 0);
	HRESULT hr = uploadBuffer->Map(0, &readRange, reinterpret_cast<void **>(&pData));

	if (FAILED(hr))
	{
		DebugPrint("Failed to map upload buffer.");
		uploadBuffer->Release();
		return false;
	}
	UINT64 dataSize = desc->Width * desc->Height;
	memcpy(pData, data, dataSize);
	uploadBuffer->Unmap(0, nullptr);

	//copy data from the upload buffer to the default heap buffer
	ID3D12CommandAllocator *cmdAlloc;
	hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&cmdAlloc));
	if (FAILED(hr))
	{
		DebugPrint("Failed to create command allocator.");
		uploadBuffer->Release();
		return false;
	}

	ID3D12GraphicsCommandList *cmdList;
	hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COPY, cmdAlloc, nullptr, IID_PPV_ARGS(&cmdList));
	if (FAILED(hr))
	{
		DebugPrint("Failed to create command list.");
		cmdAlloc->Release();
		uploadBuffer->Release();
		return false;
	}

//	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(*bufferResource, initialStates, D3D12_RESOURCE_STATE_COPY_DEST));
	cmdList->CopyBufferRegion(*bufferResource, 0, uploadBuffer, 0, dataSize);
//	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(*bufferResource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

	hr = cmdList->Close();

	if (FAILED(hr))
	{
		DebugPrint("Failed to close command list.");
		cmdList->Release();
		cmdAlloc->Release();
		uploadBuffer->Release();
		return false;
	}

	//execute the copy command
	ID3D12CommandQueue *cmdQueue;
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.NodeMask = 0;

	hr = device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&cmdQueue));

	if (FAILED(hr))
	{
		DebugPrint("Failed to create command queue.");
		cmdList->Release();
		cmdAlloc->Release();
		uploadBuffer->Release();
		return false;
	}

	ID3D12CommandList *ppCommandLists[] = { cmdList };
	cmdQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	//create a fence and wait for the copy to complete
	ID3D12Fence *fence;
	hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));

	if (FAILED(hr))
	{
		DebugPrint("Failed to create fence.");
		cmdQueue->Release();
		cmdList->Release();
		cmdAlloc->Release();
		uploadBuffer->Release();
		return false;
	}

	HANDLE hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (hEvent == nullptr)
	{
		DebugPrint("Failed to create event.");
		fence->Release();
		cmdQueue->Release();
		cmdList->Release();
		cmdAlloc->Release();
		uploadBuffer->Release();
		return false;
	}

	hr = cmdQueue->Signal(fence, 1);
	if (FAILED(hr))
	{
		DebugPrint("Failed to signal fence.");
		CloseHandle(hEvent);
		fence->Release();
		cmdQueue->Release();
		cmdList->Release();
		cmdAlloc->Release();
		uploadBuffer->Release();
		return false;
	}

	if (fence->GetCompletedValue() < 1)
	{
		hr = fence->SetEventOnCompletion(1, hEvent);
		if (SUCCEEDED(hr))
		{
			WaitForSingleObject(hEvent, INFINITE);
		}
		else
		{
			DebugPrint("Failed to set event on completion.");
		}
	}

	CloseHandle(hEvent);
	fence->Release();
	cmdQueue->Release();

	cmdList->Release();
	cmdAlloc->Release();
	uploadBuffer->Release();

	if (FAILED(hr))
	{
		DebugPrint("Failed during buffer upload process.");
		return false;
	}

	return true;
}

bool sbHeap::ImmediatelyUploadTextureToHeap(
	ID3D12Device *device,
	const D3D12_RESOURCE_DESC *desc,
	const D3D12_RESOURCE_ALLOCATION_INFO *ai,
	const void *data,
	UINT dataSize,
	D3D12_RESOURCE_STATES initialStates,
	ID3D12Resource *texture
)
{
	//check if it will fit in our upload budget
	if (ai->SizeInBytes > UPLOAD_VRAM_BUDGET)
	{
		DebugPrint("Out of GPU upload memory budget!");
		return false;
	}

	//create the intermediate buffer on the upload heap
	//remember that we cannot use @desc since that is for a texture buffer
	//and we cannot create a texture buffer on the upload heap,
	//so it must be a generic, one dimensional buffer
	D3D12_RESOURCE_DESC bufferDesc = {};
	bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufferDesc.Width = ai->SizeInBytes; //IMPORTANT: this number is greater that @dataSize since it involves alignment
	bufferDesc.Height = 1;
	bufferDesc.DepthOrArraySize = 1;
	bufferDesc.MipLevels = 1;
	bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	bufferDesc.SampleDesc.Count = 1;
	bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	ID3D12Resource *intermediateBuffer;
	if FAILED(device->CreatePlacedResource(
		uploadHeap, 0, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr, IID_PPV_ARGS(&intermediateBuffer)
	))
	{
		DebugPrint("Failed to create a committed resource.");
		return false;
	}

	//copy texture data to the upload buffer
	UINT8 *pData;
	CD3DX12_RANGE readRange(0, 0);
	HRESULT hr = intermediateBuffer->Map(0, &readRange, reinterpret_cast<void **>(&pData));

	if (FAILED(hr))
	{
		DebugPrint("Failed to map upload buffer.");
		intermediateBuffer->Release();
		return false;
	}
	memcpy(pData, data, dataSize);
	intermediateBuffer->Unmap(0, nullptr);

	ID3D12CommandAllocator *cmdAlloc;
	hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&cmdAlloc));
	if (FAILED(hr))
	{
		DebugPrint("Failed to create command allocator.");
		intermediateBuffer->Release();
		return false;
	}

	ID3D12GraphicsCommandList *cmdList;
	hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COPY, cmdAlloc, nullptr, IID_PPV_ARGS(&cmdList));
	if (FAILED(hr))
	{
		DebugPrint("Failed to create command list.");
		cmdAlloc->Release();
		intermediateBuffer->Release();
		return false;
	}

	//START COPY TO HEAP
	UINT64 requiredSize;
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
	device->GetCopyableFootprints(desc, 0, 1, 0, &footprint, nullptr, nullptr, &requiredSize);
	//copy the texture data from the intermediate buffer to the texture resource using CopyTextureRegion
	D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
	srcLocation.pResource = intermediateBuffer;
	srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	srcLocation.PlacedFootprint = footprint;

	D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
	dstLocation.pResource = texture;
	dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	dstLocation.SubresourceIndex = 0;

//	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(*bufferResource, initialStates, D3D12_RESOURCE_STATE_COPY_DEST));
	cmdList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);
//	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(*bufferResource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

	hr = cmdList->Close();

	if (FAILED(hr))
	{
		DebugPrint("Failed to close command list.");
		cmdList->Release();
		cmdAlloc->Release();
		intermediateBuffer->Release();
		return false;
	}

	//execute the copy command
	ID3D12CommandQueue *cmdQueue;
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.NodeMask = 0;

	hr = device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&cmdQueue));

	if (FAILED(hr))
	{
		DebugPrint("Failed to create command queue.");
		cmdList->Release();
		cmdAlloc->Release();
		intermediateBuffer->Release();
		return false;
	}

	ID3D12CommandList *ppCommandLists[] = { cmdList };
	cmdQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	//create a fence and wait for the copy to complete
	ID3D12Fence *fence;
	hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));

	if (FAILED(hr))
	{
		DebugPrint("Failed to create fence.");
		cmdQueue->Release();
		cmdList->Release();
		cmdAlloc->Release();
		intermediateBuffer->Release();
		return false;
	}

	HANDLE hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (hEvent == nullptr)
	{
		DebugPrint("Failed to create event.");
		fence->Release();
		cmdQueue->Release();
		cmdList->Release();
		cmdAlloc->Release();
		intermediateBuffer->Release();
		return false;
	}

	hr = cmdQueue->Signal(fence, 1);
	if (FAILED(hr))
	{
		DebugPrint("Failed to signal fence.");
		CloseHandle(hEvent);
		fence->Release();
		cmdQueue->Release();
		cmdList->Release();
		cmdAlloc->Release();
		intermediateBuffer->Release();
		return false;
	}

	if (fence->GetCompletedValue() < 1)
	{
		hr = fence->SetEventOnCompletion(1, hEvent);
		if (SUCCEEDED(hr))
		{
			WaitForSingleObject(hEvent, INFINITE);
		}
		else
		{
			DebugPrint("Failed to set event on completion.");
		}
	}

	CloseHandle(hEvent);
	fence->Release();
	cmdQueue->Release();

	cmdList->Release();
	cmdAlloc->Release();
	intermediateBuffer->Release();

	if (FAILED(hr))
	{
		DebugPrint("Failed during buffer upload process.");
		return false;
	}

	return true;
}
#if 0
bool sbHeap::CreateResource(
	ID3D12Device *device,
	const D3D12_RESOURCE_DESC *desc,
	D3D12_RESOURCE_STATES initialState,
	const D3D12_CLEAR_VALUE *clearValue,
	ID3D12Resource **resource
)
{
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
		initialState,
		clearValue,
		IID_PPV_ARGS(resource)
	))
	{
		DebugPrint("Failed to create placed resource.");
		return false;
	}

	currentOffset = newStartOffsetForResource + ai.SizeInBytes;
	return true;
}

bool sbHeap::CreateAndFillResource(
	ID3D12Device *device,
	const D3D12_RESOURCE_DESC *desc,
	D3D12_RESOURCE_STATES initialState,
	const D3D12_CLEAR_VALUE *clearValue,
	ID3D12Resource **resource
)
{
	assert(heap);

	if (!CreateResource(device, desc, initialState, clearValue, resource))
		return false;

	return ImmediatelyUploadToHeap(device, desc, ai, data, initialState, resource);
}

bool sbHeap::AllocateBufferOnUploadHeap(
	ID3D12Device *device,
	UINT64 size,
	D3D12_RESOURCE_STATES initialStates,
	ID3D12Resource **resource
)
{
	//create the resource on the GPU's default heap
	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Width = size;
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	//get the size we need to use on the heap as well as the alignment
	D3D12_RESOURCE_ALLOCATION_INFO ai = device->GetResourceAllocationInfo(0, 1, &resourceDesc);

	//check if it will fit in our upload budget
	if (ai.SizeInBytes > UPLOAD_VRAM_BUDGET)
	{
		DebugPrint("Out of GPU upload memory budget!");
		return false;
	}

	//create a buffer on the upload heap
	if FAILED(device->CreatePlacedResource(
		uploadHeap,
		0,
		&resourceDesc,
		initialStates,
		nullptr,
		IID_PPV_ARGS(resource)
	))
	{
		DebugPrint("Failed to create the placed resource on the GPU's upload heap.");
		return false;
	}

	return true;
}
#endif
/*
*	Allocates space in static VRAM to store a given buffer (for example: vertex, index buffers).
*/
bool sbHeap::AllocateAndFillBuffer(
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
	assert(uploadHeap);

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

	return ImmediatelyUploadToHeap(device, desc, &ai, data, initialStates, resource);
}

bool sbHeap::AllocateAndFillTexture(
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
	assert(uploadHeap);

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

	return ImmediatelyUploadTextureToHeap(device, desc, &ai, data, dataSize, initialStates, *resource);
}
#if 0
static UINT CalculateImageSize(DXGI_FORMAT format, UINT width, UINT height)
{
	bool isBlockCompressed = false;
	UINT bitsPerPixel = 0;
	UINT blockSize = 0;

	switch (format)
	{
	case DXGI_FORMAT_R8G8B8A8_UNORM:
	case DXGI_FORMAT_R8G8B8A8_UINT:
	case DXGI_FORMAT_D32_FLOAT:
		bitsPerPixel = 32;
		break;
	case DXGI_FORMAT_BC1_UNORM:
	case DXGI_FORMAT_BC1_UNORM_SRGB:
		isBlockCompressed = true;
		bitsPerPixel = 4;
		blockSize = 4;
		break;
	case DXGI_FORMAT_BC2_UNORM:
	case DXGI_FORMAT_BC2_UNORM_SRGB:
	case DXGI_FORMAT_BC3_UNORM:
	case DXGI_FORMAT_BC3_UNORM_SRGB:
		isBlockCompressed = true;
		bitsPerPixel = 8;
		blockSize = 4;
		break;
	default:
#ifndef NDEBUG
		DebugPrint("Unsupported format. Please add!");
#endif
		return 0;
	}

	UINT imageSize;
	if (isBlockCompressed)
	{
		UINT blockWidth = (width + blockSize - 1) / blockSize;
		UINT blockHeight = (height + blockSize - 1) / blockSize;
		imageSize = blockWidth * blockHeight * bitsPerPixel;
	}
	else
		imageSize = width * height * (bitsPerPixel / 8);

	return imageSize;
}
#endif
/*
*	Allocates space in static VRAM to store a given image (for example: a texture or a depth buffer).
*/
bool sbHeap::AllocateImage(ID3D12Device *device, DXGI_FORMAT format, UINT width, UINT height, D3D12_RESOURCE_STATES initialState, D3D12_RESOURCE_FLAGS flags, D3D12_CLEAR_VALUE *clearValue, ID3D12Resource **imageResource)
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

void sbHeap::SetSafeResetCheckpoint()
{
	assert(heap);

	safeOffset = currentOffset;
}

void sbHeap::Reset()
{
	assert(heap);
	assert(safeOffset > 0); //there needs to be something inside the heap, and we must have already called SetSafeResetCheckpoint

	//WARNING: make sure all created heap resources are released before resetting

	currentOffset = safeOffset;
}

bool sbSwapChain::Create(ID3D12Device *device, HWND hWnd, IDXGIFactory6 *factory, sbHeap &heap)
{
	//create command queue
	{
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		if FAILED(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&queue)))
		{
			DebugPrint("Failed to create command queue.");
			return false;
		}
	}

	//get the window's client area size
	RECT r;
	GetClientRect(hWnd, &r);
	LONG width = r.right;
	LONG height = r.bottom;

	//create the swap chain
	{

		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		swapChainDesc.Width = width;
		swapChainDesc.Height = height;
		swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = NUM_RENDER_TARGETS;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

		IDXGISwapChain1 *tempSwapChain = nullptr;
		if FAILED(factory->CreateSwapChainForHwnd(queue, hWnd, &swapChainDesc, nullptr, nullptr, &tempSwapChain))
		{
			DebugPrint("Failed to create a swap chain for a given window handle.");
			return false;
		}
		//get a IDXGISwapChain3* from the IDXGISwapChain1*
		tempSwapChain->QueryInterface(IID_PPV_ARGS(&swapChain));
		tempSwapChain->Release();
	}

	//create render target view descriptor heap and render target views
	{
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = NUM_RENDER_TARGETS;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		if FAILED(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvDescriptorHeap)))
		{
			DebugPrint("Failed to create a descriptor heap for the render target views.");
			return false;
		}

		//create render target views
		UINT rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
		for (UINT i = 0; i < 2; ++i)
		{
			rtvHandles[i] = rtvHandle;
			swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i]));
			device->CreateRenderTargetView(renderTargets[i], nullptr, rtvHandle);
			rtvHandle.Offset(1, rtvDescriptorSize);
		}
	}
	
	//create depth view descriptor heap and depth view
	{
		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
		dsvHeapDesc.NumDescriptors = 1; //we only need one descriptor for the depth buffer
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		if FAILED(device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvDescriptorHeap)))
		{
			DebugPrint("Failed to create a descriptor heap for the depth view.");
			return false;
		}

		D3D12_CLEAR_VALUE clearValue;
		clearValue.Format = DXGI_FORMAT_D32_FLOAT;
		clearValue.DepthStencil.Depth = 1.0f;
		clearValue.DepthStencil.Stencil = 0;

		//create depth buffer
		if (!heap.AllocateImage(
			device,
			DXGI_FORMAT_D32_FLOAT, //recommended depth format by nVidia
			width,
			height,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,
			&clearValue,
			&depthBuffer
		))
			return false;

		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
		device->CreateDepthStencilView(depthBuffer, &dsvDesc, dsvHandle);
	}

	//create synchronisation objects
	{
		HRESULT hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
		if (FAILED(hr))
		{
			DebugPrint("Failed to create fence.");
			return false;
		}

		fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (!fenceEvent)
		{
			DebugPrint("Failed to create fence event.");
			return false;
		}
	}

	Resize(width, height);

	return true;
}

void sbSwapChain::Destroy()
{
	if (fenceEvent)
		CloseHandle(fenceEvent);
	if (fence)
		fence->Release();

	if (depthBuffer)
		depthBuffer->Release();
	if (dsvDescriptorHeap)
		dsvDescriptorHeap->Release();

	for (UINT i = 0; i < 2; ++i)
	{
		if (renderTargets[i])
			renderTargets[i]->Release();
	}
	if (rtvDescriptorHeap)
		rtvDescriptorHeap->Release();
	
	if (swapChain)
		swapChain->Release();

	if (queue)
		queue->Release();
}

void sbSwapChain::Resize(UINT width, UINT height)
{
	viewport.Width = (float)800;
	viewport.Height = (float)600;
	viewport.MaxDepth = 1.0f;
	scissorRect.right = 800;
	scissorRect.bottom = 600;
}

void sbSwapChain::SetViewport(ID3D12GraphicsCommandList *commandList)
{
	commandList->RSSetViewports(1, &viewport);
	commandList->RSSetScissorRects(1, &scissorRect);
}

void sbSwapChain::TransitionToRenderable(ID3D12GraphicsCommandList *commandList)
{
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[frameIndex], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
}

void sbSwapChain::TransitionToPresentable(ID3D12GraphicsCommandList *commandList)
{
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[frameIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
}

void sbSwapChain::BindRenderTarget(ID3D12GraphicsCommandList *commandList)
{
	commandList->OMSetRenderTargets(1, &rtvHandles[frameIndex], FALSE, &dsvHandle);
}

void sbSwapChain::ClearRenderTarget(ID3D12GraphicsCommandList *commandList)
{
//	const float color[4] = { 0.1f, 0.2f, 0.0f, 1.0f };
//	commandList->ClearRenderTargetView(rtvHandles[frameIndex], color, 0, nullptr);

	commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
}

void sbSwapChain::ExecuteAndPresent(UINT numCommandLists, ID3D12CommandList *const *commandLists)
{
	assert(swapChain);
	assert(queue);

	//execute the recorded commands
	queue->ExecuteCommandLists(numCommandLists, commandLists);

	//present the frame
	if FAILED(swapChain->Present(1, 0))
	{
		DebugPrint("Failed to present the swap chain.");
		return;
	}

	//schedule a Signal command in the queue
	const UINT64 fenceValue = fenceValues[frameIndex];
	if FAILED(queue->Signal(fence, fenceValue))
	{
		DebugPrint("Failed to signal the fence.");
		return;
	}

	//update frame index
	frameIndex = swapChain->GetCurrentBackBufferIndex();
	assert(frameIndex >= 0 && frameIndex < NUM_RENDER_TARGETS);

	//if the next frame is not ready to be rendered yet, wait until it is ready
	if (fence->GetCompletedValue() < fenceValues[frameIndex])
	{
		if FAILED(fence->SetEventOnCompletion(fenceValues[frameIndex], fenceEvent))
		{
			DebugPrint("Failed to set event on completion.");
			return;
		}

		WaitForSingleObjectEx(fenceEvent, INFINITE, FALSE);
	}

	//update the fence value for the next frame
	fenceValues[frameIndex] = fenceValue + 1;
}

void sbSwapChain::WaitForGpu()
{
	if (!fenceEvent)
		return; //no need to wait for anything if the swapchain has not been fully initialised

	//schedule a Signal command in the queue
	if FAILED(queue->Signal(fence, fenceValues[frameIndex]))
	{
		DebugPrint("Failed to signal the fence.");
		return;
	}

	//wait until the fence has been processed
	if FAILED(fence->SetEventOnCompletion(fenceValues[frameIndex], fenceEvent))
	{
		DebugPrint("Failed to set event on completion.");
		return;
	}
	WaitForSingleObjectEx(fenceEvent, INFINITE, FALSE);

	//increment the fence value for the current frame
	fenceValues[frameIndex]++;
}

void sbSwapChain::MoveToNextFrame()
{
	//schedule a Signal command in the queue
	const UINT64 currentFenceValue = fenceValues[frameIndex];
	if FAILED(queue->Signal(fence, currentFenceValue))
	{
		DebugPrint("Failed to schedule a signal command in the queue.");
		return;
	}

	//update the frame index
	frameIndex = swapChain->GetCurrentBackBufferIndex();

	//if the next frame is not ready to be rendered yet, wait until it is ready
	if (fence->GetCompletedValue() < fenceValues[frameIndex])
	{
		if FAILED(fence->SetEventOnCompletion(fenceValues[frameIndex], fenceEvent))
		{
			DebugPrint("Failed to start waiting on frame render completion.");
			return;
		}
		WaitForSingleObjectEx(fenceEvent, INFINITE, FALSE);
	}

	//set the fence value for the next frame
	fenceValues[frameIndex] = currentFenceValue + 1;
}

#if 0
/*
*	We are using a single queue for both rendering and presenting the swap chain,
*	it simplifies GPU synchronization and is perfectly suitable for what we do.
*/
class sbCmdQueue
{
	ID3D12CommandQueue *queue = nullptr;

public:
	bool Create(ID3D12Device *device)
	{
		D3D12_COMMAND_QUEUE_DESC cqd;
		cqd.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;		//this type of command list can be used for all graphics, compute and presentation commands
		cqd.Priority = 0;
		cqd.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		cqd.NodeMask = 0;

		HRESULT hr = device->CreateCommandQueue(&cqd, IID_PPV_ARGS(&queue));
		return SUCCEEDED(hr);
	}

	void Destroy()
	{
		queue->Release();
	}

	void Execute(unsigned int count, ID3D12CommandList *const *commandLists)
	{
		queue->ExecuteCommandLists(count, commandLists);
	}
};
#endif

bool sbBaseRenderer::Create(HWND hWnd)
{
	assert(hWnd);
	UINT dxgiFactoryFlags = 0;

#ifndef NDEBUG
	//enable the debug layer, this will help us during development
	//by printing useful messages/errors about the state of the GPU
	ID3D12Debug *debugController;
	if SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))
	{
		debugController->EnableDebugLayer();
		debugController->Release();
		dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
	}
#endif

	//we need to get the a DXGIFactory to fetch adapters present on the system,
	//and to create the swapchain later on...
	if FAILED(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)))
	{
		DebugPrint("Failed to create DXGI factory.");
		return false;                                                                 
	}

	//fetch a hardware adapter and try to create a D3D12 device
	{
		IDXGIAdapter1 *adapter;
		bool requestHighPerformanceAdapter = true;
		for (unsigned int adapterIndex = 0;
			SUCCEEDED(factory->EnumAdapterByGpuPreference(
				adapterIndex,
				requestHighPerformanceAdapter == true ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_UNSPECIFIED,
				IID_PPV_ARGS(&adapter))
			);
			++adapterIndex)
		{
			DXGI_ADAPTER_DESC1 desc;
			adapter->GetDesc1(&desc);

			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			{
				//don't select the Basic Render Driver adapter
				continue;
			}

			//check if the adapter supports D3D12 by trying to create a D3D12Device
			if (SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device))))
				break; //ok
		}
		if (adapter)
			adapter->Release();
	}

	if (!device)
	{
		DebugPrint("No Direct3D 12 compatible adapter found.");
		return false;
	}

#ifndef NDEBUG
	//enable additional debugging messages
	ID3D12InfoQueue *infoQueue;
	if SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&infoQueue)))
	{
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
		infoQueue->Release();
	}
#endif

	//first of all, let's check if this device supports all the features our engine uses
	if (!AreAllEngineFeaturesSupported(device, hWnd))
		return false;

	if (!heap.Create(device))
		return false;

	if (!swapChain.Create(device, hWnd, factory, heap))
		return false;

	//create command allocator and command list
	{
		HRESULT hr = device->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(&commandAllocator)
		);
		if (FAILED(hr))
		{
			DebugPrint("Failed to create command allocator.");
			return false;
		}

		// Create command list
		hr = device->CreateCommandList(
			0,
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			commandAllocator,
			nullptr, //initial pipeline state object (not needed for this example)
			IID_PPV_ARGS(&commandList)
		);
		if (FAILED(hr))
		{
			DebugPrint("Failed to create command list.");
			return false;
		}

		//command lists are created in the recording state, but there is nothing to record yet,
		//so close it to switch to the executable state
		commandList->Close();

		return true;
	}

	return true;
}

void sbBaseRenderer::Destroy()
{
	//ensure that the GPU is no longer referencing resources
	//that are about to be cleaned up
	swapChain.WaitForGpu();

	if (commandList)
		commandList->Release();
	if (commandAllocator)
		commandAllocator->Release();

	swapChain.Destroy();
	heap.Destroy();

	if (device)
		device->Release();
	if (factory)
		factory->Release();
}

bool sbBaseRenderer::StartFrame()
{
	HRESULT hr = commandAllocator->Reset();
	if (FAILED(hr))
	{
		DebugPrint("Failed to reset command allocator.");
		return false;
	}

	hr = commandList->Reset(commandAllocator, nullptr);
	if (FAILED(hr))
	{
		DebugPrint("Failed to reset command list.");
		return false;
	}

	swapChain.SetViewport(commandList);

	//indicate that the back buffer will be used as a render target
	swapChain.TransitionToRenderable(commandList);

	swapChain.BindRenderTarget(commandList);

	//we are now ready to record commands

	return true; //we successfully started the frame
}

void sbBaseRenderer::EndAndPresentFrame()
{
	//indicate that the back buffer will now be used to present
	swapChain.TransitionToPresentable(commandList);

	//we are done recording commands
	HRESULT hr = commandList->Close();
	if (FAILED(hr))
	{
		DebugPrint("Failed to close command list.");
		return;
	}

	ID3D12CommandList *commandLists[] = { commandList };
	swapChain.ExecuteAndPresent(_countof(commandLists), commandLists);

	swapChain.MoveToNextFrame();
}
