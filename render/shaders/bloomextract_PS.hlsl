// shaders\bllomextract_PS.hlsl

#include "global_header.hlsl"

Texture2D g_Texture : register(t0);
SamplerState g_SamplerState : register(s0);

cbuffer C_PEROBJECT_BUFFER : register(b11)
{
    float4 g_vBloomThreshold : packoffset(c0);
    float g_fBaseSaturation : packoffset(c1.x);
    float g_fBloomSaturation : packoffset(c1.y);
    float4 g_vBaseIntensity : packoffset(c2);
    float4 g_vBloomIntensity : packoffset(c3);
}

float4 PSMain(PS_INPUT_BLOOM Input) : SV_TARGET
{
    float4 vFinalColor = g_Texture.Sample(g_SamplerState, Input.vTexCoords);
	
    vFinalColor = saturate((vFinalColor - g_vBloomThreshold) / (1.0f - g_vBloomThreshold));
  
    return vFinalColor;
}