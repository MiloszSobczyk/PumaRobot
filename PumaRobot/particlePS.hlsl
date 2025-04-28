Texture2D ParticleTexture : register(t0);
Texture2D OpacityTexture : register(t1);
SamplerState Sampler : register(s0);

struct PSInput
{
    float4 Position : SV_POSITION;
    float2 TexCoord1 : TEXCOORD0;
    float2 TexCoord2 : TEXCOORD1;
};

float4 main(PSInput input) : SV_TARGET
{
    // Sample the cloud texture using the provided texture coordinates
    float4 color = ParticleTexture.Sample(Sampler, input.TexCoord1);
    
    // Adjust color based on alpha
    color.rgb *= color.a;
    
    // Compute the final alpha based on opacity texture and cloud alpha
    float finalAlpha = color.a * (1.0f - input.TexCoord2.x);
    
    // Return the final color with adjusted alpha
    return float4(color.rgb, finalAlpha);
}
