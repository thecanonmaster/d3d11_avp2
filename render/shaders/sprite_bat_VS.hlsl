// shaders\sprite_bat_VS.hlsl

#include "global_header.hlsl"

VS_PERFRAME_BUFFER_3D

cbuffer C_PEROBJECT_BUFFER : register(b3)
{
    row_major float4x4 g_mSpriteWorldView : packoffset(c0);
    row_major float4x4 g_mSpriteWorldViewProj : packoffset(c4);
    row_major float4x4 g_mSpriteWorldViewRC : packoffset(c8);
    row_major float4x4 g_mSpriteWorldViewProjRC : packoffset(c12);
    Sprite_VPS g_aSprite[SPRITE_BATCH_SIZE] : packoffset(c16);
}

PS_INPUT_SPRITE_BAT VSMain(VS_INPUT_SPRITE_BAT Input)
{
    PS_INPUT_SPRITE_BAT Output;
    uint dwMode = g_aSprite[Input.dwDataIndex].m_vModeAndTextureIndex.x;
    
    row_major float4x4 mWorldView;
    row_major float4x4 mWorldViewProj;
    if (dwMode & MODE_REALLY_CLOSE)
    {
        mWorldView = g_mSpriteWorldViewRC;
        mWorldViewProj = g_mSpriteWorldViewProjRC;
    }
    else
    {
        mWorldView = g_mSpriteWorldView;
        mWorldViewProj = g_mSpriteWorldViewProj;
    }
    
    Output.vPosition = mul(Input.vPosition, mWorldViewProj);
    Output.vTexCoords = Input.vTexCoords;
    Output.fFogFactor = 0.0f;
    Output.dwDataIndex = Input.dwDataIndex;
    
    if (dwMode & MODE_FOG_ENABLED)
    {
        if (g_fVFogEnabled == 0.0f)
        {
            float4 vPosRelCamera = mul(Input.vPosition, mWorldView);
            Output.fFogFactor = ComputeFogFactor(vPosRelCamera.z, g_vFogStartEnd.x, g_vFogStartEnd.y);
        }
        else
        {
            float4 vPosRelCamera = mul(Input.vPosition, mWorldView);
      
            Output.fFogFactor = ComputeVFogFactor(g_vCameraPos.y, vPosRelCamera.z, Input.vPosition.y, g_vFogMinMaxY.x, g_vFogMinMaxY.y,
                g_vFogMinMaxYVal.x, g_vFogMinMaxYVal.y, g_fVFogDensity, g_fVFogMax);
        }
    }
    
    return Output;
}