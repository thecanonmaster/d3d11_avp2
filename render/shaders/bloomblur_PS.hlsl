// shaders\bloomblur_PS.hlsl

#include "global_header.hlsl"

Texture2D g_Texture : register(t0);
SamplerState g_SamplerState : register(s1);

cbuffer C_PEROBJECT_BUFFER : register(b11)
{
    float4 g_avSampleWeight[MAX_BLOOM_SAMPLES] : packoffset(c0);
    float4 g_avSampleOffset[MAX_BLOOM_SAMPLES] : packoffset(c15);
}

float4 PSMain(PS_INPUT_BLOOM Input) : SV_TARGET
{
    float4 vFinalColor = 0.0f;

    for (uint i = 0; i < MAX_BLOOM_SAMPLES; i++)
        vFinalColor += g_avSampleWeight[i] * g_Texture.Sample(g_SamplerState, Input.vTexCoords + g_avSampleOffset[i].xy);

    return vFinalColor;
}