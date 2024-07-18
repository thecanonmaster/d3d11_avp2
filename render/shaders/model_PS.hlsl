// shaders\model_PS.hlsl

#include "global_header.hlsl"

Texture2D g_GlobalEnvMapTexture : register(t1);
Texture2D g_OtherTextures[MAX_MODEL_TEXTURES] : register(t5);

SamplerState g_SamplerState : register(s0);

PS_PERFRAME_BUFFER_3D

cbuffer C_PEROBJECT_BUFFER : register(b1)
{
    float4 g_avModeColorAndMode[MAX_MODEL_TEXTURES] : packoffset(c0);
    float4 g_vAlphaRefs : packoffset(c4);
#ifdef MODEL_PIXEL_LIGHTING
    float3 g_vDirLightColor : packoffset(c5.x);
    uint g_dwDynamicLightCount : packoffset(c6.x);
    uint g_dwStaticLightCount : packoffset(c6.y);
    uint g_adwDynamicLightIndex[MAX_DYNAMIC_LIGHTS_PER_MODEL] : packoffset(c7);
    StaticLight_VPS g_aStaticLight[MAX_STATIC_LIGHTS_PER_MODEL] : packoffset(c23);
#endif
}

#ifdef MODEL_PIXEL_LIGHTING
cbuffer C_DYNAMIC_LIGHTS : register(b10)
{
    DynamicLight_VPS g_aDynamicLight[MAX_LIGHTS_PER_BUFF] : packoffset(c0);
    ModelShadowLight_VPS g_aModelShadowLight[MAX_MS_LIGHTS_PER_BUFF] : packoffset(c384);
}

void ApplyLights(inout float3 vColor, float3 vAmbientColor, float3 vWorldPos, float3 vWorldNormal, float3 vWorldNormalNoRef, uint dwMode)
{
    float3 vLightColor = 0.0f;

    vLightColor = ColorAdd(vLightColor, GetDirLightColor(vWorldPos, vWorldNormalNoRef, g_vDirLightColor, g_vDirLightDir));
    
    //[unroll(MAX_DYNAMIC_LIGHTS_PER_MODEL)]
    for (uint i = 0; i < g_dwDynamicLightCount; i++)
    {
        vLightColor = ColorAdd(vLightColor, GetDynamicLightColor(vWorldPos, vWorldNormal, g_aDynamicLight[g_adwDynamicLightIndex[i]], 
            false, !(dwMode & MODE_REALLY_CLOSE)));
    }
    
    //[unroll(MAX_STATIC_LIGHTS_PER_MODEL)]
    for (i = 0; i < g_dwStaticLightCount; i++)
        vLightColor = ColorAdd(vLightColor, GetStaticLightColor(vWorldPos, vWorldNormal, g_aStaticLight[i], g_aStaticLight[i].m_vDirectionAndType.a));
    
    vColor = saturate(ColorModulate(vColor, vAmbientColor) + ColorModulate(vColor, vLightColor, 2.0f));
}
#else
void ApplyLights(inout float3 vColor, uint dwMode, float3 vAmbientColor, float3 vLightColor)
{   
    // TODO - needs clamp?
    float3 vMax = vColor * 1.2f;
    
    vColor = saturate(ColorOp(dwMode, vColor, (vAmbientColor + vLightColor)));
    
    if (!(dwMode & MODE_CUSTOM_COLOR_OPS))
        vColor = clamp(vColor, 0.0f, vMax);
}
#endif

float4 PSMain(PS_INPUT_MODEL Input) : SV_TARGET
{
    float4 vFinalColor;
    
    uint dwMode = asuint(g_avModeColorAndMode[Input.dwTextureIndex].a);
    
    if (!(dwMode & MODE_NO_TEXTURE))
    {
        vFinalColor = SampleTextureArray4(Input.dwTextureIndex, Input.vTexCoords, g_SamplerState, g_OtherTextures);
    
        if (dwMode & MODE_ENV_MAP)
        {
            float4 vEnvMapColor = g_GlobalEnvMapTexture.Sample(g_SamplerState, Input.vEnvCoords);
            
            // TODO - not sure about blending
            vFinalColor = EnvMapAlphaInvOp(vFinalColor, vEnvMapColor);
        }
    }
    else
    {
        vFinalColor = float4(1.0f, 1.0f, 1.0f, 1.0f);
    }
    
    if (!(dwMode & MODE_NO_LIGHT))
#ifdef MODEL_PIXEL_LIGHTING
        ApplyLights(vFinalColor.rgb, Input.vDiffuseColor.rgb, Input.vWorldPosition, Input.vWorldNormal, Input.vWorldNormalNoRef, dwMode);
#else
        ApplyLights(vFinalColor.rgb, dwMode, Input.vDiffuseColor.rgb, Input.vLightColor);
#endif
    
    vFinalColor.a *= Input.vDiffuseColor.a;
     
    if (dwMode & MODE_CHROMAKEY)
        clip((vFinalColor.a < g_vAlphaRefs[Input.dwTextureIndex]) ? -1.0f : 1.0f);
    
    if (!(dwMode & MODE_NO_LIGHT))
        vFinalColor.rgb = ColorModulate(vFinalColor.rgb, g_vGlobalVertexTint);
     
    if (dwMode & MODE_FLAT_COLOR)
        vFinalColor.rgb = g_avModeColorAndMode[Input.dwTextureIndex].rgb;
    else if (dwMode & MODE_FOG_ENABLED)
        ApplyFog(vFinalColor, g_vFogColor, Input.fFogFactor, dwMode);
    
    return vFinalColor;
}