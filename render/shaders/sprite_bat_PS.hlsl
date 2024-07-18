// shaders\sprite_bat_PS.hlsl

#include "global_header.hlsl"

Texture2D g_OtherTextures[WORLD_MODEL_TEXTURES_PER_DRAW] : register(t5);

SamplerState g_SamplerState : register(s1);

PS_PERFRAME_BUFFER_3D

cbuffer C_PEROBJECT_BUFFER : register(b3)
{
    row_major float4x4 g_mSpriteWorldView : packoffset(c0);
    row_major float4x4 g_mSpriteWorldViewProj : packoffset(c4);
    row_major float4x4 g_mSpriteWorldViewRC : packoffset(c8);
    row_major float4x4 g_mSpriteWorldViewProjRC : packoffset(c12);
    Sprite_VPS g_aSprite[SPRITE_BATCH_SIZE] : packoffset(c16);
}

float4 PSMain(PS_INPUT_SPRITE_BAT Input) : SV_TARGET
{ 
    float4 vFinalColor = SampleTextureArray12(g_aSprite[Input.dwDataIndex].m_vModeAndTextureIndex.y, Input.vTexCoords, g_SamplerState, g_OtherTextures);
    
    uint dwMode = g_aSprite[Input.dwDataIndex].m_vModeAndTextureIndex.x;
    vFinalColor.a *= g_aSprite[Input.dwDataIndex].m_vDiffuseColor.a;
    vFinalColor.rgb = ColorOp(dwMode, vFinalColor.rgb, g_aSprite[Input.dwDataIndex].m_vDiffuseColor.rgb);
    
    if (dwMode & MODE_FLAT_COLOR)
        vFinalColor.rgb = g_aSprite[Input.dwDataIndex].m_vModeColor;
    else if (dwMode & MODE_FOG_ENABLED)
        ApplyFog(vFinalColor, g_vFogColor, Input.fFogFactor, dwMode);

     
    return vFinalColor;
}