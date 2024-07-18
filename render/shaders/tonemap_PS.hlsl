// shaders\tonemap_PS.hlsl

#include "global_header.hlsl"

Texture2D g_Texture : register(t0);
SamplerState g_SamplerState : register(s0);

PS_PERFRAME_BUFFER_2D

float4 PSMain(PS_INPUT_TONEMAP Input) : SV_TARGET
{
    float4 vFinalColor = g_Texture.Sample(g_SamplerState, Input.vTexCoords);
	
    vFinalColor.rgb = saturate(vFinalColor.rgb * g_fLinearExposure);
  
    return vFinalColor;
}