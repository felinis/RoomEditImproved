////////////////////////////////////////////////////////////////////////////////////////////////
//	Sabre Engine Graphics - Rasterisation Renderer implementation
//	(C) Moczulski Alan, 2023.
////////////////////////////////////////////////////////////////////////////////////////////////

#include "sbRasterRenderer.hh"

////////////////////////////////////////////////////////////////////////////////////////////////

static constexpr uint32_t TEXTURES_ROOT_PARAM_INDEX = 0;
static constexpr uint32_t SAMPLER_ROOT_PARAM_INDEX = 1;
static constexpr uint32_t MATRIX_ROOT_PARAM_INDEX = 2;
static constexpr uint32_t TEXINDEX_ROOT_PARAM_INDEX = 3;

////////////////////////////////////////////////////////////////////////////////////////////////

bool sbDescriptorHeap::Create(ID3D12Device *device)
{
	//create a descriptor heap for all the SRVs
	{
		//flags indicate that this descriptor heap can be bound to the pipeline 
		//and that descriptors contained in it can be referenced by a root table
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.NumDescriptors = MAX_NUM_TEXTURES; //for textures
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		if FAILED(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&srvDescriptorHeap)))
			return false;
		srvDescriptorHeap->SetName(L"Textures Descriptor Heap");

		srvDescriptorHandle = srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		srvDescriptorHandleIncrementSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}
	
	//create a descriptor heap for the sampler
	{
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.NumDescriptors = 1;
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		if FAILED(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&samplerDescriptorHeap)))
			return false;
		samplerDescriptorHeap->SetName(L"Samplers Descriptor Heap");

		samplerDescriptorHandle = samplerDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		samplerDescriptorHandleIncrementSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	}

	//create the sampler
	{
		D3D12_SAMPLER_DESC samplerDesc = {};
		samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
		samplerDesc.MaxAnisotropy = 1;
		samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;

		//create the sampler in the descriptor heap
		device->CreateSampler(&samplerDesc, samplerDescriptorHandle);

		samplerDescriptorHandle.ptr += samplerDescriptorHandleIncrementSize;
	}

	return true;
}

void sbDescriptorHeap::Destroy()
{
	if (samplerDescriptorHeap)
		samplerDescriptorHeap->Release();
	if (srvDescriptorHeap)
		srvDescriptorHeap->Release();
}

void sbDescriptorHeap::SetWorldViewProjectionMatrix(ID3D12GraphicsCommandList *commandList, const Matrix &m)
{
	commandList->SetGraphicsRoot32BitConstants(MATRIX_ROOT_PARAM_INDEX, 16, m, 0);
}

void sbDescriptorHeap::SetTexture(ID3D12GraphicsCommandList *commandList, uint32_t textureIndex0)
{
	uint32_t data[] = { textureIndex0 };
	commandList->SetGraphicsRoot32BitConstants(TEXINDEX_ROOT_PARAM_INDEX, 1, &data, 0);
}

void sbDescriptorHeap::SetTexture(ID3D12GraphicsCommandList *commandList, uint32_t textureIndex0, uint32_t textureIndex1)
{
	uint32_t data[] = { textureIndex0, textureIndex1 };
	commandList->SetGraphicsRoot32BitConstants(TEXINDEX_ROOT_PARAM_INDEX, 2, &data, 0);
}

void sbDescriptorHeap::Use(ID3D12GraphicsCommandList *commandList)
{
	ID3D12DescriptorHeap *heaps[] = { srvDescriptorHeap, samplerDescriptorHeap };
	commandList->SetDescriptorHeaps(_countof(heaps), heaps);
	commandList->SetGraphicsRootDescriptorTable(TEXTURES_ROOT_PARAM_INDEX, srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	commandList->SetGraphicsRootDescriptorTable(SAMPLER_ROOT_PARAM_INDEX, samplerDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
}

D3D12_CPU_DESCRIPTOR_HANDLE sbDescriptorHeap::CreateSRV(ID3D12Device *device, DXGI_FORMAT format, uint32_t index, ID3D12Resource *texture)
{
	assert(index < MAX_NUM_TEXTURES);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = 1;

	//create a shader resource view at the given offset in memory
	CD3DX12_CPU_DESCRIPTOR_HANDLE handle(srvDescriptorHandle);
	handle.Offset(index, srvDescriptorHandleIncrementSize);
	device->CreateShaderResourceView(texture, &srvDesc, handle); //can also be used to overwrite an already-existing SRV in the heap
	return handle;
}

////////////////////////////////////////////////////////////////////////////////////////////////

bool sbRasterRenderer::Create(HWND hwnd)
{
	//create the base renderer
	if (!sbBaseRenderer::Create(hwnd))
		return false;

	if (!descriptorHeap.Create(device))
		return false;

	//create the root signature
	{
		CD3DX12_DESCRIPTOR_RANGE1 ranges[2];
		ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
		ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);

		CD3DX12_ROOT_PARAMETER1 rootParameters[4];
		//SRV descriptor range for the bindless textures
		rootParameters[TEXTURES_ROOT_PARAM_INDEX].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);
		//sampler for the bindless textures
		rootParameters[SAMPLER_ROOT_PARAM_INDEX].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_PIXEL);

		//CONSTANT BUFFERS (ROOT CONSTANTS)
		//16 floats for the 4x4 view-projection matrix
		rootParameters[MATRIX_ROOT_PARAM_INDEX].InitAsConstants(16, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
		//2 UINTs for texture indices
		rootParameters[TEXINDEX_ROOT_PARAM_INDEX].InitAsConstants(2, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL);

		//allow input layout and deny uneccessary access to certain pipeline stages
		D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, rootSignatureFlags);

		ID3DBlob *signature;
		ID3DBlob *error;
		if FAILED(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_1, &signature, &error))
		{
			return false;
		}
		if FAILED(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature)))
		{
			return false;
		}
	}

	return true;
}

void sbRasterRenderer::Destroy()
{
	if (rootSignature)
		rootSignature->Release();

	descriptorHeap.Destroy();

	//destroy the base renderer
	sbBaseRenderer::Destroy();
}

void sbRasterRenderer::SaveInternalState()
{
	heapManager.SetSafeResetCheckpoint(); //TODO: put this in sbBaseRenderer::SaveInternalState
//	SetSafeResetCheckpoint(fallbackTexture.descriptorHandle);
}

void sbRasterRenderer::RestoreInternalState()
{
	heapManager.Reset(); //TODO: put this in sbBaseRenderer::SaveInternalState
}

void sbRasterRenderer::ClearAndPresentImmediately()
{
	if (!sbBaseRenderer::StartFrame())
		return;

	swapChain.ClearRenderTarget(commandList);

	sbBaseRenderer::EndAndPresentFrame();
}

bool sbRasterRenderer::StartFrame()
{
	if (!sbBaseRenderer::StartFrame())
		return false;

	swapChain.ClearRenderTarget(commandList);

	commandList->SetGraphicsRootSignature(rootSignature);

	descriptorHeap.Use(commandList);

	return true;
}

void sbRasterRenderer::EndAndPresentFrame()
{
	sbBaseRenderer::EndAndPresentFrame();
}

bool sbRasterRenderer::CreatePipeline(Pipeline &pipeline) const
{
	return pipeline.Create(device, rootSignature);
}

void sbRasterRenderer::DestroyPipeline(Pipeline &pipeline) const
{
	pipeline.Destroy();
}

void sbRasterRenderer::UsePipeline(const Pipeline &pipeline) const
{
	pipeline.Use(commandList);
}

void sbRasterRenderer::DestroyMesh(sbMesh &mesh)
{
	if (mesh.vertexBuffer)
		((ID3D12Resource*)mesh.vertexBuffer)->Release();
	if (mesh.indexBuffer)
		((ID3D12Resource*)mesh.indexBuffer)->Release();
}

void sbRasterRenderer::DrawBoundMesh(uint32_t numIndices, uint32_t startIndex)
{
	commandList->DrawIndexedInstanced(numIndices, 1, startIndex, 0, 0);
}

GPUResource sbRasterRenderer::CreateTexture(
	const void *data, uint32_t dataSize, uint32_t width, uint32_t height, DXGI_FORMAT format, uint32_t index
)
{
	ID3D12Resource *texture;
	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Width = width;
	if (desc.Width < 4)
		desc.Width = 4; //D3D12 alignment requirement
	desc.Height = height;
	if (desc.Height < 4)
		desc.Height = 4; //D3D12 alignment requirement
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = format;
	desc.SampleDesc.Count = 1;
	if (!heapManager.AllocateAndFillTexture(device, &desc, data, dataSize, D3D12_RESOURCE_STATE_COPY_DEST, &texture))
		return nullptr;

	//create a shader resource view so that we can access the texture from a shader
	D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandle = descriptorHeap.CreateSRV(device, format, index, texture);

	return texture;
}

void sbRasterRenderer::DestroyTexture(GPUResource& texture)
{
	if (texture)
		((ID3D12Resource*)texture)->Release();
}

void sbRasterRenderer::SetWorldViewProjectionMatrix(const Matrix &m)
{
	descriptorHeap.SetWorldViewProjectionMatrix(commandList, m);
}

void sbRasterRenderer::UseOneTexture(uint32_t textureIndex0)
{
	descriptorHeap.SetTexture(commandList, textureIndex0);
}

void sbRasterRenderer::UseTwoTextures(uint32_t textureIndex0, uint32_t textureIndex1)
{
	descriptorHeap.SetTexture(commandList, textureIndex0, textureIndex1);
}
