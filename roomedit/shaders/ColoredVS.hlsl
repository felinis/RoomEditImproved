#include "globals.hlsl"

PSColoredInput VSMain(VSColoredInput input)
{
    PSColoredInput output;
	output.position = mul(float4(input.position, 1.0f), viewProjection);
    output.color = input.color;
	return output;
}
