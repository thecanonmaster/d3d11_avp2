// shaders\particlesystem_VS.hlsl

#include "global_header.hlsl"

VS_PERFRAME_BUFFER_3D

cbuffer C_PEROBJECT_BUFFER : register(b4)
{
    float4x4 g_mSystemWorld : packoffset(c0);
    row_major float4x4 g_mSystemWorldView : packoffset(c4);
    row_major float4x4 g_mSystemWorldViewProj : packoffset(c8);
    float4 g_vColorScale : packoffset(c12);
    float4 g_vParticleUp : packoffset(c13);
    float4 g_vParticleRight : packoffset(c14);
    float3 g_vModeColor : packoffset(c15.x);
    uint g_dwMode : packoffset(c15.w);
}

float3 GetScaledVertexOffset(uint dwIndex, float fScale)
{
    float4 fResult;
    switch (dwIndex)
    {
        case 1:
            fResult = (g_vParticleUp + g_vParticleRight) * fScale;
            break;
        case 2:
            fResult = (-g_vParticleUp + g_vParticleRight) * fScale;
            break;
        case 3:
            fResult = (-g_vParticleUp - g_vParticleRight) * fScale;
            break;
        case 0:
        default:
            fResult = (g_vParticleUp - g_vParticleRight) * fScale;
            break;
    }
    
    return fResult.xyz;
}

PS_INPUT_CANVAS VSMain(VS_INPUT_PARTICLESYSTEM Input, uint dwIndex : SV_VertexId)
{
    PS_INPUT_CANVAS Output;

    float4 vPosition = { Input.vPosition.xyz + GetScaledVertexOffset(dwIndex, Input.fScale), 1.0f };
    
    Output.vPosition = mul(vPosition, g_mSystemWorldViewProj);
   
    Output.vTexCoords = Input.vTexCoords;
    Output.vDiffuseColor = Input.vDiffuseColor * g_vColorScale;
    Output.fFogFactor = 0.0f;
    
    if (g_dwMode & MODE_FOG_ENABLED)
    {
        if (g_fVFogEnabled == 0.0f)
        {
            float4 vPosRelCamera = mul(vPosition, g_mSystemWorldView);
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