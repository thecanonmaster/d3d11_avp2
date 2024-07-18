// shaders\polygrid_PS.hlsl

#include "global_header.hlsl"

Texture2D g_MainTexture : register(t0);
Texture2D g_EnvTexture : register(t5);

SamplerState g_SamplerState : register(s0);

PS_PERFRAME_BUFFER_3D

cbuffer C_PEROBJECT_BUFFER : register(b6)
{
    float4x4 g_mGridWorld : packoffset(c0);
    row_major float4x4 g_mGridWorldView : packoffset(c4);
    row_major float4x4 g_mGridWorldViewProj : packoffset(c8);
    float4 g_vColorScale : packoffset(c12);
    float3 g_vModeColor : packoffset(c13.x);
    uint g_dwMode : packoffset(c13.w);
}

float4 PSMain(PS_INPUT_POLYGRID Input) : SV_TARGET
{
    float4 vFinalColor;
    
    if (!(g_dwMode & MODE_ENV_MAP_ONLY))
    {
        vFinalColor = g_MainTexture.Sample(g_SamplerState, Input.vTexCoords);
        
        if (g_dwMode & MODE_ENV_MAP)
        {
            float4 vEnvColor = g_EnvTexture.Sample(g_SamplerState, Input.vEnvCoords);
            vFinalColor.rgb = ColorModulate(vFinalColor.rgb, vEnvColor.rgb);
        }
    }
    else
    {
        vFinalColor = g_EnvTexture.Sample(g_SamplerState, Input.vEnvCoords);
    }
  
    vFinalColor.a *= Input.vDiffuseColor.a;
    vFinalColor.rgb = ColorOp(g_dwMode, vFinalColor.rgb, Input.vDiffuseColor.rgb);
    
    if (g_dwMode & MODE_FLAT_COLOR)
        vFinalColor.rgb = g_vModeColor;
    else if (g_dwMode & MODE_FOG_ENABLED)
        ApplyFog(vFinalColor, g_vFogColor, Input.fFogFactor, g_dwMode);
    
    return vFinalColor;
}