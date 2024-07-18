// shaders\worldmodel_VS.hlsl

#include "global_header.hlsl"

StructuredBuffer<VertexFXData> g_aVertexFXData : register(t0);
StructuredBuffer<LMVertexData> g_aLMVertexData : register(t1);

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

void ApplyLMColorBlends(inout float3 vColor, uint nStartIndex)
{
    uint dwOffset = nStartIndex;
    for (uint i = 0; i < g_dwLMAnimCount; i++)
    {          
        if (g_aLMAnimData[i].m_vFrameDataAndShadowMap.y != UNKNOWN_INDEX && (g_aLMVertexData[dwOffset].m_dwFlags & LMVD_FLAG_USE_COLORS))
        {
            float3 vColor1 = g_aLMVertexData[dwOffset + g_aLMAnimData[i].m_vFrameDataAndShadowMap.y].m_vColor;
            float3 vColor2 = g_aLMVertexData[dwOffset + g_aLMAnimData[i].m_vFrameDataAndShadowMap.z].m_vColor;
            
            float3 vColorBetween = lerp(vColor1, vColor2, g_aLMAnimData[i].m_fBetweenAndBlendPercent.x);
            
            vColor = ColorAdd(vColor, vColorBetween * g_aLMAnimData[i].m_fBetweenAndBlendPercent.y);
        }
        
        dwOffset += g_aLMAnimData[i].m_vFrameDataAndShadowMap.x;
    }
    
    vColor = saturate(vColor);
}

PS_INPUT_WORLD_MODEL VSMain(VS_INPUT_WORLD_MODEL Input)
{
    PS_INPUT_WORLD_MODEL Output;
 
    Output.vPosition = mul(Input.vPosition, g_mModelWorldViewProj);
    Output.vWorldPosition = mul(Input.vPosition, g_mModelWorld).xyz;
    
    uint dwMode = g_adwMode[Input.adwTextureIndices[0]];
    
    if (dwMode & MODE_SURFACE_FX)
        Output.vTexCoords = g_aVertexFXData[Input.adwTextureIndices[3]].m_vTexCoords;
    else
        Output.vTexCoords = Input.vTexCoords;
    
    if (dwMode & MODE_ENV_MAP)
    {
        // TODO - use scaled m_mWorldEnvMap?
        float3 vCameraVector = normalize(g_vCameraPos.xyz - Input.vPosition.xyz);
        Output.vExtraCoords = reflect(-vCameraVector, Input.vNormal).xy * g_fEnvScale;
    }
    else
    {
        Output.vExtraCoords = Input.vExtraCoords;
    }
    
    if (dwMode & MODE_SKY_PAN)
        Output.vSkyPanCoords = (Input.vPosition.xz + g_vPanSkyOffset) * g_vPanSkyScale;
    else
        Output.vSkyPanCoords = Input.vTexCoords;
    
    //Output.vDiffuseColor = Input.vDiffuseColor * g_vDiffuseColor;
    Output.vDiffuseColor = float4(0.0f, 0.0f, 0.0f, g_vDiffuseColor.a);
    
    if (Input.adwTextureIndices[2] != UNKNOWN_INDEX)
        ApplyLMColorBlends(Output.vDiffuseColor.rgb, Input.adwTextureIndices[2]);
          
    Output.vLightMapCoords = Input.vLightMapCoords;
    
    Output.adwTextureIndices = Input.adwTextureIndices;
    Output.fFogFactor = 0.0f;
    
    if (dwMode & MODE_FOG_ENABLED)
    {
        if (g_fVFogEnabled == 0.0f)
        {
            //float4 vPosRelCamera = mul(Input.vPosition, g_mModelWorldView);
            //Output.fFogFactor = ComputeFogFactor(vPosRelCamera.z, g_vFogStartEnd.x, g_vFogStartEnd.y);
            
            float3 vPosRelCamera = mul(Input.vPosition, g_mModelWorldView).xyz;
            float fDistTo = length(vPosRelCamera);
            Output.fFogFactor = ComputeFogFactor(fDistTo, g_vFogStartEnd.x, g_vFogStartEnd.y);
        }
        else
        {
            float4 vPosRelCamera = mul(Input.vPosition, g_mModelWorldView);
            float4 vPosRelWorld = mul(Input.vPosition, g_mModelWorld);
            
            Output.fFogFactor = ComputeVFogFactor(g_vCameraPos.y, vPosRelCamera.z, vPosRelWorld.y, g_vFogMinMaxY.x, g_vFogMinMaxY.y,
                g_vFogMinMaxYVal.x, g_vFogMinMaxYVal.y, g_fVFogDensity, g_fVFogMax);
        }
    }
               
    return Output;
}