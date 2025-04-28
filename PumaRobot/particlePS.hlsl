Texture2D cloudMap : register(t0);
Texture2D opacityMap : register(t1);
SamplerState colorSampler : register(s0);

struct PSInput
{
    float4 pos : SV_POSITION;
    float2 tex1 : TEXCOORD0;
    float2 tex2 : TEXCOORD1;
};

float4 main(PSInput i) : SV_TARGET
{
    float4 color = cloudMap.Sample(colorSampler, i.tex1);
    color.rgb *= color.a;
    float alpha = color.a * (1 - i.tex2.x);
    return float4(color.rgb, alpha);
};