// shaders\sprite_PS.hlsl

#include "global_header.hlsl"

Texture2D g_Texture : register(t0);
SamplerState g_SamplerState : register(s1);

PS_PERFRAME_BUFFER_3D

cbuffer C_PEROBJECT_BUFFER : register(b3)
{
    float4x4 g_mSpriteWorld : packoffset(c0);
    row_major float4x4 g_mSpriteWorldView : packoffset(c4);
    row_major float4x4 g_mSpriteWorldViewProj : packoffset(c8);
    float3 g_vModeColor : packoffset(c12.x);
    uint g_dwMode : packoffset(c12.w);
    float4 g_vDiffuseColor : packoffset(c13);
}

float4 PSMain(PS_INPUT_SPRITE Input) : SV_TARGET
{ 
    float4 vFinalColor = g_Texture.Sample(g_SamplerState, Input.vTexCoords);
    
    vFinalColor.a *= g_vDiffuseColor.a;
    vFinalColor.rgb = ColorOp(g_dwMode, vFinalColor.rgb, g_vDiffuseColor.rgb);
    
    // TODO - test
    if (!(g_dwMode & MODE_NO_LIGHT))
        vFinalColor.rgb = ColorModulate(vFinalColor.rgb, g_vGlobalVertexTint);
    
    if (g_dwMode & MODE_FLAT_COLOR)
        vFinalColor.rgb = g_vModeColor;
    else if (g_dwMode & MODE_FOG_ENABLED)
        ApplyFog(vFinalColor, g_vFogColor, Input.fFogFactor, g_dwMode);

    return vFinalColor;
}