struct VS_INPUT
{
    float3 Pos: POSITION;
    float2 TexCoord: TEXCOORD;
};

struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float2 TexCoord : TEXCOORD;
};

cbuffer ConstantBuffer : register(b0)
{
    float4x4 WorldViewProjMatrix;
}

VS_OUTPUT main(VS_INPUT Input)
{
    VS_OUTPUT Output;
    
    Output.TexCoord = Input.TexCoord; //Input.Color;
    Output.Pos = mul(float4(Input.Pos, 1), WorldViewProjMatrix);
    
    return Output;
}