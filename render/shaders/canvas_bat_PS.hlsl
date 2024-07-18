// shaders\canvas_bat_PS.hlsl

#include "global_header.hlsl"

Texture2D g_OtherTextures[WORLD_MODEL_TEXTURES_PER_DRAW] : register(t5);

SamplerState g_SamplerState : register(s1);

PS_PERFRAME_BUFFER_3D

cbuffer C_PEROBJECT_BUFFER : register(b5)
{
    row_major float4x4 g_mCanvasWorldView : packoffset(c0);
    row_major float4x4 g_mCanvasWorldViewProj : packoffset(c4);
    row_major float4x4 g_mCanvasWorldViewRC : packoffset(c8);
    row_major float4x4 g_mCanvasWorldViewProjRC : packoffset(c12);
    Canvas_VPS g_aCanvas[CANVAS_BATCH_SIZE] : packoffset(c16);
}

float4 PSMain(PS_INPUT_CANVAS_BAT Input) : SV_TARGET
{
    float4 vFinalColor;
    uint dwMode = g_aCanvas[Input.dwDataIndex].m_vModeAndTextureIndex.x;
    
    if (!(dwMode & MODE_NO_TEXTURE))
    {
        vFinalColor = SampleTextureArray12(g_aCanvas[Input.dwDataIndex].m_vModeAndTextureIndex.y, Input.vTexCoords, g_SamplerState, g_OtherTextures);
        
        vFinalColor.a *= Input.vDiffuseColor.a;
        vFinalColor.rgb = ColorOp(dwMode, vFinalColor.rgb, Input.vDiffuseColor.rgb);
    }
    else
    {
        vFinalColor = Input.vDiffuseColor;
    }
    
    if (dwMode & MODE_FLAT_COLOR)
        vFinalColor.rgb = g_aCanvas[Input.dwDataIndex].m_vModeColor;
    else if (dwMode & MODE_FOG_ENABLED)
        ApplyFog(vFinalColor, g_vFogColor, Input.fFogFactor, dwMode);
  
    return vFinalColor;
}