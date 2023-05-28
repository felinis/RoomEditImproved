#include "globals.hlsl"

float4 PSMain(PSPositionColorInput input) : SV_TARGET
{
	float4 diffuse = BindlessTextures[texindexDiffuse].Sample(BindlessSampler, input.texcoordDiffuse);
	float4 light = BindlessTextures[texindexLightmap].Sample(BindlessSampler, input.texcoordLightmap);

	return diffuse * light;
}
