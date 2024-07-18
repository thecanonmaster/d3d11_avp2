// shaders\screenfx_PS.hlsl

#include "global_header.hlsl"

Texture2D g_Texture : register(t0);
SamplerState g_SamplerState : register(s0);

PS_PERFRAME_BUFFER_3D

float4 PSMain(PS_INPUT_SCREENFX Input) : SV_TARGET
{
    float4 vFinalColor = g_Texture.Sample(g_SamplerState, Input.vTexCoords);
    
    if (g_fInvertHack)
        vFinalColor.rgb = 1.0f - vFinalColor.rgb;
    
    if (!all(g_vLightScale == float3(1.0f, 1.0f, 1.0f)))
        vFinalColor.rgb *= g_vLightScale.rgb;

    if (any(g_vLightAdd.rgb))
        vFinalColor.rgb = saturate(vFinalColor.rgb + g_vLightAdd.rgb);
    
    return vFinalColor;
}