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
}

struct VSPositionColorInput
{
	float3 position : POSITION;
	float2 texcoordDiffuse : TEXCOORD0;
	float2 texcoordLightmap : TEXCOORD1;
};

struct PSPositionColorInput
{
	float4 position : SV_POSITION;
	float2 texcoordDiffuse : TEXCOORD0;
	float2 texcoordLightmap : TEXCOORD1;
};
