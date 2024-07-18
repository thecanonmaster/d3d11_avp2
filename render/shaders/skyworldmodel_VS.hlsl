// shaders\skyworldmodel_VS.hlsl

#include "global_header.hlsl"

StructuredBuffer<VertexFXData> g_aVertexFXData : register(t0);

VS_PERFRAME_BUFFER_3D

cbuffer C_PEROBJECT_BUFFER : register(b8)
{
    float4x4 g_mModelWorld : packoffset(c0);
    row_major float4x4 g_mModelWorldView : packoffset(c4);
    row_major float4x4 g_mModelWorldViewProj : packoffset(c8);
    float4 g_vDiffuseColor : packoffset(c12);
}

cbuffer C_PEROBJECT_SUB_BUFFER : register(b9)
{
    uint g_adwMode[WORLD_MODEL_TEXTURES_PER_DRAW] : packoffset(c0);
}

PS_INPUT_SKY_WORLD_MODEL VSMain(VS_INPUT_WORLD_MODEL Input)
{
    PS_INPUT_SKY_WORLD_MODEL Output;
 
    Output.vPosition = mul(Input.vPosition, g_mModelWorldViewProj);
    
    uint dwMode = g_adwMode[Input.adwTextureIndices[0]];
    
    if (dwMode & MODE_SURFACE_FX)
        Output.vTexCoords = g_aVertexFXData[Input.adwTextureIndices[3]].m_vTexCoords;
    else
        Output.vTexCoords = Input.vTexCoords;
    
    Output.vDiffuseColor = Input.vDiffuseColor * g_vDiffuseColor;
    Output.dwMainTextureIndex = Input.adwTextureIndices[0];
    Output.fFogFactor = 0.0f;
    
    if (dwMode & MODE_FOG_ENABLED)
    {
        float4 vPosRelCamera = mul(Input.vPosition, g_mModelWorldView);
        Output.fFogFactor = ComputeFogFactor(vPosRelCamera.z, g_fSkyFogNear, g_fSkyFogFar);
    }
               
    return Output;
}