/*
*	Sabre Engine Graphics - Rasterisation Renderer implementation
*	(C) Moczulski Alan, 2023.
*/

#include "sbRasterRenderer.h"
#include "shaders/vsScreenColored.h"
#include "shaders/psColor.h"
#include <assert.h>

static constexpr uint32_t TEXTURES_ROOT_PARAM_INDEX = 0;
static constexpr uint32_t SAMPLER_ROOT_PARAM_INDEX = 1;
static constexpr uint32_t MATRIX_ROOT_PARAM_INDEX = 2;
static constexpr uint32_t TEXINDEX_ROOT_PARAM_INDEX = 3;

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

void sbDescriptorHeap::SetViewProjectionMatrix(ID3D12GraphicsCommandList *commandList, Matrix &m)
{
	commandList->SetGraphicsRoot32BitConstants(MATRIX_ROOT_PARAM_INDEX, 16, m, 0);
}

void sbDescriptorHeap::SetTexture(ID3D12GraphicsCommandList *commandList, uint32_t texindexDiffuse, uint32_t texindexLightmap)
{
	uint32_t data[2] = {texindexDiffuse, texindexLightmap};
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

	//create the graphics pipeline state object (PSO)
	{
		//define the vertex input layout
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		/*
		ID3D12RootSignature *pRootSignature;
		D3D12_SHADER_BYTECODE VS;
		D3D12_SHADER_BYTECODE PS;
		D3D12_SHADER_BYTECODE DS;
		D3D12_SHADER_BYTECODE HS;
		D3D12_SHADER_BYTECODE GS;
		D3D12_STREAM_OUTPUT_DESC StreamOutput;
		D3D12_BLEND_DESC BlendState;
		uint32_t SampleMask;
		D3D12_RASTERIZER_DESC RasterizerState;
		D3D12_DEPTH_STENCIL_DESC DepthStencilState;
		D3D12_INPUT_LAYOUT_DESC InputLayout;
		D3D12_INDEX_BUFFER_STRIP_CUT_VALUE IBStripCutValue;
		D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType;
		uint32_t NumRenderTargets;
		DXGI_FORMAT RTVFormats[ 8 ];
		DXGI_FORMAT DSVFormat;
		DXGI_SAMPLE_DESC SampleDesc;
		uint32_t NodeMask;
		D3D12_CACHED_PIPELINE_STATE CachedPSO;
		D3D12_PIPELINE_STATE_FLAGS Flags;
		*/

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = rootSignature;

		psoDesc.VS.pShaderBytecode = g_VSMain;
		psoDesc.VS.BytecodeLength = sizeof(g_VSMain);
		psoDesc.PS.pShaderBytecode = g_PSMain;
		psoDesc.PS.BytecodeLength = sizeof(g_PSMain);

		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.RasterizerState.FrontCounterClockwise = TRUE;
		psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };

		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

		psoDesc.SampleDesc.Count = 1;

		if FAILED(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState)))
		{
			return false;
		}
	}

	//create the fallback texture
	{
		struct RGBAColor
		{
			uint8_t r, g, b, a;
		};
		RGBAColor data[1] =
		{
			255, 255, 255, 255
		};
		fallbackTexture = CreateTexture(data, sizeof(data), 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, FALLBACK_TEXTURE_INDEX);
	}

	return true;
}

void sbRasterRenderer::Destroy()
{
	DestroyTexture(fallbackTexture);

	if (pipelineState)
		pipelineState->Release();
	if (rootSignature)
		rootSignature->Release();

	descriptorHeap.Destroy();

	//destroy the base renderer
	sbBaseRenderer::Destroy();
}

void sbRasterRenderer::SaveInternalState()
{
	heap.SetSafeResetCheckpoint(); //TODO: put this in sbBaseRenderer::SaveInternalState
//	SetSafeResetCheckpoint(fallbackTexture.descriptorHandle);
}

void sbRasterRenderer::RestoreInternalState()
{
	heap.Reset(); //TODO: put this in sbBaseRenderer::SaveInternalState
}

bool sbRasterRenderer::StartFrame(Matrix &viewProjection)
{
	if (!sbBaseRenderer::StartFrame())
		return false;

	swapChain.ClearRenderTarget(commandList);

	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	commandList->SetGraphicsRootSignature(rootSignature);
	commandList->SetPipelineState(pipelineState);

	descriptorHeap.Use(commandList);
	descriptorHeap.SetViewProjectionMatrix(commandList, viewProjection);

//	BindMesh(testCube);
//	DrawBoundMesh(testCube.numIndices);

	return true;
}

void sbRasterRenderer::EndAndPresentFrame()
{
	sbBaseRenderer::EndAndPresentFrame();
}
#if 0
std::optional<sbMesh> sbRasterRenderer::CreateTestMesh()
{
	Vertex verts[] =
	{
		{ { -1.0f, -1.0f, -1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
		{ { -1.0f, -1.0f,  1.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
		{ { -1.0f,  1.0f, -1.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } },
		{ { -1.0f,  1.0f,  1.0f }, { 1.0f, 1.0f, 0.0f, 1.0f } },
		{ {  1.0f, -1.0f, -1.0f }, { 1.0f, 0.0f, 1.0f, 1.0f } },
		{ {  1.0f, -1.0f,  1.0f }, { 0.0f, 1.0f, 1.0f, 1.0f } },
		{ {  1.0f,  1.0f, -1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
		{ {  1.0f,  1.0f,  1.0f }, { 0.5f, 0.5f, 0.5f, 1.0f } },
	};
	uint32_t indices[] =
	{
		0, 1, 2, 1, 3, 2,
		4, 0, 6, 0, 2, 6,
		5, 4, 7, 4, 6, 7,
		1, 5, 3, 5, 7, 3,
		4, 5, 0, 5, 1, 0,
		2, 3, 6, 3, 7, 6,
	};

	return CreateMesh(verts, 8, indices, 36);
}
#endif
sbMesh sbRasterRenderer::CreateMesh(const Vertex *verts, uint32_t numVerts, const uint32_t *indices, uint32_t numIndices)
{
	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Height = 1;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.SampleDesc.Count = 1;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	//create and fill vertex buffer
	ID3D12Resource *vertexBuffer;
	{
		const uint32_t vertexBufferSize = numVerts * sizeof(Vertex);
		desc.Width = vertexBufferSize;
		if (!heap.AllocateAndFillBuffer(device, &desc, verts, D3D12_RESOURCE_STATE_COPY_DEST, &vertexBuffer))
		{
			assert(0);
			return sbMesh();
		}
	}

	//create and fill index buffer
	ID3D12Resource *indexBuffer;
	{
		const uint32_t indexBufferSize = numIndices * sizeof(UINT);
		desc.Width = indexBufferSize;
		if (!heap.AllocateAndFillBuffer(device, &desc, indices, D3D12_RESOURCE_STATE_COPY_DEST, &indexBuffer))
		{
			assert(0);
			return sbMesh();
		}
	}

	return std::move(sbMesh(vertexBuffer, indexBuffer, numIndices));
}

void sbRasterRenderer::DestroyMesh(sbMesh &mesh)
{
	if (mesh.vertexBuffer)
		((ID3D12Resource*)mesh.vertexBuffer)->Release();
	if (mesh.indexBuffer)
		((ID3D12Resource*)mesh.indexBuffer)->Release();
}

void sbRasterRenderer::BindMesh(const sbMesh &mesh)
{
	ID3D12Resource* vb = (ID3D12Resource*)mesh.vertexBuffer;
	ID3D12Resource* ib = (ID3D12Resource*)mesh.indexBuffer;

	D3D12_VERTEX_BUFFER_VIEW vbv{};
	vbv.BufferLocation = vb->GetGPUVirtualAddress();
	vbv.StrideInBytes = sizeof(Vertex);
	vbv.SizeInBytes = vb->GetDesc().Width;
	commandList->IASetVertexBuffers(0, 1, &vbv);

	D3D12_INDEX_BUFFER_VIEW ibv{};
	ibv.BufferLocation = ib->GetGPUVirtualAddress();
	ibv.SizeInBytes = ib->GetDesc().Width;
	ibv.Format = DXGI_FORMAT_R32_UINT;
	commandList->IASetIndexBuffer(&ibv);
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
	desc.Height = height;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = format;
	desc.SampleDesc.Count = 1;
	if (!heap.AllocateAndFillTexture(device, &desc, data, dataSize, D3D12_RESOURCE_STATE_COPY_DEST, &texture))
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

void sbRasterRenderer::UseDiffuseAndLightmap(uint32_t texindexDiffuse, uint32_t texindexLightmap)
{
	descriptorHeap.SetTexture(commandList, texindexDiffuse, texindexLightmap);
}

void sbRasterRenderer::UseFullbrightDiffuse(uint32_t texindexDiffuse)
{
	descriptorHeap.SetTexture(commandList, texindexDiffuse, FALLBACK_TEXTURE_INDEX);
}

void sbRasterRenderer::UseFallbackDiffuseAndLightmap()
{
	descriptorHeap.SetTexture(commandList, FALLBACK_TEXTURE_INDEX, FALLBACK_TEXTURE_INDEX);
}
