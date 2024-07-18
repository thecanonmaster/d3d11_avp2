// shaders\clearscreen_PS.hlsl

#include "global_header.hlsl"

static const float4 g_vZero = { 0.0f, 0.0f, 0.0f, 0.0f };

cbuffer C_PEROBJECT_BUFFER : register(b3)
{
    float4 g_vFillArea : packoffset(c0);
    float4 g_vFillColor : packoffset(c1);
};

float4 PSMain(PS_INPUT_CLEARSCREEN Input) : SV_TARGET
{
    if (Input.vPosition.x > g_vFillArea.x && Input.vPosition.x < g_vFillArea.z  &&
        Input.vPosition.y > g_vFillArea.y && Input.vPosition.y < g_vFillArea.w)
        return g_vFillColor;

    return g_vZero;
}