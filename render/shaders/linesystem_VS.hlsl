// shaders\linesystem_VS.hlsl

#include "global_header.hlsl"

VS_PERFRAME_BUFFER_3D

cbuffer C_PEROBJECT_BUFFER : register(b7)
{
    float4x4 g_mSystemWorld : packoffset(c0);
    row_major float4x4 g_mSystemWorldView : packoffset(c4);
    row_major float4x4 g_mSystemWorldViewProj : packoffset(c8);
    float g_fAlphaScale : packoffset(c12.x);
    uint g_dwMode : packoffset(c12.y);
}

PS_INPUT_LINESYSTEM VSMain(VS_INPUT_LINESYSTEM Input)
{
    PS_INPUT_LINESYSTEM Output;

    Output.vPosition = mul(Input.vPosition, g_mSystemWorldViewProj);
    
    Output.vDiffuseColor = Input.vDiffuseColor;
    Output.vDiffuseColor.a *= g_fAlphaScale;
    Output.fFogFactor = 0.0f;
    
    if (g_dwMode & MODE_FOG_ENABLED)
    {
        if (g_fVFogEnabled == 0.0f)
        {
            float4 vPosRelCamera = mul(Input.vPosition, g_mSystemWorldView);
            Output.fFogFactor = ComputeFogFactor(vPosRelCamera.z, g_vFogStartEnd.x, g_vFogStartEnd.y);
        }
        else
        {
            float4 vPosRelCamera = mul(Input.vPosition, g_mSystemWorldView);
            float4 vPosRelWorld = mul(Input.vPosition, g_mSystemWorld);
            
            Output.fFogFactor = ComputeVFogFactor(g_vCameraPos.y, vPosRelCamera.z, vPosRelWorld.y, g_vFogMinMaxY.x, g_vFogMinMaxY.y,
                g_vFogMinMaxYVal.x, g_vFogMinMaxYVal.y, g_fVFogDensity, g_fVFogMax);
        }
    }

    return Output;
}