#include "globals.hlsl"

PSPositionColorInput VSMain(VSPositionColorInput input)
{
	PSPositionColorInput output;
	output.position = mul(float4(input.position, 1.0f), viewProjection);
	output.texcoordDiffuse = input.texcoordDiffuse;
	output.texcoordLightmap = input.texcoordLightmap;
	return output;
}
