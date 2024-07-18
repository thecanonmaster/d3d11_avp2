// shaders\optimizedsurface_VS.hlsl

#include "global_header.hlsl"

VS_PERFRAME_BUFFER_2D

PS_INPUT_OPTIMIZED VSMain(VS_INPUT_OPTIMIZED Input)
{
    PS_INPUT_OPTIMIZED Output;
    
    Output.vPosition.xy = Input.vPosition.xy * g_vScreenDims * 2.0f;
    Output.vPosition.xy -= 1.0f;
    Output.vPosition.y *= -1.0f;
    Output.vPosition.zw = float2(0.0f, 1.0f);
    
    Output.vTexCoords = Input.vTexCoords;
    
    return Output;
}