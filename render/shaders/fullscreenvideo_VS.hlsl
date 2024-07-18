// shaders\fullscreenvideo_VS.hlsl

#include "global_header.hlsl"

VS_PERFRAME_BUFFER_2D

PS_INPUT_FSVIDEO VSMain(VS_INPUT_FSVIDEO Input)
{
    PS_INPUT_FSVIDEO Output;
    
    Output.vPosition.xy = Input.vPosition.xy * g_vScreenDims * 2.0f;
    Output.vPosition.xy -= 1.0f;
    Output.vPosition.y *= -1.0f;
    Output.vPosition.zw = float2(0.0f, 1.0f);
    
    Output.vTexCoords = Input.vTexCoords;

    return Output;
}