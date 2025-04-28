Texture2D cloudMap : register(t0);
SamplerState colorSampler : register(s0);

struct PSInput
{
	float4 pos : SV_POSITION;
	float2 tex: TEXCOORD0;
};

float4 main(PSInput i) : SV_TARGET
{
    return cloudMap.Sample(colorSampler, i.tex);
};