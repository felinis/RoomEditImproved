#pragma once

#include "HeapManager.hh"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <d3d12.h>
#include <dxgi1_6.h> //DXGI_SWAP_CHAIN_DESC1
#include "d3dx12.hh"

#ifndef NDEBUG
#define DebugPrint(x) {OutputDebugString(x); DebugBreak();}
#else
#define DebugPrint(x)
#endif

class sbSwapChain
{
	static constexpr UINT NUM_RENDER_TARGETS = 2; //double-buffered

	//resolution limits, so that we know how much VRAM to allocate on the heap
	//for the swapchain images
	static constexpr UINT MAX_RESOLUTION_WIDTH = 3840;
	static constexpr UINT MAX_RESOLUTION_HEIGHT = 2160;

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
	bool Create(ID3D12Device *device, HWND hWnd, IDXGIFactory6 *factory, HeapManager &heapManager);
	void Destroy();

	void Resize(ID3D12Device *device, HeapManager &heapManager, UINT width, UINT height);

	void SetViewport(ID3D12GraphicsCommandList *commandList) const;

	void TransitionToRenderable(ID3D12GraphicsCommandList *commandList) const;
	void TransitionToPresentable(ID3D12GraphicsCommandList *commandList) const;

	void BindRenderTarget(ID3D12GraphicsCommandList *commandList) const;
	void ClearRenderTarget(ID3D12GraphicsCommandList *commandList) const;
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

	HeapManager heapManager;
	sbSwapChain swapChain;

	//we will use these to record commands
	ID3D12CommandAllocator *commandAllocator = nullptr;
	ID3D12GraphicsCommandList *commandList = nullptr;

public:
	bool Create(HWND hWnd);
	void Destroy();

	bool IsReady() const;

	bool StartFrame() const;
	void EndAndPresentFrame();

	void ResizeFramebuffer(UINT width, UINT height);

	//draws mesh directly
	void DrawDynamic(void *verts, uint32_t numVertices, uint32_t vertexSize, uint32_t *indices, uint32_t numIndices) const;
};
