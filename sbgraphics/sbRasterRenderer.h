#pragma once
#include "base/sbBaseRenderer.h"
#include "matrix.inl"

struct Vertex
{
	float position[3] = {};
	float texcoordDiffuse[2] = {};
	float texcoordLightmap[2] = {};
};

//this is the device-specific resource (used for meshes and textures)
typedef void* GPUResource;

/*
*	A mesh residing on the GPU heap.
*/
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


//TODO: rename to "sbTextureDescriptorHeap"
class sbDescriptorHeap
{
	//defines the maximum number of textures that we support
	static constexpr uint32_t MAX_NUM_TEXTURES = 512;

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

	void SetViewProjectionMatrix(ID3D12GraphicsCommandList *commandList, Matrix &m);
	void SetTexture(ID3D12GraphicsCommandList *commandList, uint32_t texindexDiffuse, uint32_t texindexLightmap);
	void Use(ID3D12GraphicsCommandList *commandList);

	D3D12_CPU_DESCRIPTOR_HANDLE CreateSRV(ID3D12Device *device, DXGI_FORMAT format, uint32_t index, ID3D12Resource *texture);
};

class sbRasterRenderer final : public sbBaseRenderer
{
	sbDescriptorHeap descriptorHeap;

	ID3D12RootSignature *rootSignature = nullptr;
	ID3D12PipelineState *pipelineState = nullptr;

	//the index of the fallback texture
	static constexpr uint32_t FALLBACK_TEXTURE_INDEX = 0;
	GPUResource fallbackTexture;

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
	bool StartFrame(Matrix &viewProjection);
	void EndAndPresentFrame();

	//meshes
	sbMesh CreateMesh(const Vertex *verts, uint32_t numVerts, const uint32_t *indices, uint32_t numIndices);
	void DestroyMesh(sbMesh &mesh);
	//TODO: rename to "UseMesh"
	void BindMesh(const sbMesh &mesh);
	void DrawBoundMesh(uint32_t numIndices, uint32_t startIndex = 0);

	//textures
	GPUResource CreateTexture(const void *data, uint32_t dataSize, uint32_t width, uint32_t height, DXGI_FORMAT format, uint32_t index);
	void DestroyTexture(GPUResource &texture);
	void UseDiffuseAndLightmap(uint32_t texindexDiffuse, uint32_t texindexLightmap);
	void UseFullbrightDiffuse(uint32_t texindexDiffuse);
	void UseFallbackDiffuseAndLightmap();
};
