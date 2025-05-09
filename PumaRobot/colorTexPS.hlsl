Texture2D colorMap : register(t0);
SamplerState colorSampler : register(s0);

cbuffer cbSurfaceColor : register(b0)
{
	float4 surfaceColor;
}

struct PSInput
{
	float4 pos : SV_POSITION;
	float2 tex: TEXCOORD0;
};

float4 main(PSInput i) : SV_TARGET
{
	// DONE : 0.10 Like in texturedPS.hlsl, sample color from the map, but add it to surfaceColor;
	float4 fragcolor = colorMap.Sample(colorSampler, i.tex.xy);
	return fragcolor + surfaceColor;
}