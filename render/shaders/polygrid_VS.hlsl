// shaders\polygrid_VS.hlsl

#include "global_header.hlsl"

VS_PERFRAME_BUFFER_3D

cbuffer C_PEROBJECT_BUFFER : register(b6)
{
    float4x4 g_mGridWorld : packoffset(c0);
    row_major float4x4 g_mGridWorldView : packoffset(c4);
    row_major float4x4 g_mGridWorldViewProj : packoffset(c8);
    float4 g_vColorScale : packoffset(c12);
    float3 g_vModeColor : packoffset(c13.x);
    uint g_dwMode : packoffset(c13.w);
}

PS_INPUT_POLYGRID VSMain(VS_INPUT_POLYGRID Input)
{
    PS_INPUT_POLYGRID Output;

    Output.vPosition = mul(Input.vPosition, g_mGridWorldViewProj);
    
    Output.vTexCoords = Input.vTexCoords;
    Output.vDiffuseColor = Input.vDiffuseColor * g_vColorScale;
    
    if (g_dwMode & MODE_ENV_MAP)
    {
        float3 vCameraVector = normalize(g_vCameraPos.xyz - Input.vPosition.xyz);
        Output.vEnvCoords = reflect(-vCameraVector, Input.vNormal).xy * g_fEnvScale + g_vCameraPos.xz * g_fEnvPanSpeed;
    }
    else
    {
        Output.vEnvCoords = Input.vTexCoords;
    }
    
    Output.fFogFactor = 0.0f;
    
    if (g_dwMode & MODE_FOG_ENABLED)
    {
        if (g_fVFogEnabled == 0.0f)
        {
            float4 vPosRelCamera = mul(Input.vPosition, g_mGridWorldView);
            Output.fFogFactor = ComputeFogFactor(vPosRelCamera.z, g_vFogStartEnd.x, g_vFogStartEnd.y);
        }
        else
        {
            float4 vPosRelCamera = mul(Input.vPosition, g_mGridWorldView);
            float4 vPosRelWorld = mul(Input.vPosition, g_mGridWorld);
            
            Output.fFogFactor = ComputeVFogFactor(g_vCameraPos.y, vPosRelCamera.z, vPosRelWorld.y, g_vFogMinMaxY.x, g_vFogMinMaxY.y,
                g_vFogMinMaxYVal.x, g_vFogMinMaxYVal.y, g_fVFogDensity, g_fVFogMax);
        }
    }
       
    return Output;
}