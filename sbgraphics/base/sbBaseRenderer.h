#pragma once

//avoid parsing a lot of rarely-used stuff
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <d3d12.h>
#include <dxgi1_6.h> //DXGI_SWAP_CHAIN_DESC1
#include "d3dx12.h"

#ifndef NDEBUG
#define DebugPrint(x) {OutputDebugString(x); DebugBreak();}
#else
#define DebugPrint(x)
#endif

static constexpr UINT64 ONE_MEGABYTE = 1024 * 1024;

/*
*	Manual video memory management.
*/
class sbHeap
{
	//default, fastest heap
	static constexpr UINT64 VRAM_BUDGET = ONE_MEGABYTE * 64;
	ID3D12Heap *heap = nullptr;
	UINT64 currentOffset = 0; //resource offset in the heap, the current VRAM pointer
	UINT64 safeOffset = 0;

	//upload heap, used exclusively for uploading data to the default, fastest heap
	static constexpr UINT64 UPLOAD_VRAM_BUDGET = ONE_MEGABYTE * 8;
	ID3D12Heap *uploadHeap = nullptr;

	//align the current offset to 64 KiB (D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT)
	UINT64 AlignOffset(UINT64 offset, UINT64 alignment)
	{
		UINT64 a = alignment - 1;
		return ((offset + a) & ~a);
	}

	bool sbHeap::ImmediatelyUploadToHeap(
		ID3D12Device *device,
		const D3D12_RESOURCE_DESC *desc,
		const D3D12_RESOURCE_ALLOCATION_INFO *ai,
		const void *data,
		D3D12_RESOURCE_STATES initialStates,
		ID3D12Resource **resource
	);

	bool sbHeap::ImmediatelyUploadTextureToHeap(
		ID3D12Device *device,
		const D3D12_RESOURCE_DESC *desc,
		const D3D12_RESOURCE_ALLOCATION_INFO *ai,
		const void *data,
		UINT dataSize,
		D3D12_RESOURCE_STATES initialStates,
		ID3D12Resource *texture
	);

public:
	bool Create(ID3D12Device *device);
	void Destroy();
#if 0
	bool sbHeap::CreateResource(
		ID3D12Device *device,
		const D3D12_RESOURCE_DESC *desc,
		D3D12_RESOURCE_STATES initialState,
		const D3D12_CLEAR_VALUE *clearValue,
		ID3D12Resource **resource
	);

	bool sbHeap::CreateAndFillResource(
		ID3D12Device *device,
		const D3D12_RESOURCE_DESC *desc,
		D3D12_RESOURCE_STATES initialState,
		const D3D12_CLEAR_VALUE *clearValue,
		ID3D12Resource **resource
	);

	bool AllocateBufferOnUploadHeap(
		ID3D12Device *device,
		UINT64 size,
		D3D12_RESOURCE_STATES initialStates,
		ID3D12Resource **resource
	);
#endif
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
};

class sbSwapChain
{
	static constexpr UINT NUM_RENDER_TARGETS = 2; //double-buffered

	//this is the queue we submit the recorded commands to
	ID3D12CommandQueue *queue = nullptr;

	IDXGISwapChain3 *swapChain = nullptr;

	//render targets
	ID3D12DescriptorHeap *rtvDescriptorHeap = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[NUM_RENDER_TARGETS] = {};
	ID3D12Resource *renderTargets[NUM_RENDER_TARGETS] = {};

	//depth buffer
	ID3D12DescriptorHeap *dsvDescriptorHeap = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = {};
	ID3D12Resource *depthBuffer = nullptr;

	//synchronisation objects
	UINT64 frameIndex = 0;
	UINT64 fenceValues[NUM_RENDER_TARGETS] = {};
	ID3D12Fence *fence = nullptr;
	HANDLE fenceEvent = nullptr;

	D3D12_VIEWPORT viewport = {};
	D3D12_RECT scissorRect = {};

public:
	bool Create(ID3D12Device *device, HWND hWnd, IDXGIFactory6 *factory, sbHeap &heap);
	void Destroy();

	void Resize(UINT width, UINT height);

	void SetViewport(ID3D12GraphicsCommandList *commandList);

	void TransitionToRenderable(ID3D12GraphicsCommandList *commandList);
	void TransitionToPresentable(ID3D12GraphicsCommandList *commandList);

	void BindRenderTarget(ID3D12GraphicsCommandList *commandList);
	void ClearRenderTarget(ID3D12GraphicsCommandList *commandList);
	void ExecuteAndPresent(UINT numCommandLists, ID3D12CommandList *const *commandLists);
	void WaitForGpu();
	void MoveToNextFrame();
};

/*
*	Default rendering interface.
*/
class sbBaseRenderer
{
protected:
	IDXGIFactory6 *factory = nullptr;
	ID3D12Device *device = nullptr;

	sbHeap heap;
	sbSwapChain swapChain;

	//we will use these to record commands
	ID3D12CommandAllocator *commandAllocator = nullptr;
	ID3D12GraphicsCommandList *commandList = nullptr;

public:
	bool Create(HWND hWnd);
	void Destroy();

	bool StartFrame();
	void EndAndPresentFrame();
};
