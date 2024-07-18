// shaders\skyportal_PS.hlsl

#include "global_header.hlsl"

Texture2D g_Texture : register(t5);
SamplerState g_SamplerState : register(s0);

PS_PERFRAME_BUFFER_3D

cbuffer C_PEROBJECT_BUFFER : register(b8)
{
    row_major float4x4 g_mModelWorldViewProj : packoffset(c0);
    float2 g_fScreenDims : packoffset(c4);
}

float4 PSMain(PS_INPUT_SKY_PORTAL Input) : SV_TARGET
{
    float4 vFinalColor;
    
    vFinalColor = g_Texture.Sample(g_SamplerState, float2(Input.vPosition.x / g_fScreenDims.x, Input.vPosition.y / g_fScreenDims.y));
          
    return vFinalColor;
}