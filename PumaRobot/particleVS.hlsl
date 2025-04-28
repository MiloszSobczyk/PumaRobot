cbuffer ViewCB : register(b1) // Constant buffer bound to slot b1
{
    matrix gViewMatrix;
};

struct VertexInput
{
    float3 Position : POSITION0;
    float3 PreviousPos : POSITION1;
    float Age : TEXCOORD0;
    float Size : TEXCOORD1;
};

struct VertexOutput
{
    float4 Position : POSITION0;
    float4 PreviousPos : POSITION1;
    float Age : TEXCOORD0;
    float Size : TEXCOORD1;
};

VertexOutput main(VertexInput input)
{
    VertexOutput output;

    // Transform current position
    float4 worldPos = float4(input.Position, 1.0f);
    output.Position = mul(gViewMatrix, worldPos);

    // Transform previous position
    float4 worldPrevPos = float4(input.PreviousPos, 1.0f);
    output.PreviousPos = mul(gViewMatrix, worldPrevPos);

    // Pass through age and size
    output.Age = input.Age;
    output.Size = input.Size;

    return output;
}
