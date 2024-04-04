#pragma once
#include "base/sbBaseRenderer.hh"
#include "matrix.inl"
#include <assert.h>

class GpuUploadBuffer
{
public:
	ID3D12Resource* GetResource()
	{
		return m_resource;
	}

protected:
	ID3D12Resource* m_resource;

	GpuUploadBuffer()
	{
	}
	~GpuUploadBuffer()
	{
		if (m_resource)
		{
			m_resource->Unmap(0, nullptr);
			m_resource->Release();
		}
	}

	void Allocate(ID3D12Device* device, UINT bufferSize, LPCWSTR resourceName = nullptr)
	{
		auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

		auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
		if FAILED(device->CreateCommittedResource(
			&uploadHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_resource)))
			assert(0);
		m_resource->SetName(resourceName);
	}

	uint8_t* MapCpuWriteOnly()
	{
		uint8_t* mappedData;
		// We don't unmap this until the app closes. Keeping buffer mapped for the lifetime of the resource is okay.
		CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
		if FAILED(m_resource->Map(0, &readRange, reinterpret_cast<void**>(&mappedData)))
			assert(0);
		return mappedData;
	}
};

// Shader record = {{Shader ID}, {RootArguments}}
class ShaderRecord
{
public:
	ShaderRecord(void* pShaderIdentifier, UINT shaderIdentifierSize) :
		shaderIdentifier(pShaderIdentifier, shaderIdentifierSize)
	{
	}

	ShaderRecord(void* pShaderIdentifier, UINT shaderIdentifierSize, void* pLocalRootArguments, UINT localRootArgumentsSize) :
		shaderIdentifier(pShaderIdentifier, shaderIdentifierSize),
		localRootArguments(pLocalRootArguments, localRootArgumentsSize)
	{
	}

	void CopyTo(void* dest) const
	{
		uint8_t* byteDest = static_cast<uint8_t*>(dest);
		memcpy(byteDest, shaderIdentifier.ptr, shaderIdentifier.size);
		if (localRootArguments.ptr)
		{
			memcpy(byteDest + shaderIdentifier.size, localRootArguments.ptr, localRootArguments.size);
		}
	}

	struct PointerWithSize
	{
		void* ptr;
		UINT size;

		PointerWithSize() : ptr(nullptr), size(0)
		{
		}
		PointerWithSize(void* _ptr, UINT _size) : ptr(_ptr), size(_size)
		{
		};
	};
	PointerWithSize shaderIdentifier;
	PointerWithSize localRootArguments;
};

// Shader table = {{ ShaderRecord 1}, {ShaderRecord 2}, ...}
class ShaderTable : public GpuUploadBuffer
{
	uint8_t* m_mappedShaderRecords;
	UINT m_shaderRecordSize;

	// Debug support
	std::wstring m_name;
	std::vector<ShaderRecord> m_shaderRecords;

	ShaderTable()
	{
	}
public:
	ShaderTable(ID3D12Device* device, UINT numShaderRecords, UINT shaderRecordSize, LPCWSTR resourceName = nullptr)
		: m_name(resourceName)
	{
		m_shaderRecordSize = Align(shaderRecordSize, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
		m_shaderRecords.reserve(numShaderRecords);
		UINT bufferSize = numShaderRecords * m_shaderRecordSize;
		Allocate(device, bufferSize, resourceName);
		m_mappedShaderRecords = MapCpuWriteOnly();
	}

	void push_back(const ShaderRecord& shaderRecord)
	{
		assert(m_shaderRecords.size() < m_shaderRecords.capacity());
		m_shaderRecords.push_back(shaderRecord);
		shaderRecord.CopyTo(m_mappedShaderRecords);
		m_mappedShaderRecords += m_shaderRecordSize;
	}

	UINT GetShaderRecordSize()
	{
		return m_shaderRecordSize;
	}
};

class sbRTRenderer final : public sbBaseRenderer
{
	ID3D12Device5* dxrDevice;
	ID3D12GraphicsCommandList4* dxrCommandList;

	ID3D12RootSignature* raytracingGlobalRootSignature;
	ID3D12RootSignature* raytracingLocalRootSignature;

	struct Dimensions
	{
		uint32_t x, y;
	};

	// Raytracing scene
	struct RayGenConstantBuffer
	{
		Dimensions textureDim;
		BOOL doCheckerboardRayGeneration;
		BOOL checkerboardGenerateRaysForEvenPixels;

		UINT seed;
		UINT numSamplesPerSet;
		UINT numSampleSets;
		UINT numPixelsPerDimPerSet;
	};
	RayGenConstantBuffer rayGenCB;

	ID3D12StateObject* dxrStateObject;

	ID3D12DescriptorHeap* descriptorHeap;
	UINT descriptorSize;

	ID3D12Resource* rayGenShaderTable;
	ID3D12Resource* missShaderTable;
	ID3D12Resource* hitGroupShaderTable;

	ID3D12Resource* raytracingOutput;
	UINT raytracingOutputResourceUAVDescriptorHeapIndex;
	D3D12_GPU_DESCRIPTOR_HANDLE raytracingOutputResourceUAVGpuDescriptor;

public:
	bool Create(HWND hWnd);
	void Destroy();

	bool StartFrame(Matrix& viewProjection);
	void EndAndPresentFrame();
};
