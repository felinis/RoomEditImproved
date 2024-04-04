/*
*	Room Editor Application
*	(C) Moczulski Alan, 2023.
*/

#include "ColoredSurfaceGraphicsPipeline.hh"
#include "compiled/ColoredVS.h"
#include "compiled/ColoredPS.h"

bool ColoredSurfaceGraphicsPipeline::Create(ID3D12Device *device, ID3D12RootSignature *rootSignature, ID3DBlob *blob)
{
	//define the vertex input layout
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = rootSignature;

	psoDesc.VS.pShaderBytecode = g_VSMain;
	psoDesc.VS.BytecodeLength = sizeof(g_VSMain);
	psoDesc.PS.pShaderBytecode = g_PSMain;
	psoDesc.PS.BytecodeLength = sizeof(g_PSMain);

	struct ALPHA_ENABLED_BLEND_DESC : public D3D12_BLEND_DESC
	{
		ALPHA_ENABLED_BLEND_DESC()
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
		MY_RASTERIZER_DESC()
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
		MY_DEPTH_STENCIL_DESC()
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

	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;

	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

	psoDesc.SampleDesc.Count = 1;
	//TODO: FIX THIS!!!
//	psoDesc.CachedPSO.pCachedBlob = blob;
//	psoDesc.CachedPSO.CachedBlobSizeInBytes = blob->GetBufferSize();

	return SUCCEEDED(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState)));
}

void ColoredSurfaceGraphicsPipeline::Use(ID3D12GraphicsCommandList *commandList) const
{
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
	commandList->SetPipelineState(pipelineState);
}
