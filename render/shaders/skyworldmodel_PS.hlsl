// shaders\skyworldmodel_PS.hlsl

#include "global_header.hlsl"

Texture2D g_OtherTextures[WORLD_MODEL_TEXTURES_PER_DRAW] : register(t5);

SamplerState g_SamplerState : register(s0);

PS_PERFRAME_BUFFER_3D

struct ModeColorAndMode
{
    float3 m_vModeColor;
    uint m_dwMode;
};

cbuffer C_PEROBJECT_SUB_BUFFER : register(b9)
{
    ModeColorAndMode g_aModeData[WORLD_MODEL_TEXTURES_PER_DRAW] : packoffset(c0);
    float g_afAlphaRef[WORLD_MODEL_TEXTURES_PER_DRAW] : packoffset(c12);
}

float4 PSMain(PS_INPUT_SKY_WORLD_MODEL Input) : SV_TARGET
{
    float4 vFinalColor;
    
    uint dwMode = g_aModeData[Input.dwMainTextureIndex].m_dwMode;
    
    if (!(dwMode & MODE_NO_TEXTURE))
        vFinalColor = SampleTextureArray12(Input.dwMainTextureIndex, Input.vTexCoords, g_SamplerState, g_OtherTextures);
    else
        vFinalColor = float4(1.0f, 1.0f, 1.0f, 1.0f);
    
    vFinalColor = ColorOp(dwMode, vFinalColor, Input.vDiffuseColor);
    
    // TODO - test
    if (!(dwMode & MODE_NO_LIGHT))
        vFinalColor.rgb = ColorModulate(vFinalColor.rgb, g_vGlobalVertexTint);
    
    if (dwMode & MODE_FLAT_COLOR)
        vFinalColor.rgb = g_aModeData[Input.dwMainTextureIndex].m_vModeColor;
    else if (dwMode & MODE_FOG_ENABLED)
        ApplyFog(vFinalColor, g_vFogColor, Input.fFogFactor, dwMode);
        
    return vFinalColor;
}