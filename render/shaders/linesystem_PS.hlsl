// shaders\linesystem_PS.hlsl

#include "global_header.hlsl"

PS_PERFRAME_BUFFER_3D

cbuffer C_PEROBJECT_BUFFER : register(b7)
{
    float4x4 g_mSystemWorld : packoffset(c0);
    row_major float4x4 g_mSystemWorldView : packoffset(c4);
    row_major float4x4 g_mSystemWorldViewProj : packoffset(c8);
    float g_fAlphaScale : packoffset(c12.x);
    uint g_dwMode : packoffset(c12.y);
}

float4 PSMain(PS_INPUT_LINESYSTEM Input) : SV_TARGET
{
    float4 vFinalColor = Input.vDiffuseColor;
    
    if (g_dwMode & MODE_FOG_ENABLED)
        ApplyFog(vFinalColor, g_vFogColor, Input.fFogFactor, g_dwMode);
    
    return vFinalColor;
}