#pragma once
#include "sbgraphics/sbRenderer.hh"

//defines the PSO for colored surfaces
class ColoredSurfaceGraphicsPipeline final: public Pipeline
{
public:
	bool Create(ID3D12Device *device, ID3D12RootSignature *rootSignature, ID3DBlob *blob) override;
	void Use(ID3D12GraphicsCommandList *commandList) const override;
};
