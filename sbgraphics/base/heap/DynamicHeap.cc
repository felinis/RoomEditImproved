/*
*	Sabre Engine Graphics - Direct3D 12 implementation
*	(C) Moczulski Alan, 2023.
*/

#include "DynamicHeap.hh"
#include "../d3dx12.hh"
#include <assert.h>

#ifndef NDEBUG
#define DebugPrint(x) {OutputDebugString(x); DebugBreak();}
#else
#define DebugPrint(x)
#endif

static constexpr UINT64 CPU_VISIBLE_VRAM_BUDGET = 1024 * 1024;

bool DynamicHeap::Create(ID3D12Device *device)
{
	D3D12_HEAP_PROPERTIES heapProperties = {};
	//we are only using a single node/GPU
	heapProperties.CreationNodeMask = 1;
	heapProperties.VisibleNodeMask = 1;
	heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD; //an upload heap is CPU-visible

	D3D12_HEAP_DESC heapDesc = {};
	heapDesc.SizeInBytes = CPU_VISIBLE_VRAM_BUDGET;
	heapDesc.Properties = heapProperties;
	heapDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	heapDesc.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
	if FAILED(device->CreateHeap(&heapDesc, IID_PPV_ARGS(&heap)))
		return false;

	D3D12_RESOURCE_DESC bufferDesc = {};
	bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufferDesc.Width = CPU_VISIBLE_VRAM_BUDGET; //we fill the whole heap with that one buffer
	bufferDesc.Height = 1;
	bufferDesc.DepthOrArraySize = 1;
	bufferDesc.MipLevels = 1;
	bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	bufferDesc.SampleDesc.Count = 1;
	bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	//create the buffer
	if FAILED(device->CreatePlacedResource(
		heap,
		0, //we place that at offset 0
		&bufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&dynamicBuffer)
	))
		return false;

	return true;
}

void DynamicHeap::Destroy()
{
	dynamicBuffer->Release();
	heap->Release();
}

void DynamicHeap::UpdateData(void *data, uint32_t size) const
{
	assert(size <= CPU_VISIBLE_VRAM_BUDGET);

	void *pData;
	CD3DX12_RANGE readRange(0, 0); //zero, the CPU is not reading anything from the GPU
	HRESULT hr = dynamicBuffer->Map(0, &readRange, &pData);
	if (FAILED(hr))
	{
		DebugPrint("Could not access contents of the dynamic buffer!");
		return;
	}

	memcpy(pData, data, size);
	dynamicBuffer->Unmap(0, nullptr);
}

uint64_t DynamicHeap::GetGPUAddress() const
{
	return dynamicBuffer->GetGPUVirtualAddress();
}
