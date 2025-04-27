Texture2D    mirrorTex : register(t0);
SamplerState mirrorSampler : register(s0);

struct PSInput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

float4 main(PSInput i) : SV_TARGET
{
    return mirrorTex.Sample(mirrorSampler, i.uv);
}
