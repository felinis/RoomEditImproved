////////////////////////////////////////////////////////////////////////////////////////////////
//	Sabre Engine Graphics - Direct3D 12 implementation
//	(C) Moczulski Alan, 2023.
////////////////////////////////////////////////////////////////////////////////////////////////

#include "UploadHeap.hh"
#include "../d3dx12.hh"

////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef NDEBUG
#define DebugPrint(x) {OutputDebugString(x); DebugBreak();}
#else
#define DebugPrint(x)
#endif

static constexpr UINT64 ONE_MEGABYTE = 1024 * 1024;
//upload heap, used exclusively for uploading data to the default, fastest heap
static constexpr UINT64 UPLOAD_VRAM_BUDGET = ONE_MEGABYTE * 8;

////////////////////////////////////////////////////////////////////////////////////////////////

bool UploadHeap::Create(ID3D12Device *device)
{
	D3D12_HEAP_PROPERTIES heapProperties = {};
	//we are only using a single node/GPU
	heapProperties.CreationNodeMask = 1;
	heapProperties.VisibleNodeMask = 1;
	heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_HEAP_DESC heapDesc = {};
	heapDesc.SizeInBytes = UPLOAD_VRAM_BUDGET;
	heapDesc.Properties = heapProperties;
	heapDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	heapDesc.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
	if FAILED(device->CreateHeap(&heapDesc, IID_PPV_ARGS(&heap)))
		return false;

	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.NodeMask = 0;
	if (FAILED(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&copyCommandQueue))))
	{
		DebugPrint("Failed to create command queue.");
		return false;
	}

	if (FAILED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&copyCommandAllocator))))
	{
		DebugPrint("Failed to create command allocator.");
		return false;
	}

	if (FAILED(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COPY, copyCommandAllocator, nullptr, IID_PPV_ARGS(&copyCommandList))))
	{
		DebugPrint("Failed to create command list.");
		return false;
	}

	return true;
}

void UploadHeap::Destroy()
{
	copyCommandList->Release();
	copyCommandAllocator->Release();
	copyCommandQueue->Release();
	heap->Release();
}

bool UploadHeap::PrepareCommandList()
{
	if (FAILED(copyCommandAllocator->Reset()))
	{
		DebugPrint("Failed to reset command allocator.");
		return false;
	}

	if (FAILED(copyCommandList->Reset(copyCommandAllocator, nullptr)))
	{
		DebugPrint("Failed to reset command list.");
		return false;
	}

	return true;
}

bool UploadHeap::ImmediatelyUploadToHeap(
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
		heap,
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

//	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(*bufferResource, initialStates, D3D12_RESOURCE_STATE_COPY_DEST));
	copyCommandList->CopyBufferRegion(*bufferResource, 0, uploadBuffer, 0, dataSize);
//	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(*bufferResource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

	if (FAILED(copyCommandList->Close()))
	{
		DebugPrint("Failed to close command list.");
		uploadBuffer->Release();
		return false;
	}

	ID3D12CommandList *ppCommandLists[] = { copyCommandList };
	copyCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	//create a fence and wait for the copy to complete
	ID3D12Fence *fence;
	if (FAILED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence))))
	{
		DebugPrint("Failed to create fence.");
		uploadBuffer->Release();
		return false;
	}

	HANDLE hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (hEvent == nullptr)
	{
		DebugPrint("Failed to create event.");
		fence->Release();
		uploadBuffer->Release();
		return false;
	}

	hr = copyCommandQueue->Signal(fence, 1);
	if (FAILED(hr))
	{
		DebugPrint("Failed to signal fence.");
		CloseHandle(hEvent);
		fence->Release();
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
	uploadBuffer->Release();

	if (!PrepareCommandList())
		return false;

	return true;
}

bool UploadHeap::ImmediatelyUploadTextureToHeap(
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
//	bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	bufferDesc.SampleDesc.Count = 1;
	bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	ID3D12Resource *intermediateBuffer;
	if FAILED(device->CreatePlacedResource(
		heap, 0, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr, IID_PPV_ARGS(&intermediateBuffer)
	))
	{
		DebugPrint("Failed to create a placed resource.");
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
//	dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
//	dstLocation.SubresourceIndex = 0;

//	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(*bufferResource, initialStates, D3D12_RESOURCE_STATE_COPY_DEST));
	copyCommandList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);
//	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(*bufferResource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

	if (FAILED(copyCommandList->Close()))
	{
		DebugPrint("Failed to close command list.");
		intermediateBuffer->Release();
		return false;
	}

	ID3D12CommandList *ppCommandLists[] = { copyCommandList };
	copyCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	//create a fence and wait for the copy to complete
	ID3D12Fence *fence;
	if (FAILED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence))))
	{
		DebugPrint("Failed to create fence.");
		intermediateBuffer->Release();
		return false;
	}

	HANDLE hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (hEvent == nullptr)
	{
		DebugPrint("Failed to create event.");
		fence->Release();
		intermediateBuffer->Release();
		return false;
	}

	if (FAILED(copyCommandQueue->Signal(fence, 1)))
	{
		DebugPrint("Failed to signal fence.");
		CloseHandle(hEvent);
		fence->Release();
		intermediateBuffer->Release();
		return false;
	}

	if (fence->GetCompletedValue() < 1)
	{
		if (SUCCEEDED(fence->SetEventOnCompletion(1, hEvent)))
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
	intermediateBuffer->Release();

	if (!PrepareCommandList())
		return false;

	return true;
}
