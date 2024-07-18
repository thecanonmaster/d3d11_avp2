// shaders\skyportal_VS.hlsl

#include "global_header.hlsl"

VS_PERFRAME_BUFFER_3D

cbuffer C_PEROBJECT_BUFFER : register(b8)
{
    row_major float4x4 g_mModelWorldViewProj : packoffset(c0);
    float2 g_fScreenDims : packoffset(c4);
}

PS_INPUT_SKY_PORTAL VSMain(VS_INPUT_SKY_PORTAL Input)
{
    PS_INPUT_SKY_PORTAL Output;

    Output.vPosition = mul(Input.vPosition, g_mModelWorldViewProj);
             
    return Output;
}