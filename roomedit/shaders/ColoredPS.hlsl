#include "globals.hlsl"

float4 PSMain(PSColoredInput input) : SV_TARGET
{
	//just use given color
    return float4(input.color, 1.0f);
}
