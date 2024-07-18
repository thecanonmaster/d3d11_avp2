// shaders\model_VS.hlsl

#include "global_header.hlsl"

VS_PERFRAME_BUFFER_3D

cbuffer C_PEROBJECT_BUFFER : register(b1)
{
    float4x4 g_mModelWorld : packoffset(c0);
    row_major float4x4 g_mModelWorldView : packoffset(c4);
    row_major float4x4 g_mModelWorldViewProj : packoffset(c8);
    float4 g_vDiffuseColor : packoffset(c12);
    row_major float4x4 g_mNormalRef : packoffset(c13);
    uint4 g_vModes : packoffset(c17);
    row_major float4x4 g_amNodeTransforms[MAX_NODE_TRANSFORMS] : packoffset(c18);
#ifndef MODEL_PIXEL_LIGHTING
    float3 g_vDirLightColor : packoffset(c402.x);
    uint g_dwDynamicLightCount : packoffset(c403.x);
    uint g_dwStaticLightCount : packoffset(c403.y);
    uint g_adwDynamicLightIndex[MAX_DYNAMIC_LIGHTS_PER_MODEL] : packoffset(c404);
    StaticLight_VPS g_aStaticLight[MAX_STATIC_LIGHTS_PER_MODEL] : packoffset(c420);  
#endif
}

#ifndef MODEL_PIXEL_LIGHTING
cbuffer C_DYNAMIC_LIGHTS : register(b10)
{
    DynamicLight_VPS g_aDynamicLight[MAX_LIGHTS_PER_BUFF] : packoffset(c0);
    ModelShadowLight_VPS g_aModelShadowLight[MAX_MS_LIGHTS_PER_BUFF] : packoffset(c384);
}

void ApplyLights(inout float3 vLightColor, float3 vWorldPos, float3 vWorldNormal, float3 vWorldNormalNoRef, uint dwMode)
{
    vLightColor = 0.0f;
    
    vLightColor = ColorAdd(vLightColor, GetDirLightColor(vWorldPos, vWorldNormalNoRef, g_vDirLightColor, g_vDirLightDir));
    bool bCloseHack = !(dwMode & MODE_REALLY_CLOSE);
    
    //[unroll(MAX_DYNAMIC_LIGHTS_PER_MODEL)]
    for (uint i = 0; i < g_dwDynamicLightCount; i++)
    {
        vLightColor = ColorAdd(vLightColor, GetDynamicLightColor(vWorldPos, vWorldNormal, g_aDynamicLight[g_adwDynamicLightIndex[i]],
            false, bCloseHack));
    }      
    
    //[unroll(MAX_STATIC_LIGHTS_PER_MODEL)]
    for (i = 0; i < g_dwStaticLightCount; i++)
    {
        vLightColor = ColorAdd(vLightColor, GetStaticLightColor(vWorldPos, vWorldNormal, g_aStaticLight[i], bCloseHack,
            g_aStaticLight[i].m_vDirectionAndType.a));
    }
    
    vLightColor = saturate(vLightColor * 2.0f);
}
#endif

void CalcWorldPositionAndNormal(inout float3 vWorldPosition, inout float3 vWorldNormal, inout float3 vWorldNormalNoRef, float4 vPosition, 
    float3 vNormal, float3 vNormalNoRef, uint dwMode)
{
    if (!(dwMode & MODE_REALLY_CLOSE))
    {
        vWorldPosition = mul(vPosition, g_mModelWorld).xyz;
        vWorldNormal = normalize(mul(vNormal, (float3x3)g_mModelWorld));
        vWorldNormalNoRef = normalize(mul(vNormalNoRef, (float3x3)g_mModelWorld));

    }
    else
    {
        vWorldPosition = mul(mul(vPosition, g_mInvView), g_mModelWorld).xyz;
        vWorldNormal = normalize(mul(mul(vNormal, (float3x3)g_mInvView), (float3x3)g_mModelWorld));
        vWorldNormalNoRef = normalize(mul(mul(vNormalNoRef, (float3x3)g_mInvView), (float3x3)g_mModelWorld));
    }
}

void ComposeVertexPosition(inout float4 vPos, in float4 vTemp)
{
    vTemp.w = 1.0f / vTemp.w;
    
    vPos.x = vTemp.x * vTemp.w;
    vPos.y = vTemp.y * vTemp.w;
    vPos.z = vTemp.z * vTemp.w;
}

void MatVMul_AddLT(inout float4 vOut, in float4x4 mMat, in float4 vVec)
{
    vOut.x += mMat[0][0] * vVec.x + mMat[0][1] * vVec.y + mMat[0][2] * vVec.z + mMat[0][3] * vVec.w;
    vOut.y += mMat[1][0] * vVec.x + mMat[1][1] * vVec.y + mMat[1][2] * vVec.z + mMat[1][3] * vVec.w;
    vOut.z += mMat[2][0] * vVec.x + mMat[2][1] * vVec.y + mMat[2][2] * vVec.z + mMat[2][3] * vVec.w;
    vOut.w += mMat[3][0] * vVec.x + mMat[3][1] * vVec.y + mMat[3][2] * vVec.z + mMat[3][3] * vVec.w;
}

void ApplyNodeTransforms(inout VS_INPUT_MODEL Input)
{    
    float4 vTemp = 0.0f;
    float4 avWeights[4] =
    {
        Input.vBlendWeight0,
        Input.vBlendWeight1,
        Input.vBlendWeight2,
        Input.vBlendWeight3       
    };
    
    [unroll(4)]
    for (uint i = 0; i < Input.dwWeights; i++)
    {
        //MatVMul_AddLT(vTemp, g_amNodeTransforms[Input.dwBlendIndices[i]], avWeights[i]);
        vTemp += mul(g_amNodeTransforms[Input.dwBlendIndices[i]], avWeights[i]);
    }
    
    ComposeVertexPosition(Input.vPosition, vTemp);
     
    // TODO - no idea
    Input.vNormal = normalize(mul(Input.vNormal, (float3x3)g_mNormalRef));
}

PS_INPUT_MODEL VSMain(VS_INPUT_MODEL Input)
{
    PS_INPUT_MODEL Output;
    float3 vWorldNormalNoRef = Input.vNormal;
    
    if (Input.dwWeights > 0)
        ApplyNodeTransforms(Input);
 
    Output.vPosition = mul(Input.vPosition, g_mModelWorldViewProj);
    
    uint dwMode = g_vModes[Input.dwTextureIndex];
    
 #ifdef MODEL_PIXEL_LIGHTING
    CalcWorldPositionAndNormal(Output.vWorldPosition, Output.vWorldNormal, Output.vWorldNormalNoRef, Input.vPosition, Input.vNormal, vWorldNormalNoRef, dwMode);
 #else
    float3 vWorldPosition;
    float3 vWorldNormal;
    
    CalcWorldPositionAndNormal(vWorldPosition, vWorldNormal, vWorldNormalNoRef, Input.vPosition, Input.vNormal, vWorldNormalNoRef, dwMode);
#endif
    
    Output.vTexCoords = Input.vTexCoords;
    Output.vEnvCoords = Input.vTexCoords;
    
#ifdef MODEL_PIECELESS_RENDERING
    Output.dwTextureIndex = Input.dwTextureIndex;
#endif
    
    // TODO - not only RC models
    if (dwMode & MODE_ENV_MAP)
    {
        float4 vPosTimesView = mul(Input.vPosition, g_mView);
        float3 vCameraVector; 
        
        if (dwMode & MODE_REALLY_CLOSE)
            vCameraVector = normalize(-vPosTimesView.xyz);
        else
            vCameraVector = normalize(g_vCameraPos.xyz - vPosTimesView.xyz);
        
        Output.vEnvCoords = reflect(-vCameraVector, Input.vNormal.xyz).xy * g_fEnvScale + g_vCameraPos.xz * g_fEnvPanSpeed;
    }
     
    Output.vDiffuseColor = float4(saturate(g_vDiffuseColor.rgb * 2.0f), g_vDiffuseColor.a);
    
#ifndef MODEL_PIXEL_LIGHTING 
    ApplyLights(Output.vLightColor, vWorldPosition, vWorldNormal, vWorldNormalNoRef, dwMode);
#endif
    
    Output.fFogFactor = 0.0f;
    
    if (dwMode & MODE_FOG_ENABLED)
    {
        if (g_fVFogEnabled == 0.0f)
        {
            float4 vPosRelCamera = mul(Input.vPosition, g_mModelWorldView);
            Output.fFogFactor = ComputeFogFactor(vPosRelCamera.z, g_vFogStartEnd.x, g_vFogStartEnd.y);
        }
        else
        {         
            float4 vPosRelCamera = mul(Input.vPosition, g_mModelWorldView);
            float4 vPosRelWorld = mul(Input.vPosition, g_mModelWorld);
            
            // TODO - not dense enough? zero Z-fog makes Y-fog zero
            Output.fFogFactor = ComputeVFogFactor(g_vCameraPos.y, vPosRelCamera.z, vPosRelWorld.y, g_vFogMinMaxY.x, g_vFogMinMaxY.y, 
                g_vFogMinMaxYVal.x, g_vFogMinMaxYVal.y, g_fVFogDensity, g_fVFogMax);
        }
    }
               
    return Output;
}