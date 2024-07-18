// shaders\particlesystem_PS.hlsl

#include "global_header.hlsl"

Texture2D g_Texture : register(t0);
SamplerState g_SamplerState : register(s1);

PS_PERFRAME_BUFFER_3D

cbuffer C_PEROBJECT_BUFFER : register(b4)
{
    float4x4 g_mSystemWorld : packoffset(c0);
    row_major float4x4 g_mSystemWorldView : packoffset(c4);
    row_major float4x4 g_mSystemWorldViewProj : packoffset(c8);
    float4 g_vColorScale : packoffset(c12);
    float4 g_vParticleUp : packoffset(c13);
    float4 g_vParticleRight : packoffset(c14);
    float3 g_vModeColor : packoffset(c15.x);
    uint g_dwMode : packoffset(c15.w);
}

float4 PSMain(PS_INPUT_PARTICLESYSTEM Input) : SV_TARGET
{
    float4 vFinalColor = g_Texture.Sample(g_SamplerState, Input.vTexCoords);
    
    vFinalColor.a *= Input.vDiffuseColor.a;
    vFinalColor.rgb = ColorOp(g_dwMode, vFinalColor.rgb, Input.vDiffuseColor.rgb);
    
    if (g_dwMode & MODE_FLAT_COLOR)
        vFinalColor.rgb = g_vModeColor;
    else if (g_dwMode & MODE_FOG_ENABLED)
        ApplyFog(vFinalColor, g_vFogColor, Input.fFogFactor, g_dwMode);
    
    return vFinalColor;
}