struct VS_INPUT
{
    float3 Pos: POSITION;
    float4 Color: COLOR;
};

struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float4 Color : COLOR;
};

VS_OUTPUT main(VS_INPUT Input)
{
    VS_OUTPUT Output;
    
    Output.Color = Input.Color;
    Output.Pos = float4(Input.Pos, 1.0f);
    
    return Output;
}