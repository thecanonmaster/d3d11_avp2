// shaders\passthrough_PS.hlsl

#include "global_header.hlsl"

Texture2D g_Texture : register(t0);
SamplerState g_SamplerState : register(s0);

float4 PSMain(PS_INPUT_PASSTHROUGH Input) : SV_TARGET
{
    float4 vFinalColor = g_Texture.Sample(g_SamplerState, Input.vTexCoords);
    
    return vFinalColor;
}