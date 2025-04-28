cbuffer cbView : register(b1) //Vertex Shader constant buffer slot 1
{
	matrix viewMatrix;
};

struct VSInput
{
    float3 Pos : POSITION0;
    float3 PreviousPos : POSITION1;
    float Age : TEXCOORD0;
};

struct GSInput
{
    float3 Pos : POSITION0;
    float3 PreviousPos : POSITION1;
    float Age : TEXCOORD0;
};

GSInput main(VSInput input)
{
    GSInput output;
    output.Pos = mul(viewMatrix, float4(input.Pos, 1.0)).xyz;
    output.PreviousPos = mul(viewMatrix, float4(input.PreviousPos, 1.0)).xyz;
    output.Age = input.Age;
    return output;
}