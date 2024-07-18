// shaders\soliddrawing_PS.hlsl

#include "global_header.hlsl"

Texture2D g_Texture : register(t0);
SamplerState g_SamplerState : register(s0);

float4 PSMain(PS_INPUT_SOLIDDRAW Input) : SV_TARGET
{
    float4 vFinalColor = g_Texture.Sample(g_SamplerState, Input.vTexCoords);
	
    vFinalColor.a = all(vFinalColor == 1.0f) ? 0.0f : 1.0f;
	
    return vFinalColor;
}