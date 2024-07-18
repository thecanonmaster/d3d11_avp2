// shaders\optimizedsurface_VS.hlsl

cbuffer C_PERFRAME_BUFFER : register(b0)
{
    float4x4    g_mWorld : packoffset(c0);
    float4x4    g_mView : packoffset(c4);
    float4x4    g_mProjection : packoffset(c8);
    float4      g_vBaseColor : packoffset(c12);
};

struct VS_INPUT
{
    float4  vPosition : POSITION;
    float2  vTexCoords : TEXCOORD0;
};

struct VS_OUTPUT
{
    float4  vPosition : SV_POSITION;
    float4  vBaseColor : COLOR0;
    float2  vTexCoords : TEXCOORD0;   
};

VS_OUTPUT VSMain(VS_INPUT Input)
{
    VS_OUTPUT Output;

    Output.vPosition = mul(Input.vPosition, g_mWorld);
    Output.vPosition = mul(Output.vPosition, g_mView);
    Output.vPosition = mul(Output.vPosition, g_mProjection);
    
    Output.vTexCoords = Input.vTexCoords;
    Output.vBaseColor = g_vBaseColor;

    return Output;
}