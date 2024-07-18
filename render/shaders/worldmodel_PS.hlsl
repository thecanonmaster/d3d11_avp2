// shaders\worldmodel_PS.hlsl

#include "global_header.hlsl"

Texture2D g_GlobalPanTexture : register(t2);

StructuredBuffer<LMVertexData> g_aLMVertexData : register(t3);

Texture2DArray g_LightmapPages : register(t4);
Texture2D g_OtherTextures[WORLD_MODEL_TEXTURES_PER_DRAW] : register(t5);

SamplerState g_SamplerState : register(s0);

PS_PERFRAME_BUFFER_3D

struct ModeColorAndMode
{
    float3 m_vModeColor;
    uint m_dwMode;
};

cbuffer C_PEROBJECT_SUB_BUFFER : register(b9)
{
    ModeColorAndMode g_aModeData[WORLD_MODEL_TEXTURES_PER_DRAW] : packoffset(c0);
    float g_afAlphaRef[WORLD_MODEL_TEXTURES_PER_DRAW] : packoffset(c12);
    uint2 g_vDynLightAndMSLightCount : packoffset(c24);
    uint2 g_vdwDynLightAndMSLightIndex[MAX_LIGHTS_PER_SUB_WORLD_MODEL] : packoffset(c25);
}

cbuffer C_DYNAMIC_LIGHTS : register(b10)
{
    DynamicLight_VPS g_aDynamicLight[MAX_LIGHTS_PER_BUFF] : packoffset(c0);
    ModelShadowLight_VPS g_aModelShadowLight[MAX_MS_LIGHTS_PER_BUFF] : packoffset(c384);
}

void ApplyLMTextureBlends(inout float3 vColor, uint nStartIndex, float2 vBaseTexCoords, float3 vWorldPos)
{ 
    uint dwOffset = nStartIndex;
    
    for (uint i = 0; i < g_dwLMAnimCount; i++)
    {   
        // TODO - cache LMAnimData?
        LMAnimData_PS animData = g_aLMAnimData[i];
        
        if (animData.m_vFrameDataAndShadowMap.y != UNKNOWN_INDEX && (g_aLMVertexData[dwOffset].m_dwFlags & LMVD_FLAG_USE_TEXTURE))
        {
            LMVertexData data1 = g_aLMVertexData[dwOffset + animData.m_vFrameDataAndShadowMap.y];
            LMVertexData data2 = g_aLMVertexData[dwOffset + animData.m_vFrameDataAndShadowMap.z];
            
            float3 vColor1 = (float3)g_LightmapPages.SampleLevel(g_SamplerState, float3(vBaseTexCoords + data1.m_vTexCoordOffsets, (float)data1.m_dwPage), 0);
            float3 vColor2 = (float3)g_LightmapPages.SampleLevel(g_SamplerState, float3(vBaseTexCoords + data2.m_vTexCoordOffsets, (float)data2.m_dwPage), 0);
            
            float3 vColorBetween = lerp(vColor1, vColor2, animData.m_fBetweenAndBlendPercent.x);
            
            if (animData.m_vFrameDataAndShadowMap.w)
                vColorBetween = GetShadowMapLightColor(vWorldPos, vColorBetween, animData, true);
            
            vColor = ColorAdd(vColor, vColorBetween * animData.m_fBetweenAndBlendPercent.y);
        }
        
        dwOffset += animData.m_vFrameDataAndShadowMap.x;
    }
}

void ApplyDynamicLights(inout float3 vColor, float3 vWorldPos)
{
    //[unroll(MAX_LIGHTS_PER_SUB_WORLD_MODEL)]
    for (uint i = 0; i < g_vDynLightAndMSLightCount.x; i++)
    {
        DynamicLight_VPS dynamicLight = g_aDynamicLight[g_vdwDynLightAndMSLightIndex[i].x];
        vColor = ColorAdd(vColor, GetDynamicLightColor(vWorldPos, dynamicLight, true));
    }
}

void ApplyModelShadowLights(inout float3 vColor, float3 vWorldPos, float fMod)
{
    //[unroll(MAX_LIGHTS_PER_SUB_WORLD_MODEL)]
    for (uint i = 0; i < g_vDynLightAndMSLightCount.y; i++)
    {
        ModelShadowLight_VPS shadowLight = g_aModelShadowLight[g_vdwDynLightAndMSLightIndex[i].y];
        vColor = ColorModulate(vColor, GetModelShadowLightColor(vWorldPos, shadowLight, fMod));
    }
}

float4 PSMain(PS_INPUT_WORLD_MODEL Input) : SV_TARGET
{ 
    float4 vFinalColor;
    
    uint dwMainTextureIndex = Input.adwTextureIndices[0];
    uint dwLightMapIndex = Input.adwTextureIndices[2];
    uint dwMode = g_aModeData[dwMainTextureIndex].m_dwMode;
       
    if (!(dwMode & MODE_NO_TEXTURE))
    {
        vFinalColor = SampleTextureArray12(dwMainTextureIndex, Input.vTexCoords, g_SamplerState, 
            g_OtherTextures);
        
        if (dwMode & MODE_DETAIL_TEXTURE)
        {
            float4 vDetailColor = SampleTextureArray12(Input.adwTextureIndices[1], Input.vExtraCoords, g_SamplerState, 
                g_OtherTextures);
            
            vFinalColor.rgb = ColorAdd(vFinalColor.rgb, vDetailColor.rgb, -0.5f);
        }
        else if (dwMode & MODE_ENV_MAP)
        {
            float4 vEnvMapColor = SampleTextureArray12(Input.adwTextureIndices[1], Input.vExtraCoords, g_SamplerState, 
                g_OtherTextures);
            
            if (dwMode & MODE_ENV_MAP_ALPHA)
                vFinalColor.rgb = EnvMapAlphaOp(vFinalColor, vEnvMapColor).rgb;
            else
                vFinalColor.rgb = ColorAdd(vFinalColor.rgb, vEnvMapColor.rgb, -0.5f);
        }
    }
    else
    {
        //vFinalColor = float4(RGBA_GETFR(Input.adwTextureIndices[3]) / 255.0f, 
        //    RGBA_GETFG(Input.adwTextureIndices[3]) / 255.0f,
        //    RGBA_GETFB(Input.adwTextureIndices[3]) / 255.0f, Input.vDiffuseColor.a);

        vFinalColor = float4(1.0f, 1.0f, 1.0f, 1.0f);
        //vFinalColor.rgb = Input.vDiffuseColor.rgb;
        //vFinalColor.a = 1.0f;
    }
    
    if (dwMode & MODE_SKY_PAN)
    {     
        float4 vSkyPanColor = g_GlobalPanTexture.Sample(g_SamplerState, Input.vSkyPanCoords);
        vFinalColor.rgb = ColorModulate(vFinalColor.rgb, vSkyPanColor.rgb);
    }
    
    vFinalColor.a *= Input.vDiffuseColor.a;
    
    if (dwMode & MODE_CHROMAKEY)
        clip((vFinalColor.a < g_afAlphaRef[dwMainTextureIndex]) ? -1.0f : 1.0f);
    
    if (!(dwMode & MODE_NO_LIGHT) && dwLightMapIndex != UNKNOWN_INDEX)
    {   
        float fFullBriteAlpha = (dwMode & MODE_FULL_BRITE) ? vFinalColor.a : 0.0f;
        
        // TODO - a better way to blend lightmaps?
        float3 vMax = saturate(vFinalColor.rgb * 2.0f);
        float3 vLightMapColor = Input.vDiffuseColor.rgb;
        
        ApplyLMTextureBlends(vLightMapColor, dwLightMapIndex, Input.vLightMapCoords, Input.vWorldPosition); 
        ApplyDynamicLights(vLightMapColor, Input.vWorldPosition);
             
        if (dwMode & MODE_FOG_ENABLED)
        {
            ApplyFog(vLightMapColor, g_vFogColorMP, Input.fFogFactor, dwMode);
            ApplyFog(vFinalColor.rgb, g_vFogColorMP, Input.fFogFactor, dwMode);
            
            //vMax = vFinalColor.rgb * 2.0f;
        }

        vFinalColor.rgb = lerp(ColorOpLM(dwMode, vFinalColor.rgb, vLightMapColor), vFinalColor.rgb, fFullBriteAlpha);
        
        // TODO - extra processing for MODE_FULL_BRITE textures?
        vMax = lerp(vFinalColor.rgb, vMax, 0.75f);
        
        if (dwMode & MODE_CUSTOM_COLOR_OPS)
            vFinalColor.rgb = clamp(vFinalColor.rgb, 0.0f, vMax);
    }
    else
    {
        // TODO - not sure
        //ApplyDynamicLights(vFinalColor.rgb, Input.vWorldPosition);
        
        if (dwMode & MODE_FOG_ENABLED)
            ApplyFog(vFinalColor, g_vFogColor, Input.fFogFactor, dwMode); 
    }
    
    ApplyModelShadowLights(vFinalColor.rgb, Input.vWorldPosition, Input.vDiffuseColor.a);
    
    // TODO - test
    if (!(dwMode & MODE_NO_LIGHT))
        vFinalColor.rgb = ColorModulate(vFinalColor.rgb, g_vGlobalVertexTint);
    
    if (dwMode & MODE_FLAT_COLOR)
        vFinalColor.rgb = g_aModeData[dwMainTextureIndex].m_vModeColor;
    
    return vFinalColor;
}