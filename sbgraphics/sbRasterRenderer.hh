#pragma once
#include "base/sbBaseRenderer.hh"
#include "matrix.inl"
#include <assert.h>

////////////////////////////////////////////////////////////////////////////////////////////////

//describes a graphics shading pipeline
class Pipeline
{
protected:
	ID3D12PipelineState *pipelineState;

public:
	Pipeline():
		pipelineState(nullptr)
	{}

	virtual bool Create(ID3D12Device *device, ID3D12RootSignature *rootSignature, ID3DBlob *blob = nullptr) = 0;
	inline void Destroy()
	{
		if (pipelineState)
			pipelineState->Release();
	}
	virtual void Use(ID3D12GraphicsCommandList *commandList) const = 0;
};

////////////////////////////////////////////////////////////////////////////////////////////////

//this is the device-specific resource (used for textures and vertex/index buffers)
typedef void* GPUResource; //actually ID3D12Resource* for a D3D12 renderer

//describes a mesh residing on the GPU heap
struct sbMesh
{
	GPUResource vertexBuffer;
	GPUResource indexBuffer;
	uint32_t numIndices;

	sbMesh():
		vertexBuffer(nullptr), indexBuffer(nullptr), numIndices(0) {}
	sbMesh(ID3D12Resource *vertexBuffer, ID3D12Resource *indexBuffer, uint32_t numIndices):
		vertexBuffer(vertexBuffer), indexBuffer(indexBuffer), numIndices(numIndices) {}
};

////////////////////////////////////////////////////////////////////////////////////////////////

//TODO: rename to "sbTextureDescriptorHeap"
class sbDescriptorHeap
{
	//defines the maximum number of textures that we support
	static constexpr uint32_t MAX_NUM_TEXTURES = 1024;

	ID3D12DescriptorHeap *srvDescriptorHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE srvDescriptorHandle;
	uint32_t srvDescriptorHandleIncrementSize;

	ID3D12DescriptorHeap *samplerDescriptorHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE samplerDescriptorHandle;
	uint32_t samplerDescriptorHandleIncrementSize;

public:
	sbDescriptorHeap() :
		srvDescriptorHeap(nullptr),
		srvDescriptorHandle(),
		srvDescriptorHandleIncrementSize(0),
		samplerDescriptorHeap(nullptr),
		samplerDescriptorHandle(),
		samplerDescriptorHandleIncrementSize(0)
	{}

	bool Create(ID3D12Device *device);
	void Destroy();

	void SetWorldViewProjectionMatrix(ID3D12GraphicsCommandList *commandList, const Matrix &m);
	void SetTexture(ID3D12GraphicsCommandList *commandList, uint32_t textureIndex0);
	void SetTexture(ID3D12GraphicsCommandList *commandList, uint32_t textureIndex0, uint32_t textureIndex1);
	void Use(ID3D12GraphicsCommandList *commandList);

	D3D12_CPU_DESCRIPTOR_HANDLE CreateSRV(ID3D12Device *device, DXGI_FORMAT format, uint32_t index, ID3D12Resource *texture);
};

////////////////////////////////////////////////////////////////////////////////////////////////

class sbRasterRenderer final : public sbBaseRenderer
{
	sbDescriptorHeap descriptorHeap;

	ID3D12RootSignature *rootSignature = nullptr;

public:
	bool Create(HWND hwnd);
	void Destroy();

	//saves the internal state of the renderer, so we can restore to it any time
	//after creating textures or meshes, for example when loading another level
	//we will restore the state and start loading the new textures and meshes
	void SaveInternalState();

	//restores the state of the renderer like it was from the start
	//no need to defragment GPU memory, or any fancy stuff, it's just about
	//managing GPU memory in a simple and clever way
	void RestoreInternalState();

	//frame
	void ClearAndPresentImmediately();
	bool StartFrame();
	void EndAndPresentFrame();

	//pipelines
	bool CreatePipeline(Pipeline &pipeline) const;
	void DestroyPipeline(Pipeline &pipeline) const;
	void UsePipeline(const Pipeline &pipeline) const;

	//meshes
	template <typename VertexFormat>
	sbMesh CreateMesh(const VertexFormat *verts, uint32_t numVerts, const uint32_t *indices, uint32_t numIndices)
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
			const uint32_t vertexBufferSize = numVerts * sizeof(VertexFormat);
			desc.Width = vertexBufferSize;
			if (!heapManager.AllocateAndFillBuffer(device, &desc, verts, D3D12_RESOURCE_STATE_COPY_DEST, &vertexBuffer))
			{
				assert(0);
				return sbMesh();
			}
		}

		//create and fill index buffer
		ID3D12Resource *indexBuffer;
		{
			const uint32_t indexBufferSize = numIndices * sizeof(uint32_t);
			desc.Width = indexBufferSize;
			if (!heapManager.AllocateAndFillBuffer(device, &desc, indices, D3D12_RESOURCE_STATE_COPY_DEST, &indexBuffer))
			{
				assert(0);
				return sbMesh();
			}
		}

		return std::move(sbMesh(vertexBuffer, indexBuffer, numIndices));
	}
	void DestroyMesh(sbMesh &mesh);

	//TODO: rename to "UseMesh"
	template <typename VertexFormat>
	void BindMesh(const sbMesh &mesh)
	{
		ID3D12Resource *vb = (ID3D12Resource *)mesh.vertexBuffer;
		ID3D12Resource *ib = (ID3D12Resource *)mesh.indexBuffer;

		D3D12_VERTEX_BUFFER_VIEW vbv{};
		vbv.BufferLocation = vb->GetGPUVirtualAddress();
		vbv.StrideInBytes = sizeof(VertexFormat);
		vbv.SizeInBytes = (UINT)vb->GetDesc().Width;
		commandList->IASetVertexBuffers(0, 1, &vbv);

		D3D12_INDEX_BUFFER_VIEW ibv{};
		ibv.BufferLocation = ib->GetGPUVirtualAddress();
		ibv.SizeInBytes = (UINT)ib->GetDesc().Width;
		ibv.Format = DXGI_FORMAT_R32_UINT;
		commandList->IASetIndexBuffer(&ibv);
	}
	void DrawBoundMesh(uint32_t numIndices, uint32_t startIndex = 0);
#if 0
	template <typename VertexFormat>
	void DrawDynamicMesh(const VertexFormat *verts, uint32_t numVerts, const uint32_t *indices, uint32_t numIndices)
	{
		const uint32_t vertexBufferSize = numVerts * sizeof(VertexFormat);
		const uint32_t indexBufferSize = numIndices * sizeof(uint32_t);
		assert(verts + vertexBufferSize == indices); //TEMP: make sure that indices follow vertices, since we use one buffer
		heapManager.DrawDynamicMesh(commandList, verts, vertexBufferSize + indexBufferSize);
	}
#endif
	//textures
	GPUResource CreateTexture(const void *data, uint32_t dataSize, uint32_t width, uint32_t height, DXGI_FORMAT format, uint32_t index);
	void DestroyTexture(GPUResource &texture);

	//transformation
	void SetWorldViewProjectionMatrix(const Matrix &m);
	void UseOneTexture(uint32_t textureIndex0);
	void UseTwoTextures(uint32_t textureIndex0, uint32_t textureIndex1);
};
