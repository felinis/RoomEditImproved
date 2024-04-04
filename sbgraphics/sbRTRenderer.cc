/*
*	Sabre Engine Graphics - Ray-Tracing Renderer implementation
*	(C) Moczulski Alan, 2023.
*/

#include "sbRTRenderer.h"
#include "shaders/rt.h"

enum GlobalRootSignatureParams
{
	OutputViewSlot,
	AccelerationStructureSlot,
	Count
};

enum LocalRootSignatureParams
{
	ViewportConstantSlot,
	Count
};

static bool SerializeAndCreateRaytracingRootSignature(ID3D12Device* device, D3D12_ROOT_SIGNATURE_DESC& desc, ID3D12RootSignature*& rootSig)
{
	ID3DBlob* blob;
	ID3DBlob* error;

	if FAILED(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &error))
		return false;
	if FAILED(device->CreateRootSignature(1, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&rootSig)))
		return false;
}

// Local root signature and shader association
// This is a root signature that enables a shader to have unique arguments that come from shader tables.
static void CreateLocalRootSignatureSubobjects(CD3DX12_STATE_OBJECT_DESC* raytracingPipeline, ID3D12RootSignature* raytracingLocalRootSignature)
{
	// Hit group and miss shaders in this sample are not using a local root signature and thus one is not associated with them.

	// Local root signature to be used in a ray gen shader.
	{
		auto localRootSignature = raytracingPipeline->CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
		localRootSignature->SetRootSignature(raytracingLocalRootSignature);
		// Shader association
		auto rootSignatureAssociation = raytracingPipeline->CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
		rootSignatureAssociation->SetSubobjectToAssociate(*localRootSignature);
		rootSignatureAssociation->AddExport(L"MyRaygenShader");
	}
}

// Allocate a descriptor and return its index. 
// If the passed descriptorIndexToUse is valid, it will be used instead of allocating a new one.
static UINT AllocateDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE* cpuDescriptor, UINT descriptorIndexToUse, ID3D12DescriptorHeap* descriptorHeap, UINT descriptorSize)
{
	auto descriptorHeapCpuBase = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
//	if (descriptorIndexToUse >= descriptorHeap->GetDesc().NumDescriptors)
//	{
//		descriptorIndexToUse = descriptorsAllocated++;
//	}
	*cpuDescriptor = CD3DX12_CPU_DESCRIPTOR_HANDLE(descriptorHeapCpuBase, descriptorIndexToUse, descriptorSize);
	return descriptorIndexToUse;
}

#define SizeOfInUint32(obj) ((sizeof(obj) - 1) / sizeof(UINT32) + 1)
bool sbRTRenderer::Create(HWND hWnd)
{
	//create the base renderer
	if (!sbBaseRenderer::Create(hWnd))
		return false;

	//we need to load some additional interfaces that support ray-tracing
	if FAILED(device->QueryInterface(IID_PPV_ARGS(&dxrDevice)))
		return false;
	if FAILED(device->QueryInterface(IID_PPV_ARGS(&dxrCommandList)))
		return false;

	//TODO: create descriptor heap here

	// Global Root Signature
	// This is a root signature that is shared across all raytracing shaders invoked during a DispatchRays() call.
	{
		CD3DX12_DESCRIPTOR_RANGE UAVDescriptor;
		UAVDescriptor.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
		CD3DX12_ROOT_PARAMETER rootParameters[GlobalRootSignatureParams::Count];
		rootParameters[GlobalRootSignatureParams::OutputViewSlot].InitAsDescriptorTable(1, &UAVDescriptor);
		rootParameters[GlobalRootSignatureParams::AccelerationStructureSlot].InitAsShaderResourceView(0);
		CD3DX12_ROOT_SIGNATURE_DESC globalRootSignatureDesc(ARRAYSIZE(rootParameters), rootParameters);
		SerializeAndCreateRaytracingRootSignature(device, globalRootSignatureDesc, raytracingGlobalRootSignature);
	}

	// Local Root Signature
	// This is a root signature that enables a shader to have unique arguments that come from shader tables.
	{
		CD3DX12_ROOT_PARAMETER rootParameters[LocalRootSignatureParams::Count];
		rootParameters[LocalRootSignatureParams::ViewportConstantSlot].InitAsConstants(SizeOfInUint32(rayGenCB), 0, 0);
		CD3DX12_ROOT_SIGNATURE_DESC localRootSignatureDesc(ARRAYSIZE(rootParameters), rootParameters);
		localRootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
		SerializeAndCreateRaytracingRootSignature(device, localRootSignatureDesc, raytracingLocalRootSignature);
	}

	{
		// Create 7 subobjects that combine into a RTPSO:
		// Subobjects need to be associated with DXIL exports (i.e. shaders) either by way of default or explicit associations.
		// Default association applies to every exported shader entrypoint that doesn't have any of the same type of subobject associated with it.
		// This simple sample utilizes default shader association except for local root signature subobject
		// which has an explicit association specified purely for demonstration purposes.
		// 1 - DXIL library
		// 1 - Triangle hit group
		// 1 - Shader config
		// 2 - Local root signature and association
		// 1 - Global root signature
		// 1 - Pipeline config
		CD3DX12_STATE_OBJECT_DESC raytracingPipeline{ D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE };


		// DXIL library
		// This contains the shaders and their entrypoints for the state object.
		// Since shaders are not considered a subobject, they need to be passed in via DXIL library subobjects.
		auto lib = raytracingPipeline.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
		D3D12_SHADER_BYTECODE libdxil = CD3DX12_SHADER_BYTECODE((void*)g_pRaytracing, ARRAYSIZE(g_pRaytracing));
		lib->SetDXILLibrary(&libdxil);
		// Define which shader exports to surface from the library.
		// If no shader exports are defined for a DXIL library subobject, all shaders will be surfaced.
		// In this sample, this could be omitted for convenience since the sample uses all shaders in the library. 
		{
			lib->DefineExport(L"MyRaygenShader");
			lib->DefineExport(L"MyClosestHitShader");
			lib->DefineExport(L"MyMissShader");
		}

		// Triangle hit group
		// A hit group specifies closest hit, any hit and intersection shaders to be executed when a ray intersects the geometry's triangle/AABB.
		// In this sample, we only use triangle geometry with a closest hit shader, so others are not set.
		auto hitGroup = raytracingPipeline.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
		hitGroup->SetClosestHitShaderImport(L"MyClosestHitShader");
		hitGroup->SetHitGroupExport(L"MyHitGroup");
		hitGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);

		// Shader config
		// Defines the maximum sizes in bytes for the ray payload and attribute structure.
		auto shaderConfig = raytracingPipeline.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
		UINT payloadSize = 4 * sizeof(float);   // float4 color
		UINT attributeSize = 2 * sizeof(float); // float2 barycentrics
		shaderConfig->Config(payloadSize, attributeSize);

		// Local root signature and shader association
		CreateLocalRootSignatureSubobjects(&raytracingPipeline, raytracingLocalRootSignature);
		// This is a root signature that enables a shader to have unique arguments that come from shader tables.

		// Global root signature
		// This is a root signature that is shared across all raytracing shaders invoked during a DispatchRays() call.
		auto globalRootSignature = raytracingPipeline.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
		globalRootSignature->SetRootSignature(raytracingGlobalRootSignature);

		// Pipeline config
		// Defines the maximum TraceRay() recursion depth.
		auto pipelineConfig = raytracingPipeline.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
		// PERFOMANCE TIP: Set max recursion depth as low as needed 
		// as drivers may apply optimization strategies for low recursion depths. 
		UINT maxRecursionDepth = 1; // ~ primary rays only. 
		pipelineConfig->Config(maxRecursionDepth);

#if _DEBUG
//		PrintStateObjectDesc(raytracingPipeline);
#endif

		//create the state object
		if FAILED(dxrDevice->CreateStateObject(raytracingPipeline, IID_PPV_ARGS(&dxrStateObject)))
			return false;
	}

	//create descriptor heap
	{
		D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
		// Allocate a heap for a single descriptor:
		// 1 - raytracing output texture UAV
		descriptorHeapDesc.NumDescriptors = 1;
		descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		descriptorHeapDesc.NodeMask = 0;
		device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap));

		descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	//build shader tables
	{
		void* rayGenShaderIdentifier;
		void* missShaderIdentifier;
		void* hitGroupShaderIdentifier;

		auto GetShaderIdentifiers = [&](auto* stateObjectProperties)
		{
			rayGenShaderIdentifier = stateObjectProperties->GetShaderIdentifier(L"MyRaygenShader");
			missShaderIdentifier = stateObjectProperties->GetShaderIdentifier(L"MyMissShader");
			hitGroupShaderIdentifier = stateObjectProperties->GetShaderIdentifier(L"MyHitGroup");
		};

		// Get shader identifiers.
		UINT shaderIdentifierSize;
		{
			ID3D12StateObjectProperties* stateObjectProperties;
			if FAILED(dxrStateObject->QueryInterface(IID_PPV_ARGS(&stateObjectProperties)))
				return false;
			GetShaderIdentifiers(stateObjectProperties);
			shaderIdentifierSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
			stateObjectProperties->Release();
		}

		// Ray gen shader table
		{
			struct RootArguments
			{
				RayGenConstantBuffer cb;
			} rootArguments;
			rootArguments.cb = rayGenCB;

			UINT numShaderRecords = 1;
			UINT shaderRecordSize = shaderIdentifierSize + sizeof(rootArguments);
			ShaderTable rayGenShaderTable(device, numShaderRecords, shaderRecordSize, L"RayGenShaderTable");
			rayGenShaderTable.push_back(ShaderRecord(rayGenShaderIdentifier, shaderIdentifierSize, &rootArguments, sizeof(rootArguments)));
			this->rayGenShaderTable = rayGenShaderTable.GetResource();
		}

		// Miss shader table
		{
			UINT numShaderRecords = 1;
			UINT shaderRecordSize = shaderIdentifierSize;
			ShaderTable missShaderTable(device, numShaderRecords, shaderRecordSize, L"MissShaderTable");
			missShaderTable.push_back(ShaderRecord(missShaderIdentifier, shaderIdentifierSize));
			this->missShaderTable = missShaderTable.GetResource();
		}

		// Hit group shader table
		{
			UINT numShaderRecords = 1;
			UINT shaderRecordSize = shaderIdentifierSize;
			ShaderTable hitGroupShaderTable(device, numShaderRecords, shaderRecordSize, L"HitGroupShaderTable");
			hitGroupShaderTable.push_back(ShaderRecord(hitGroupShaderIdentifier, shaderIdentifierSize));
			this->hitGroupShaderTable = hitGroupShaderTable.GetResource();
		}
	}

	//create 2D output texture for RT
	{
		auto backbufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

		RECT r;
		GetClientRect(hWnd, &r);

		// Create the output resource. The dimensions and format should match the swap-chain.
		auto uavDesc = CD3DX12_RESOURCE_DESC::Tex2D(backbufferFormat, r.right, r.bottom, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

		auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		if FAILED(device->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &uavDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&raytracingOutput)))
			return false;

		D3D12_CPU_DESCRIPTOR_HANDLE uavDescriptorHandle;
		raytracingOutputResourceUAVDescriptorHeapIndex = AllocateDescriptor(&uavDescriptorHandle, raytracingOutputResourceUAVDescriptorHeapIndex, descriptorHeap, descriptorSize);
		D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
		UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		device->CreateUnorderedAccessView(raytracingOutput, nullptr, &UAVDesc, uavDescriptorHandle);
		raytracingOutputResourceUAVGpuDescriptor = CD3DX12_GPU_DESCRIPTOR_HANDLE(descriptorHeap->GetGPUDescriptorHandleForHeapStart(), raytracingOutputResourceUAVDescriptorHeapIndex, descriptorSize);
	}

	return true;
}

void sbRTRenderer::Destroy()
{
}

bool sbRTRenderer::StartFrame(Matrix& viewProjection)
{
	if (!sbBaseRenderer::StartFrame())
		return false;

//	swapChain.ClearRenderTarget(commandList);

//	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

//	commandList->SetGraphicsRootSignature(rootSignature);
//	commandList->SetPipelineState(pipelineState);

//	descriptorHeap.Use(commandList);
//	descriptorHeap.SetViewProjectionMatrix(commandList, viewProjection);

	auto DispatchRays = [&](ID3D12GraphicsCommandList4* commandList, ID3D12StateObject* stateObject, D3D12_DISPATCH_RAYS_DESC* dispatchDesc)
	{
		//since each shader table has only one shader record, the stride is same as the size
		dispatchDesc->HitGroupTable.StartAddress = hitGroupShaderTable->GetGPUVirtualAddress();
		dispatchDesc->HitGroupTable.SizeInBytes = hitGroupShaderTable->GetDesc().Width;
		dispatchDesc->HitGroupTable.StrideInBytes = dispatchDesc->HitGroupTable.SizeInBytes;
		dispatchDesc->MissShaderTable.StartAddress = missShaderTable->GetGPUVirtualAddress();
		dispatchDesc->MissShaderTable.SizeInBytes = missShaderTable->GetDesc().Width;
		dispatchDesc->MissShaderTable.StrideInBytes = dispatchDesc->MissShaderTable.SizeInBytes;
		dispatchDesc->RayGenerationShaderRecord.StartAddress = rayGenShaderTable->GetGPUVirtualAddress();
		dispatchDesc->RayGenerationShaderRecord.SizeInBytes = rayGenShaderTable->GetDesc().Width;
		dispatchDesc->Width = 800;
		dispatchDesc->Height = 600;
		dispatchDesc->Depth = 1;
		commandList->SetPipelineState1(stateObject);
		commandList->DispatchRays(dispatchDesc);
	};

	commandList->SetComputeRootSignature(raytracingGlobalRootSignature);

	//bind the heaps, acceleration structure and dispatch rays
	D3D12_DISPATCH_RAYS_DESC dispatchDesc = {};
	commandList->SetDescriptorHeaps(1, &descriptorHeap);
	commandList->SetComputeRootDescriptorTable(GlobalRootSignatureParams::OutputViewSlot, raytracingOutputResourceUAVGpuDescriptor);
//	commandList->SetComputeRootShaderResourceView(GlobalRootSignatureParams::AccelerationStructureSlot, topLevelAccelerationStructure->GetGPUVirtualAddress());
//	DispatchRays(dxrCommandList, dxrStateObject, &dispatchDesc);

	return true;
}

void sbRTRenderer::EndAndPresentFrame()
{
	//copy RT output to backbuffer


	sbBaseRenderer::EndAndPresentFrame();
}
