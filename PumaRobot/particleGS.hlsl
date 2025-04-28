cbuffer ProjCB : register(b0) // Projection matrix constant buffer
{
    matrix gProjMatrix;
};

struct GSInput
{
    float4 Position : POSITION0;
    float4 PreviousPos : POSITION1;
    float Age : TEXCOORD0;
    float Size : TEXCOORD1;
};

struct PSInput
{
    float4 Position : SV_POSITION;
    float2 TexCoord1 : TEXCOORD0;
    float2 TexCoord2 : TEXCOORD1;
};

static const float MIN_SIZE = 0.05f;
static const float LIFETIME = 1.0f;

[maxvertexcount(4)]
void main(point GSInput inVertices[1], inout TriangleStream<PSInput> TriStream)
{
    GSInput input = inVertices[0];
    PSInput output = (PSInput) 0;

    // Extract positions
    float3 currentPos = input.Position.xyz;
    float3 prevPos = input.PreviousPos.xyz;

    // Calculate direction and adjust previous position
    float3 direction = normalize(currentPos - prevPos);
    prevPos = currentPos - direction * MIN_SIZE;

    // Camera-to-trail vector
    float3 camToTrail = normalize(-currentPos);

    // Calculate side vector
    float3 sideVector = normalize(cross(direction, camToTrail));

    // Offset to create width
    float halfWidth = 0.005f;
    float3 offsets[2] = { -sideVector * halfWidth, sideVector * halfWidth };

    // Base positions and texture coordinates
    float3 basePositions[2] = { prevPos, currentPos };
    float2 texCoords[2] = { float2(0.0f, 0.0f), float2(1.0f, 0.0f) };

    // Loop through the base positions (prev and current)
    for (int posIndex = 0; posIndex < 2; ++posIndex)
    {
        // Loop for left and right offsets
        for (int offsetIndex = 0; offsetIndex < 2; ++offsetIndex)
        {
            // Calculate world position based on offset
            float3 worldPosition = basePositions[posIndex] + offsets[offsetIndex];

            // Transform position with projection matrix
            output.Position = mul(gProjMatrix, float4(worldPosition, 1.0f));

            // Set texture coordinates
            output.TexCoord1 = float2(texCoords[posIndex].x, offsetIndex);
            output.TexCoord2 = float2(input.Age / LIFETIME, input.Age / LIFETIME);

            // Append to the triangle stream
            TriStream.Append(output);
        }
    }

    TriStream.RestartStrip();
}
