// shaders\canvas_VS.hlsl

#include "global_header.hlsl"

VS_PERFRAME_BUFFER_3D

cbuffer C_PEROBJECT_BUFFER : register(b5)
{
    float4x4 g_mCanvasWorld : packoffset(c0);
    row_major float4x4 g_mCanvasWorldView : packoffset(c4);
    row_major float4x4 g_mCanvasWorldViewProj : packoffset(c8);
    float3 g_vModeColor : packoffset(c12.x);
    uint g_dwMode : packoffset(c12.w);
}

PS_INPUT_CANVAS VSMain(VS_INPUT_CANVAS Input)
{
    PS_INPUT_CANVAS Output;

    Output.vPosition = mul(Input.vPosition, g_mCanvasWorldViewProj);
    
    Output.vTexCoords = Input.vTexCoords;
    Output.vDiffuseColor = Input.vDiffuseColor;
    Output.fFogFactor = 0.0f;
    
    if (g_dwMode & MODE_FOG_ENABLED)
    {
        if (g_fVFogEnabled == 0.0f)
        {
            float4 vPosRelCamera = mul(Input.vPosition, g_mCanvasWorldView);
            Output.fFogFactor = ComputeFogFactor(vPosRelCamera.z, g_vFogStartEnd.x, g_vFogStartEnd.y);
        }
        else
        {
            float4 vPosRelCamera = mul(Input.vPosition, g_mCanvasWorldView);
            
            Output.fFogFactor = ComputeVFogFactor(g_vCameraPos.y, vPosRelCamera.z, Input.vPosition.y, g_vFogMinMaxY.x, g_vFogMinMaxY.y,
                g_vFogMinMaxYVal.x, g_vFogMinMaxYVal.y, g_fVFogDensity, g_fVFogMax);
        }
    }

    return Output;
}