Texture2D BindlessTextures[] : register(t0, space0);
SamplerState BindlessSampler : register(s0, space0);

cbuffer ViewProjectionRootConstants : register(b0)
{
	row_major float4x4 viewProjection;
};

cbuffer TextureIndexRootConstants : register(b1)
{
    uint texindexDiffuse;
    uint texindexLightmap;
};

//================================
// LIGHTMAP SHADER
//===================
struct VSLightMappedInput
{
	float3 position : POSITION;
	float2 texcoordDiffuse : TEXCOORD0;
	float2 texcoordLightmap : TEXCOORD1;
};

struct PSLightMappedInput
{
	float4 position : SV_POSITION;
	float2 texcoordDiffuse : TEXCOORD0;
	float2 texcoordLightmap : TEXCOORD1;
};

//================================
// PHONG SHADER
//===================
struct VSPhongInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texcoordDiffuse : TEXCOORD;
};

struct PSPhongInput
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 texcoordDiffuse : TEXCOORD;
};

//================================
// COLORED SHADER
//===================
struct VSColoredInput
{
    float3 position : POSITION;
    float3 color : COLOR;
};

struct PSColoredInput
{
    float4 position : SV_POSITION;
    float3 color : COLOR;
};

float4 ConvertLinearToSRGB(float4 color : COLOR) : COLOR
{
//    color.rgb = pow(color.rgb, 1.0 / 2.2);
    return color;
}
