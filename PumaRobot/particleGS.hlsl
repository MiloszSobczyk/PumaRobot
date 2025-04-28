cbuffer cbProj : register(b0) //Geometry Shader constant buffer slot 0
{
    matrix projMatrix;
};

struct GSInput
{
    float3 Pos : POSITION0;
    float3 PreviousPos : POSITION1;
    float Age : TEXCOORD0;
};

struct PSInput
{
    float4 pos : SV_POSITION;
    float2 tex : TEXCOORD0;
};

static const float TimeToLive = 4.0f;

[maxvertexcount(4)]
void main(point GSInput inArray[1], inout TriangleStream<PSInput> ostream)
{
    GSInput i = inArray[0];
    PSInput o = (PSInput) 0;
    
    float4 particlePos = float4(inArray[0].Pos, 1.0);
    float4 particlePrevPos = float4(inArray[0].PreviousPos, 1.0);
	
    float dx = 0.1;
    float dy = 0.1;
    
	// TODO : 1.30 Initialize 4 vertices to make a bilboard and append them to the ostream
    o.pos = mul(projMatrix, particlePrevPos + float4(0, dy, 0.f, 0.f));
    o.tex = float2(0.f, 1.f);
    ostream.Append(o);
	
    o.pos = mul(projMatrix, particlePrevPos + float4(0, 0.f, 0.f, 0.f));
    o.tex = float2(0.f, 0.f);
    ostream.Append(o);
	
    o.pos = mul(projMatrix, particlePos + float4(0, 0.f, 0.f, 0.f));
    o.tex = float2(1.f, 1.f);
    ostream.Append(o);
	
    o.pos = mul(projMatrix, particlePos + float4(0, -dy, 0.f, 0.f));
    o.tex = float2(1.f, 0.f);
    ostream.Append(o);
    ostream.RestartStrip();
}