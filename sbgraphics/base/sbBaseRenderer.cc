////////////////////////////////////////////////////////////////////////////////////////////////
//	Sabre Engine Graphics - Direct3D 12 implementation
//	(C) Moczulski Alan, 2023.
////////////////////////////////////////////////////////////////////////////////////////////////

#include "sbBaseRenderer.hh"
#include "engineFeatures.hh"
#include <assert.h>

////////////////////////////////////////////////////////////////////////////////////////////////

bool sbSwapChain::Create(ID3D12Device *device, HWND hWnd, IDXGIFactory6 *factory, HeapManager &heapManager)
{
	assert(device && "Invalid ID3D12Device!");

	//create command queue
	{
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		if FAILED(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&queue)))
		{
			DebugPrint("Failed to create command queue.");
			return false;
		}
	}

	//some arbitrary swap-chain width and height
	//in fact it is recommended by Microsoft to resize the swap-chain
	//to much higher values later on (on WM_SIZE event, for example)
	constexpr uint32_t width = 256;
	constexpr uint32_t height = 256;

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

		//create depth buffer of big enough size
		if (!heapManager.AllocateImage(
			device,
			DXGI_FORMAT_D32_FLOAT, //recommended depth format by nVidia
			MAX_RESOLUTION_WIDTH,
			MAX_RESOLUTION_HEIGHT,
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

	for (UINT i = 0; i < NUM_RENDER_TARGETS; ++i)
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

void sbSwapChain::Resize(ID3D12Device *device, HeapManager &heapManager, UINT width, UINT height)
{
	assert(device && "Invalid ID3D12Device!");
	assert(width > 0 && height > 0 && "Invalid render area sizes!");

	if (width > MAX_RESOLUTION_WIDTH || height > MAX_RESOLUTION_HEIGHT)
		return;

//	WaitForGpu();

	//release render target views
	for (UINT i = 0; i < NUM_RENDER_TARGETS; ++i)
	{
		if (renderTargets[i])
			renderTargets[i]->Release();
	}

	//resize the swapchain
	swapChain->ResizeBuffers(NUM_RENDER_TARGETS, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);

	//recreate render target views
	UINT rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	for (UINT i = 0; i < NUM_RENDER_TARGETS; ++i)
	{
		rtvHandles[i] = rtvHandle;
		swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i]));
		device->CreateRenderTargetView(renderTargets[i], nullptr, rtvHandle);
		rtvHandle.Offset(1, rtvDescriptorSize);
	}

	viewport.Width = (float)width;
	viewport.Height = (float)height;
	viewport.MaxDepth = 1.0f;
	scissorRect.right = width;
	scissorRect.bottom = height;

	MoveToNextFrame();
}

void sbSwapChain::SetViewport(ID3D12GraphicsCommandList *commandList) const
{
	commandList->RSSetViewports(1, &viewport);
	commandList->RSSetScissorRects(1, &scissorRect);
}

void sbSwapChain::TransitionToRenderable(ID3D12GraphicsCommandList *commandList) const
{
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[frameIndex], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
}

void sbSwapChain::TransitionToPresentable(ID3D12GraphicsCommandList *commandList) const
{
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[frameIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
}

void sbSwapChain::BindRenderTarget(ID3D12GraphicsCommandList *commandList) const
{
	commandList->OMSetRenderTargets(1, &rtvHandles[frameIndex], FALSE, &dsvHandle);
}

void sbSwapChain::ClearRenderTarget(ID3D12GraphicsCommandList *commandList) const
{
	const float color[4] = { 0.3f, 0.3f, 0.3f, 1.0f };
	commandList->ClearRenderTargetView(rtvHandles[frameIndex], color, 0, nullptr);

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

////////////////////////////////////////////////////////////////////////////////////////////////

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

	if (!heapManager.Create(device))
		return false;

	if (!swapChain.Create(device, hWnd, factory, heapManager))
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
	heapManager.Destroy();

	if (device)
		device->Release();
	if (factory)
		factory->Release();

#if 0
	ID3D12DebugDevice *debugInterface;
	if (SUCCEEDED(device->QueryInterface(&debugInterface)))
	{
		debugInterface->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);
		debugInterface->Release();
	}
#endif
}

bool sbBaseRenderer::IsReady() const
{
	return commandList != nullptr;
}

bool sbBaseRenderer::StartFrame() const
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

void sbBaseRenderer::ResizeFramebuffer(UINT width, UINT height)
{
	swapChain.Resize(device, heapManager, width, height);
}

void sbBaseRenderer::DrawDynamic(void *verts, uint32_t numVertices, uint32_t vertexSize, uint32_t *indices, uint32_t numIndices) const
{
	auto verticesSize = numVertices * vertexSize;
	auto indicesSize = numIndices * sizeof(uint32_t);
//	assert((char*)verts + verticesSize == (char*)indices); //TODO: vertices must be followed by indices, since we have only one buffer

	auto bufferAddress = heapManager.UploadToDynamicBuffer(verts, verticesSize + indicesSize);

	D3D12_VERTEX_BUFFER_VIEW vbv{};
	vbv.BufferLocation = bufferAddress;
	vbv.StrideInBytes = vertexSize;
	vbv.SizeInBytes = verticesSize;
	commandList->IASetVertexBuffers(0, 1, &vbv);

//	D3D12_INDEX_BUFFER_VIEW ibv{};
//	ibv.BufferLocation = vbv.BufferLocation + vbv.SizeInBytes; //just after the vertex buffer
//	ibv.SizeInBytes = indicesSize;
//	ibv.Format = DXGI_FORMAT_R32_UINT;
//	commandList->IASetIndexBuffer(&ibv);

//	commandList->DrawIndexedInstanced(numIndices, 1, 0, 0, 0);
	commandList->DrawInstanced(numVertices, 1, 0, 0);
}
