/*
*	Room Editor Application
*	(C) Moczulski Alan, 2023.
*/

#include "LightMappedSurfaceGraphicsPipeline.hh"
#include "compiled/LightMappedVS.h"
#include "compiled/LightMappedPS.h"

bool LightMappedSurfaceGraphicsPipeline::Create(ID3D12Device *device, ID3D12RootSignature *rootSignature, ID3DBlob *blob)
{
	//define the vertex input layout
	constexpr D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
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

	struct ALPHA_ENABLED_BLEND_DESC : public D3D12_BLEND_DESC
	{
		constexpr ALPHA_ENABLED_BLEND_DESC()
		{
			AlphaToCoverageEnable = FALSE;
			IndependentBlendEnable = FALSE;
			const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc =
			{
				TRUE, FALSE,
				D3D12_BLEND_SRC_ALPHA, D3D12_BLEND_INV_SRC_ALPHA, D3D12_BLEND_OP_ADD,
				D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
				D3D12_LOGIC_OP_NOOP,
				D3D12_COLOR_WRITE_ENABLE_ALL,
			};
			for (UINT i = 0; i < 1; i++)
				RenderTarget[i] = defaultRenderTargetBlendDesc;
		}
	};
	psoDesc.BlendState = ALPHA_ENABLED_BLEND_DESC();
	psoDesc.SampleMask = UINT_MAX;

	struct MY_RASTERIZER_DESC : public D3D12_RASTERIZER_DESC
	{
		constexpr MY_RASTERIZER_DESC()
		{
			FillMode = D3D12_FILL_MODE_SOLID;
			CullMode = D3D12_CULL_MODE_BACK;
			FrontCounterClockwise = TRUE;
			DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
			DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
			SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
			DepthClipEnable = TRUE;
			MultisampleEnable = FALSE;
			AntialiasedLineEnable = FALSE;
			ForcedSampleCount = 0;
			ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
		}
	};
	psoDesc.RasterizerState = MY_RASTERIZER_DESC();

	struct MY_DEPTH_STENCIL_DESC : public D3D12_DEPTH_STENCIL_DESC
	{
		constexpr MY_DEPTH_STENCIL_DESC()
		{
			DepthEnable = TRUE;
			DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
			DepthFunc = D3D12_COMPARISON_FUNC_LESS;
			StencilEnable = FALSE;
			StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
			StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
			const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp =
			{ D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
			FrontFace = defaultStencilOp;
			BackFace = defaultStencilOp;
		}
	};
	psoDesc.DepthStencilState = MY_DEPTH_STENCIL_DESC();
	psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };

	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

	psoDesc.SampleDesc.Count = 1;

	return SUCCEEDED(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState)));
}

void LightMappedSurfaceGraphicsPipeline::Use(ID3D12GraphicsCommandList *commandList) const
{
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->SetPipelineState(pipelineState);
}
