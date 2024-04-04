#include "globals.hlsl"

float4 PSMain(PSLightMappedInput input) : SV_TARGET
{
	float4 diffuse = BindlessTextures[texindexDiffuse].Sample(BindlessSampler, input.texcoordDiffuse);
    float3 color = diffuse.rgb;
    float alpha = diffuse.a;

	float4 light = BindlessTextures[texindexLightmap].Sample(BindlessSampler, input.texcoordLightmap);
    float3 lightColor = light.rgb;

    return ConvertLinearToSRGB(float4(color * lightColor, alpha));
}
