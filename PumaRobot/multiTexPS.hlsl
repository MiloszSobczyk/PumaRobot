Texture2D colorMap1 : register(t0);
Texture2D colorMap2 : register(t1);
SamplerState colorSampler : register(s0);

struct PSInput
{
	float4 pos : SV_POSITION;
	float2 tex1: TEXCOORD0;
	float2 tex2: TEXCOORD1;
};

float4 main(PSInput i) : SV_TARGET
{
	// DONE : 1.02 Sample both textures using their respective texture coordinates
	//float4 color1 = colorMap1.Sample(colorSampler, i.tex1);
	//float4 color2 = colorMap2.Sample(colorSampler, i.tex2);

	// DONE : 1.03 For now, return only the second color
	//return color2;

	// DONE : 1.09 Change the shader to so that the two color are alpha-blended based on alpha channel of the second one
	float4 fragcolor1 = colorMap1.Sample(colorSampler, i.tex1.xy);
	float4 fragcolor2 = colorMap2.Sample(colorSampler, i.tex2.xy);

	return float4(fragcolor1.xyz * (1.0 - fragcolor2.w) + fragcolor2.xyz * fragcolor2.w, 1.0);
}