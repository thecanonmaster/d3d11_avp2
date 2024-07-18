// shaders\fullscreenvideo_PS.hlsl

#include "global_header.hlsl"

Texture2D g_Texture : register(t0);
SamplerState g_SamplerState : register(s0);

float4 PSMain(PS_INPUT_FSVIDEO Input) : SV_TARGET
{
    return g_Texture.Sample(g_SamplerState, Input.vTexCoords);
}