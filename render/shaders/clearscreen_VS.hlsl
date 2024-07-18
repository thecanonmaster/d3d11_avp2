// shaders\clearscreen_VS.hlsl

#include "global_header.hlsl"

PS_INPUT_CLEARSCREEN VSMain(uint dwIndex : SV_VertexId)
{
    PS_INPUT_CLEARSCREEN Output;
    
    float2 vCoords = float2((dwIndex << 1) & 2, dwIndex & 2);
    Output.vPosition = float4(vCoords.x * 2.0f - 1.0f, -vCoords.y * 2.0f + 1.0f, 0.0f, 1.0f);

    return Output;
}