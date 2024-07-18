// shaders\bllomcombine_PS.hlsl

#include "global_header.hlsl"

Texture2D g_BaseTexture : register(t0);
Texture2D g_BloomTexture : register(t5);

SamplerState g_SamplerState : register(s0);

cbuffer C_PEROBJECT_BUFFER : register(b11)
{
    float4 g_vBloomThreshold : packoffset(c0);
    float g_fBaseSaturation : packoffset(c1.x);
    float g_fBloomSaturation : packoffset(c1.y);
    float4 g_vBaseIntensity : packoffset(c2);
    float4 g_vBloomIntensity : packoffset(c3);
}

float4 AdjustSaturation(float4 vColor, float fSaturation)
{
    float3 vGrayscale = float3(0.2125f, 0.7154f, 0.0721f);
    float vGray = dot(vColor.rgb, vGrayscale);
    return lerp(vGray, vColor, fSaturation);
}

float4 PSMain(PS_INPUT_TONEMAP Input) : SV_TARGET
{
    float4 vBaseColor = g_BaseTexture.Sample(g_SamplerState, Input.vTexCoords);
    float4 vBloomColor = g_BloomTexture.Sample(g_SamplerState, Input.vTexCoords);

    vBaseColor = AdjustSaturation(vBaseColor, g_fBaseSaturation) * g_vBaseIntensity;
    vBloomColor = AdjustSaturation(vBloomColor, g_fBloomSaturation) * g_vBloomIntensity;

    vBaseColor *= (1.0f - saturate(vBloomColor));

    return vBaseColor + vBloomColor;
}