// shaders\optimizedsurface_PS.hlsl

#include "global_header.hlsl"

Texture2D g_Texture : register(t0);
SamplerState g_SamplerState : register(s0);

cbuffer C_PEROBJECT_BUFFER : register(b1)
{
    float3 g_vTransparentColor : packoffset(c0.x);
    float g_fAlpha : packoffset(c0.w);
    float3 g_vBaseColor : packoffset(c1.x);
};

float4 PSMain(PS_INPUT_OPTIMIZED Input) : SV_TARGET
{
	float4 vFinalColor = g_Texture.Sample(g_SamplerState, Input.vTexCoords);
    
    if (!all(vFinalColor.rgb == g_vTransparentColor))
    {     
        vFinalColor.rgb *= g_vBaseColor.rgb;
        vFinalColor.a = g_fAlpha;
    }
	else
    {
        vFinalColor.a = 0.0f;
    }
	
	return vFinalColor;
}