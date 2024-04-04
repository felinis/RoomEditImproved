#include "globals.hlsl"

PSLightMappedInput VSMain(VSLightMappedInput input)
{
	PSLightMappedInput output;
	output.position = mul(float4(input.position, 1.0f), viewProjection);
	output.texcoordDiffuse = input.texcoordDiffuse;
	output.texcoordLightmap = input.texcoordLightmap;
	return output;
}
