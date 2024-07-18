#ifndef __GLOBAL_HEADER_HLSL__
#define __GLOBAL_HEADER_HLSL__

//#define MODEL_PIXEL_LIGHTING
#define MODEL_PIECELESS_RENDERING

// Optimized surface

struct VS_INPUT_OPTIMIZED
{
    float4 vPosition : SV_POSITION;
    float2 vTexCoords : TEXCOORD0;
};

struct PS_INPUT_OPTIMIZED
{
    float4 vPosition : SV_POSITION;
    float2 vTexCoords : TEXCOORD0;
};

// Optimized surface (batch)

struct VS_INPUT_OPTIMIZED_BAT
{
    float4 vPosition : SV_POSITION;
    float2 vTexCoords : TEXCOORD0;
    uint dwDataIndex : DATAINDEX0;
};

struct PS_INPUT_OPTIMIZED_BAT
{
    float4 vPosition : SV_POSITION;
    float2 vTexCoords : TEXCOORD0;
    uint dwDataIndex : DATAINDEX0;
};

// Fullscreen video

struct VS_INPUT_FSVIDEO
{
    float4 vPosition : SV_POSITION;
    float2 vTexCoords : TEXCOORD0;
};

struct PS_INPUT_FSVIDEO
{
    float4 vPosition : SV_POSITION;
    float2 vTexCoords : TEXCOORD0;
};

// Screen FX

struct PS_INPUT_SCREENFX
{
    float4 vPosition : SV_POSITION;
    float2 vTexCoords : TEXCOORD0;
};

// Solid drawing

struct PS_INPUT_SOLIDDRAW
{
    float4 vPosition : SV_POSITION;
    float2 vTexCoords : TEXCOORD0;
};

// Tone map

struct PS_INPUT_TONEMAP
{
    float4 vPosition : SV_POSITION;
	float2 vTexCoords : TEXCOORD0;
};

// Clear screen

struct PS_INPUT_CLEARSCREEN
{
    float4 vPosition : SV_POSITION;
};

// World model

struct VS_INPUT_WORLD_MODEL
{
    float4 vPosition : SV_POSITION;
    float3 vNormal : NORMAL0;
    float4 vDiffuseColor : COLOR0;
    float2 vTexCoords : TEXCOORD0;
    float2 vExtraCoords : TEXCOORD1;
    float2 vLightMapCoords : TEXCOORD2;
    uint4 adwTextureIndices : TEXINDEX0; 
};

struct PS_INPUT_WORLD_MODEL
{
    float4 vPosition : SV_POSITION;
    float3 vWorldPosition : POSITION0;
    float2 vTexCoords : TEXCOORD0;
    float2 vExtraCoords : TEXCOORD1;
    float2 vLightMapCoords : TEXCOORD2;
    float2 vSkyPanCoords : TEXCOORD3;
    float4 vDiffuseColor : COLOR0;
    float fFogFactor : FOG;
    uint4 adwTextureIndices : TEXINDEX0;
};

struct PS_INPUT_SKY_WORLD_MODEL
{
    float4 vPosition : SV_POSITION;
    float2 vTexCoords : TEXCOORD0;
    float4 vDiffuseColor : COLOR0;
    float fFogFactor : FOG;
    uint dwMainTextureIndex : TEXINDEX0;
};

struct VS_INPUT_SKY_PORTAL
{
    float4 vPosition : SV_POSITION;
};

struct PS_INPUT_SKY_PORTAL
{
    float4 vPosition : SV_POSITION;
};

// Model

struct VS_INPUT_MODEL
{
    float4 vPosition : SV_POSITION;
    float3 vNormal : NORMAL0;
    float2 vTexCoords : TEXCOORD0;
    float4 vBlendWeight0 : BLENDWEIGHT0;
    float4 vBlendWeight1 : BLENDWEIGHT1;
    float4 vBlendWeight2 : BLENDWEIGHT2;
    float4 vBlendWeight3 : BLENDWEIGHT3;
    uint4 dwBlendIndices : BLENDINDICES0;
    uint dwWeights : WEIGHTCOUNT0;
    uint dwTextureIndex : TEXINDEX0;
};

struct PS_INPUT_MODEL
{
    float4 vPosition : SV_POSITION;
 #ifdef MODEL_PIXEL_LIGHTING
    float3 vWorldPosition : POSITION0;
    float3 vWorldNormal : NORMAL0;
    float3 vWorldNormalNoRef : NORNAL1;
 #endif
    float2 vTexCoords : TEXCOORD0;
    float2 vEnvCoords : TEXCOORD1;
    float4 vDiffuseColor : COLOR0;
 #ifndef MODEL_PIXEL_LIGHTING
    float3 vLightColor : COLOR1;
 #endif
    float fFogFactor : FOG;
    uint dwTextureIndex : TEXINDEX0;
};

// Sprite

struct VS_INPUT_SPRITE
{
    float4 vPosition : SV_POSITION;
    float2 vTexCoords : TEXCOORD0;
};

struct PS_INPUT_SPRITE
{
    float4 vPosition : SV_POSITION;
    float2 vTexCoords : TEXCOORD0;
    float fFogFactor : FOG;
};

// Sprite (batch)

struct VS_INPUT_SPRITE_BAT
{
    float4 vPosition : SV_POSITION;
    float2 vTexCoords : TEXCOORD0;
    uint dwDataIndex : DATAINDEX0;

};

struct PS_INPUT_SPRITE_BAT
{
    float4 vPosition : SV_POSITION;
    float2 vTexCoords : TEXCOORD0;
    float fFogFactor : FOG;
    uint dwDataIndex : DATAINDEX0;
};

// Particle system

struct VS_INPUT_PARTICLESYSTEM
{
    float2 vTexCoords : TEXCOORD0;
    float4 vPosition : SV_POSITION;
    float4 vDiffuseColor : COLOR0;
    float fScale : COLOR1;
};

struct PS_INPUT_PARTICLESYSTEM
{
    float4 vPosition : SV_POSITION;
    float2 vTexCoords : TEXCOORD0;
    float4 vDiffuseColor : COLOR0;
    float fFogFactor : FOG;
};

// Canvas

struct VS_INPUT_CANVAS
{
    float4 vPosition : SV_POSITION;
    float2 vTexCoords : TEXCOORD0;
    float4 vDiffuseColor : COLOR0;
};

struct PS_INPUT_CANVAS
{
    float4 vPosition : SV_POSITION;
    float2 vTexCoords : TEXCOORD0;
    float4 vDiffuseColor : COLOR0;
    float fFogFactor : FOG;
};

// Canvas (batch)

struct VS_INPUT_CANVAS_BAT
{
    float4 vPosition : SV_POSITION;
    float2 vTexCoords : TEXCOORD0;
    float4 vDiffuseColor : COLOR0;
    uint dwDataIndex : DATAINDEX0;
};

struct PS_INPUT_CANVAS_BAT
{
    float4 vPosition : SV_POSITION;
    float2 vTexCoords : TEXCOORD0;
    float4 vDiffuseColor : COLOR0;
    float fFogFactor : FOG;
    uint dwDataIndex : DATAINDEX0;
};

// Poly grid
struct VS_INPUT_POLYGRID
{
    float4 vPosition : SV_POSITION;
    float3 vNormal : NORMAL0;
    float2 vTexCoords : TEXCOORD0;
    float4 vDiffuseColor : COLOR0;
};

struct PS_INPUT_POLYGRID
{
    float4 vPosition : SV_POSITION;
    float4 vDiffuseColor : COLOR0;
    float2 vTexCoords : TEXCOORD0;
    float2 vEnvCoords : TEXCOORD1;  
    float fFogFactor : FOG;
};

// Line system

struct VS_INPUT_LINESYSTEM
{
    float4 vPosition : SV_POSITION;
    float4 vDiffuseColor : COLOR0;
};

struct PS_INPUT_LINESYSTEM
{
    float4 vPosition : SV_POSITION;
    float4 vDiffuseColor : COLOR0;
    float fFogFactor : FOG;
};
    
// Pass through

struct PS_INPUT_PASSTHROUGH
{
    float4 vPosition : SV_POSITION;
    float2 vTexCoords : TEXCOORD0;
};

// Bloom

struct PS_INPUT_BLOOM
{
    float4 vPosition : SV_POSITION;
    float2 vTexCoords : TEXCOORD0;
};

// Globals

struct LMAnimData_VS
{
    uint4 m_vFrameDataAndShadowMap;
    float2 m_fBetweenAndBlendPercent;
};

struct LMAnimData_PS
{
    uint4 m_vFrameDataAndShadowMap;
    float2 m_fBetweenAndBlendPercent;
    float3 m_vLightPos;
    float4 m_vLightColorAndRadius;
};

#define MAX_LIGHT_ANIMS_PER_BUFF        32
#define MAX_LIGHTS_PER_BUFF             192
#define MAX_MS_LIGHTS_PER_BUFF          32
#define MAX_DYNAMIC_LIGHTS_PER_MODEL    16
#define MAX_STATIC_LIGHTS_PER_MODEL     16

#define MAX_LIGHTS_PER_SUB_WORLD_MODEL        32
#define MAX_MS_LIGHTS_PER_SUB_WORLD_MODEL     16

#define MAX_MODEL_TEXTURES      4
#define MAX_PIECES_PER_MODEL    32

#ifndef MODEL_PIXEL_LIGHTING

#define VS_PERFRAME_BUFFER_3D cbuffer C_PERFRAME_BUFFER : register(b0)\
{\
    float2 g_vScreenDims : packoffset(c0.x);\
    float2 g_vFogStartEnd : packoffset(c0.z);\
    float4x4 g_mView : packoffset(c1);\
    float4x4 g_mInvView : packoffset(c5);\
    float4 g_vCameraPos : packoffset(c9);\
    float g_fEnvScale : packoffset(c10.x);\
    float g_fEnvPanSpeed : packoffset(c10.y);\
    float g_fVFogEnabled : packoffset(c10.z);\
    float g_fVFogDensity : packoffset(c10.w);\
    float2 g_vFogMinMaxY : packoffset(c11.x);\
    float2 g_vFogMinMaxYVal : packoffset(c11.z);\
    float g_fVFogMax : packoffset(c12.x);\
    float g_fSkyFogNear : packoffset(c12.y);\
    float g_fSkyFogFar : packoffset(c12.z);\
    float2 g_vPanSkyScale : packoffset(c13.x);\
    float2 g_vPanSkyOffset : packoffset(c13.z);\
    float3 g_vDirLightDir : packoffset(c14.x);\
    uint g_dwLMAnimCount : packoffset(c14.w);\
    LMAnimData_VS g_aLMAnimData[MAX_LIGHT_ANIMS_PER_BUFF] : packoffset(c15);\
}

#define PS_PERFRAME_BUFFER_3D cbuffer C_PERFRAME_BUFFER : register(b0)\
{\
    float3 g_vFogColor : packoffset(c0.x);\
    float3 g_vLightAdd : packoffset(c1.x);\
    float g_fInvertHack : packoffset(c1.w);\
    float3 g_vLightScale : packoffset(c2.x);\
    float3 g_vGlobalVertexTint : packoffset(c3.x);\
    float3 g_vFogColorMP : packoffset(c4.x);\
    uint g_dwLMAnimCount : packoffset(c4.w);\
    LMAnimData_PS g_aLMAnimData[MAX_LIGHT_ANIMS_PER_BUFF] : packoffset(c5);\
}

#else

#define VS_PERFRAME_BUFFER_3D cbuffer C_PERFRAME_BUFFER : register(b0)\
{\
    float2 g_vScreenDims : packoffset(c0.x);\
    float2 g_vFogStartEnd : packoffset(c0.z);\
    float4x4 g_mView : packoffset(c1);\
    float4x4 g_mInvView : packoffset(c5);\
    float4 g_vCameraPos : packoffset(c9);\
    float g_fEnvScale : packoffset(c10.x);\
    float g_fEnvPanSpeed : packoffset(c10.y);\
    float g_fVFogEnabled : packoffset(c10.z);\
    float g_fVFogDensity : packoffset(c10.w);\
    float2 g_vFogMinMaxY : packoffset(c11.x);\
    float2 g_vFogMinMaxYVal : packoffset(c11.z);\
    float g_fVFogMax : packoffset(c12.x);\
    float g_fSkyFogNear : packoffset(c12.y);\
    float g_fSkyFogFar : packoffset(c12.z);\
    float2 g_vPanSkyScale : packoffset(c13.x);\
    float2 g_vPanSkyOffset : packoffset(c13.z);\
    uint g_dwLMAnimCount : packoffset(c14.x);\
    LMAnimData_VS g_aLMAnimData[MAX_LIGHT_ANIMS_PER_BUFF] : packoffset(c15);\
}

#define PS_PERFRAME_BUFFER_3D cbuffer C_PERFRAME_BUFFER : register(b0)\
{\
    float3 g_vFogColor : packoffset(c0.x);\
    float3 g_vLightAdd : packoffset(c1.x);\
    float g_fInvertHack : packoffset(c1.w);\
    float3 g_vLightScale : packoffset(c2.x);\
    float3 g_vGlobalVertexTint : packoffset(c3.x);\
    float3 g_vFogColorMP : packoffset(c4.x);\
    float3 g_vDirLightDir : packoffset(c5.x);\
    uint g_dwLMAnimCount : packoffset(c5.w);\
    LMAnimData_PS g_aLMAnimData[MAX_LIGHT_ANIMS_PER_BUFF] : packoffset(c6);\
}

#endif

#define VS_PERFRAME_BUFFER_2D cbuffer C_PERFRAME_BUFFER : register(b0)\
{\
    float2 g_vScreenDims : packoffset(c0);\
}

#define PS_PERFRAME_BUFFER_2D cbuffer C_PERFRAME_BUFFER : register(b0)\
{\
    float g_fLinearExposure : packoffset(c0);\
}

struct LMVertexData
{
    float3 m_vColor;
    float2 m_vTexCoordOffsets;
    uint m_dwPage;
    uint m_dwFlags;
};

struct VertexFXData
{
    float2 m_vTexCoords;
};

struct OptimizedSurface_PS
{
    float4 m_vTransparentColorAndAlpha;
    float4 m_vBaseColorAndTextureIndex;
};

struct Canvas_VPS
{
    uint2 m_vModeAndTextureIndex;
    float3 m_vModeColor;
};

struct Sprite_VPS
{
    float3 m_vModeColor;
    float4 m_vDiffuseColor;
    uint2 m_vModeAndTextureIndex;
};

struct DynamicLight_VPS
{
    float3 m_vPosition;
    float4 m_vColorAndRadius;
};

struct ModelShadowLight_VPS
{
    float4 m_vPositionAndRadius;
};

struct StaticLight_VPS
{
    float3 m_vPosition;
    float4 m_vInnerColorAndRadius;
    float4 m_vOuterColorAndFOV;
    float4 m_vDirectionAndType;
};

#define MAX_NODE_TRANSFORMS                 96
#define WORLD_MODEL_TEXTURES_PER_DRAW	    12

#define MAX_LM_PAGES  2

#define MAX_BLOOM_SAMPLES  15

#define OPTIMIZED_SURFACE_BATCH_SIZE            128
#define OPTIMIZED_SURFACE_TEXTURES_PER_DRAW	    12
#define CANVAS_BATCH_SIZE                       96
#define CANVAS_TEXTURES_PER_DRAW	            12
#define SPRITE_BATCH_SIZE                       64
#define SPRITE_TEXTURES_PER_DRAW	            12

#define UNKNOWN_INDEX           0xFFFFFFFF
#define LMVD_FLAG_USE_COLORS    (1<<0)
#define LMVD_FLAG_USE_TEXTURE   (1<<1)

#define RGBA_GETFA(color)		((float)( color               >> 24))
#define RGBA_GETFR(color)		((float)((color & 0x00FF0000) >> 16))
#define RGBA_GETFG(color)		((float)((color & 0x0000FF00) >> 8 ))
#define RGBA_GETFB(color)		((float)((color & 0x000000FF)      ))

#define MODE_REALLY_CLOSE   (1<<0)
#define MODE_FOG_ENABLED    (1<<1)
#define MODE_NO_TEXTURE     (1<<2)
#define MODE_ENV_MAP        (1<<3)
#define MODE_ENV_MAP_ONLY   (1<<4)
#define MODE_BLEND_MULTIPLY (1<<5)
#define MODE_ADD_SIGNED		(1<<6)
#define MODE_NO_LIGHT		(1<<7)
#define MODE_FLAT_COLOR		(1<<8)
#define MODE_CHROMAKEY		(1<<9)
#define MODE_DETAIL_TEXTURE	(1<<10)
#define MODE_FULL_BRITE     (1<<11)
#define MODE_SURFACE_FX     (1<<12)
#define MODE_SKY_OBJECT     (1<<13)
#define MODE_ENV_MAP_ALPHA  (1<<14)
#define MODE_SKY_PAN        (1<<15)
#define MODE_MODULATE2X     (1<<16)
#define MODE_BLEND_ADDITIVE (1<<17)

#define MODE_CUSTOM_COLOR_OPS   (MODE_ADD_SIGNED | MODE_MODULATE2X)

float4 ColorModulate(float4 vColor1, float4 vColor2)
{
    return vColor1 * vColor2;
}

float3 ColorModulate(float3 vColor1, float3 vColor2)
{
    return vColor1 * vColor2;
}

float4 ColorModulate(float4 vColor1, float4 vColor2, float fMod)
{
    return vColor1 * vColor2 * fMod;
}

float3 ColorModulate(float3 vColor1, float3 vColor2, float fMod)
{
    return vColor1 * vColor2 * fMod;
}

float4 ColorAdd(float4 vColor1, float4 vColor2)
{
    return vColor1 + vColor2;
}

float3 ColorAdd(float3 vColor1, float3 vColor2)
{
    return vColor1 + vColor2;
}

float4 ColorAdd(float4 vColor1, float4 vColor2, float fMod)
{
    return vColor1 + vColor2 + fMod;
}

float3 ColorAdd(float3 vColor1, float3 vColor2, float fMod)
{
    return vColor1 + vColor2 + fMod;
}

float4 ColorOp(uint dwMode, float4 vColor1, float4 vColor2)
{
    if (dwMode & MODE_ADD_SIGNED)
        return ColorAdd(vColor1, vColor2, -0.5f);
    else if (dwMode & MODE_MODULATE2X)
        return ColorModulate(vColor1, vColor2, 2.0f);
    else
        return ColorModulate(vColor1, vColor2);
}

float3 ColorOp(uint dwMode, float3 vColor1, float3 vColor2)
{
    if (dwMode & MODE_ADD_SIGNED)
        return ColorAdd(vColor1, vColor2, -0.5f);
    else if (dwMode & MODE_MODULATE2X)
        return ColorModulate(vColor1, vColor2, 2.0f);
    else
        return ColorModulate(vColor1, vColor2);
}

float3 ColorOpLM(uint dwMode, float3 vColor1, float3 vColor2)
{
    // TODO - not even close to the original
    if (dwMode & MODE_ADD_SIGNED)
        return ColorAdd(vColor1, vColor2 * 2.0f, -0.5f);
    else if (dwMode & MODE_MODULATE2X)
        return ColorModulate(vColor1, vColor2, 4.0f);
    else
        return ColorModulate(vColor1, vColor2, 2.0f);
}

float4 EnvMapAlphaInvOp(float4 vColor, float4 vEnvColor)
{
    return saturate(vColor * vColor.a + vEnvColor * (1.0f - vColor.a));
}

float4 EnvMapAlphaOp(float4 vColor, float4 vEnvColor)
{
    return saturate(vColor + vEnvColor * vColor.a);
}

float ComputeVFogFactor(float fCameraY, float fDistOrZ, float fVertexY, float fFogMinY, float fFogMaxY,
    float fFogMinYVal, float fFogMaxYVal, float fDensity, float fFogMax)
{
    float fFogDiff = fFogMaxY - fFogMinY;
    float fFogValDiff = fFogMaxYVal - fFogMinYVal;
    
    float fCamFogFactorY = saturate((fCameraY - fFogMinY) / fFogDiff);
    float fCamFogFactorYModded = fCamFogFactorY * fFogValDiff + fFogMinYVal;
    
    float fFogFactorY = saturate((fVertexY - fFogMinY) / fFogDiff);
    float fFogFactorYModded = fFogFactorY * fFogValDiff + fFogMinYVal;
    
    float fFogFactorZ = saturate(fDistOrZ / fDensity);
    
    return max(fCamFogFactorYModded, fFogFactorYModded) * fFogFactorZ * fFogMax;
}

float ComputeFogFactor(float fDistOrZ, float fFogStart, float fFogEnd)
{
    return saturate((fFogStart - fDistOrZ) / (fFogStart - fFogEnd));
}

void ApplyFog(inout float4 vColor, float3 vFogColor, float fFogFactor, uint dwMode)
{
    // TODO - vFogColor * vColor.a ?
    if (!(dwMode & MODE_BLEND_ADDITIVE))
        vColor.rgb = lerp(vColor.rgb, vFogColor, fFogFactor);
    else
        vColor.rgb *= (1.0f - fFogFactor);
}

void ApplyFog(inout float3 vColor, float3 vFogColor, float fFogFactor, uint dwMode)
{
    if (!(dwMode & MODE_BLEND_ADDITIVE))
        vColor.rgb = lerp(vColor.rgb, vFogColor, fFogFactor);
    else
        vColor.rgb *= (1.0f - fFogFactor);
}

float3 GetMultiPassFogColor(float3 vColor)
{
    float3 vResult;
    
    vResult.r = sqrt(vColor.r * 255.0f * 0.001953125f);
    vResult.g = sqrt(vColor.g * 255.0f * 0.001953125f);
    vResult.b = sqrt(vColor.b * 255.0f * 0.001953125f);
    
    return vResult;
}

float4 SampleTextureArray4(uint dwIndex, float2 vTexCoords, SamplerState samplerState,
    Texture2D textureArray[4])
{
    switch (dwIndex)
    {
        case 1: return textureArray[1].Sample(samplerState, vTexCoords);
        case 2: return textureArray[2].Sample(samplerState, vTexCoords);
        case 3: return textureArray[3].Sample(samplerState, vTexCoords);
        
        case 0:
        default: return textureArray[0].Sample(samplerState, vTexCoords);
    }
}

float4 SampleTextureArray12(uint dwIndex, float2 vTexCoords, SamplerState samplerState, 
    Texture2D textureArray[12])
{ 
    switch (dwIndex)
    {
        case 1: return textureArray[1].Sample(samplerState, vTexCoords);
        case 2: return textureArray[2].Sample(samplerState, vTexCoords);
        case 3: return textureArray[3].Sample(samplerState, vTexCoords);
        case 4: return textureArray[4].Sample(samplerState, vTexCoords);
        case 5: return textureArray[5].Sample(samplerState, vTexCoords);
        case 6: return textureArray[6].Sample(samplerState, vTexCoords);
        case 7: return textureArray[7].Sample(samplerState, vTexCoords);
        case 8: return textureArray[8].Sample(samplerState, vTexCoords);
        case 9: return textureArray[9].Sample(samplerState, vTexCoords);
        case 10: return textureArray[10].Sample(samplerState, vTexCoords);
        case 11: return textureArray[11].Sample(samplerState, vTexCoords);
        
        case 0:
        default: return textureArray[0].Sample(samplerState, vTexCoords);
    }
}

float3 GetShadowMapLightColor(float3 vWorldPos, float3 vShadowMap, LMAnimData_PS animData, bool bBlackLights)
{  
    float3 vToLight = animData.m_vLightPos - vWorldPos;
    float fDist = length(vToLight);
    float3 vFinalColor;
    
    if (bBlackLights)
        vFinalColor = saturate((animData.m_vLightColorAndRadius.rgb - 0.5f) * 2.4f);   
    else
        vFinalColor = animData.m_vLightColorAndRadius.rgb * 1.2f;       
    
    float fDistNorm = 1.0f - saturate(fDist * (1.0f / animData.m_vLightColorAndRadius.a));
    vFinalColor = vFinalColor * fDistNorm * vShadowMap;
  
    return vFinalColor;
}

float3 GetDynamicLightColor(float3 vWorldPos, DynamicLight_VPS dynamicLight, bool bBlackLights)
{
    float3 vToLight = dynamicLight.m_vPosition.xyz - vWorldPos;
    float fDist = length(vToLight);
    float3 vFinalColor;

    if (bBlackLights)
        vFinalColor = (dynamicLight.m_vColorAndRadius.rgb - 0.5f) * 2.4f;
    else
        vFinalColor = dynamicLight.m_vColorAndRadius.rgb * 1.2f;
    
    float fDistNorm = 1.0f - saturate(fDist * (1.0f / dynamicLight.m_vColorAndRadius.a));
    vFinalColor *= fDistNorm;
  
    return vFinalColor;
}

float3 GetModelShadowLightColor(float3 vWorldPos, ModelShadowLight_VPS shadowLight, float fMod)
{
    float3 vToLight = shadowLight.m_vPositionAndRadius.xyz - vWorldPos;
    float fDist = length(vToLight);
    
    float fDistNorm = 1.0f - saturate(fDist * (1.0f / shadowLight.m_vPositionAndRadius.a));
    float3 vFinalColor = 0.6f * (fDistNorm * fDistNorm);
  
    return (1.0f - vFinalColor) * fMod;
}

float3 GetDynamicLightColor(float3 vWorldPos, float3 vWorldNormal, DynamicLight_VPS dynamicLight, 
    bool bBlackLights, bool bCloseHack)
{
    float3 vToLight = dynamicLight.m_vPosition.xyz - vWorldPos;
    float fDist = length(vToLight);
    vToLight /= fDist;
    
    float fDistNorm = 1.0f - saturate(fDist * (1.0f / dynamicLight.m_vColorAndRadius.a));
    float fIntensity = saturate(dot(vToLight, vWorldNormal));
    
    if (bCloseHack)
        fIntensity += pow(fDistNorm, 5.0f);
    
    float3 vFinalColor;

    if (bBlackLights)
        vFinalColor = (dynamicLight.m_vColorAndRadius.rgb - 0.5f) * 2.0f * fIntensity;
    else
        vFinalColor = dynamicLight.m_vColorAndRadius.rgb * fIntensity;
    
    vFinalColor *= fDistNorm;
  
    return vFinalColor;
}

float3 GetStaticLightColor(float3 vWorldPos, float3 vWorldNormal, StaticLight_VPS staticLight, bool bCloseHack, float fType)
{
    float3 vToLight = staticLight.m_vPosition.xyz - vWorldPos;
    float fDist = length(vToLight);
    vToLight /= fDist;
    
    float3 vFinalColor = 0.0f;
    
    if (fType == 1.0f)
    {
        float fDistNorm = 1.0f - saturate(fDist * (1.0f / staticLight.m_vInnerColorAndRadius.a));
        float fIntensity = saturate(dot(vToLight, vWorldNormal));
        
        if (bCloseHack)
            fIntensity += pow(fDistNorm, 5.0f);
             
        vFinalColor = staticLight.m_vInnerColorAndRadius.rgb * fIntensity;       
        vFinalColor *= fDistNorm;
    }
    else
    {
        vFinalColor = 0.0f;
        float fDot = -dot(vToLight, staticLight.m_vDirectionAndType.rgb);
        
        float fFOV = staticLight.m_vOuterColorAndFOV.a;
        
        if (fDot >= fFOV)
        {
            float fDistNorm = 1.0f - saturate(fDist * (1.0f / staticLight.m_vInnerColorAndRadius.a));
            float fIntensity = saturate(dot(vToLight, vWorldNormal));
            
            if (bCloseHack)
                fIntensity += pow(fDistNorm, 5.0f);
            
            float3 vInnerColor = staticLight.m_vInnerColorAndRadius.rgb * fIntensity;
            float3 vOuterColor = staticLight.m_vOuterColorAndFOV.rgb * fIntensity;
            
            float fAngleMod = (1.0f - (fDot + 1.0f) * 0.5f) / (1.0f - (fFOV + 1.0f) * 0.5f);
            
            vFinalColor.r = (1.0f - fAngleMod) * vInnerColor.r + fAngleMod * vOuterColor.r;
            vFinalColor.g = (1.0f - fAngleMod) * vInnerColor.g + fAngleMod * vOuterColor.g;
            vFinalColor.b = (1.0f - fAngleMod) * vInnerColor.b + fAngleMod * vOuterColor.b;
            
            // TODO - cone smooth edge?    
            //vFinalColor *= fDot;
            vFinalColor *= fDistNorm * 0.25f;
        }
    }

    return vFinalColor;
}

float3 GetDirLightColor(float3 vWorldPos, float3 vWorldNormal, float3 vColor, float3 vDir)
{
    //return vColor * pow(saturate(-dot(vDir, vWorldNormal)), 0.6f);
    return (vColor * 0.1f) + (vColor * pow(saturate(-dot(vDir, vWorldNormal)), 0.6f));
}

#endif