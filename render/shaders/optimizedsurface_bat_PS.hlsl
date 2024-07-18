// shaders\optimizedsurface_bat_PS.hlsl

#include "global_header.hlsl"

Texture2D g_OtherTextures[WORLD_MODEL_TEXTURES_PER_DRAW] : register(t5);

SamplerState g_SamplerState : register(s0);

cbuffer C_PEROBJECT_BUFFER : register(b1)
{
    OptimizedSurface_PS g_aOptimizedSurface[OPTIMIZED_SURFACE_BATCH_SIZE] : packoffset(c0);
};

float4 PSMain(PS_INPUT_OPTIMIZED_BAT Input) : SV_TARGET
{
    OptimizedSurface_PS surface = g_aOptimizedSurface[Input.dwDataIndex];
    
    float4 vFinalColor = SampleTextureArray12(asuint(surface.m_vBaseColorAndTextureIndex.a), Input.vTexCoords, g_SamplerState, g_OtherTextures);
    
    if (!all(vFinalColor.rgb == surface.m_vTransparentColorAndAlpha.rgb))
    {     
        vFinalColor.rgb *= surface.m_vBaseColorAndTextureIndex.rgb;
        vFinalColor.a = surface.m_vTransparentColorAndAlpha.a;
    }
	else
    {
        vFinalColor.a = 0.0f;
    }
	
	return vFinalColor;
}