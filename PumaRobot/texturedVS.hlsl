cbuffer cbWorld : register(b0)
{
    matrix worldMatrix;
};
cbuffer cbView : register(b1)
{
    matrix viewMatrix;
};
cbuffer cbProj : register(b2)
{
    matrix projMatrix;
};
cbuffer cbTexTransform : register(b3)
{
    matrix texMatrix;
};

struct VSInput
{
    float3 pos : POSITION;
};

struct PSInput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

PSInput main(VSInput i)
{
    PSInput o;

    o.pos = float4(i.pos, 1.0f);

    o.pos = mul(worldMatrix, o.pos);
    o.pos = mul(viewMatrix, o.pos);
    o.pos = mul(projMatrix, o.pos);

    float4 uv4 = mul(texMatrix, float4(i.pos, 1.0f));
    o.uv = uv4.xy;

    return o;
}
