Texture2D Tex1 : register(t0);
SamplerState Sampler1 : register(s0);

struct VS_OUTPUT
{
    float4 Pos: SV_POSITION;
    float2 TexCoord : TEXCOORD;
};

float4 main(VS_OUTPUT Input) : SV_TARGET
{
    return Tex1.Sample(Sampler1, Input.TexCoord);
}