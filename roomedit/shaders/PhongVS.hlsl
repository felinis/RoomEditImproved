#include "globals.hlsl"

PSPhongInput VSMain(VSPhongInput input)
{
    PSPhongInput output;
	output.position = mul(float4(input.position, 1.0f), viewProjection);
    output.normal = input.normal;
	output.texcoordDiffuse = input.texcoordDiffuse;
	return output;
}
