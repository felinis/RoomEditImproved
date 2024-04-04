#include "globals.hlsl"

float4 PSMain(PSPhongInput input) : SV_TARGET
{
	//sample diffuse texture and extract color and alpha
	float4 diffuse = BindlessTextures[texindexDiffuse].Sample(BindlessSampler, input.texcoordDiffuse);
	float3 color = diffuse.rgb;
	float alpha = diffuse.a;
   
	//compute light contribution given a normal vector
//	float lightContribution = dot(input.normal, float3(0.0f, 0.0f, 1.0f));
    float lightContribution = 1.0f;
	
    return ConvertLinearToSRGB(float4(color * lightContribution, alpha));
}
