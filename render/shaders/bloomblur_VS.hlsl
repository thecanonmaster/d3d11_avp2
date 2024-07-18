// shaders\bloomblur_VS.hlsl

#include "global_header.hlsl"

PS_INPUT_BLOOM VSMain(uint dwIndex : SV_VertexId)
{
    PS_INPUT_BLOOM Output;
    
    float2 vTexCoords = float2((dwIndex << 1) & 2, dwIndex & 2);
    
    Output.vTexCoords = vTexCoords;
    Output.vPosition = float4(vTexCoords.x * 2.0f - 1.0f, -vTexCoords.y * 2.0f + 1.0f, 0.0f, 1.0f);

    return Output;
}