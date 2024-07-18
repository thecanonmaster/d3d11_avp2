#include "pch.h"

#ifndef __D3D11_H__
#include <d3d11_1.h>
#define __D3D11_H__
#endif

#include "rendererconsolevars.h"

ConVar<int> g_CV_D3D11_ModeScaling("D3D11_ModeScaling", -1);
ConVar<int> g_CV_D3D11_Format("D3D11_Format", -1);
ConVar<float> g_CV_D3D11_RefreshRate("D3D11_RefreshRate", 144.0f);
ConVar<float> g_CV_D3D11_GammaLevel("D3D11_GammaLevel", 1.0f);

ConVar<int> g_CV_TripleBuffer("TripleBuffer", 0);
ConVar<int> g_CV_VSync("VSync", 0);

ConVar<int> g_CV_MaxFPS("MaxFPS", 60);
ConVar<int> g_CV_SetCurrentFPS("SetCurrentFPS", 0);

ConVar<int> g_CV_LoseFocusNoShutdown("LoseFocusNoShutdown", 1);
ConVar<int> g_CV_NoLockOnFlip("NoLockOnFlip", 1);

ConVar<int> g_CV_SolidDrawingMode("SolidDrawingMode", SDM_SingleMenuFastGame);
ConVar<int> g_CV_QueueObjectsMode("QueueObjectsMode", QOM_VisQueryEx);
ConVar<int> g_CV_VQE_AlwaysDrawSprites("VQE_AlwaysDrawSprites", 0);

ConVar<int> g_CV_RenderDebug("RenderDebug", 0);
ConVar<int> g_CV_RenderDebugFile("RenderDebugFile", 1);

#if defined(DEBUG) || defined(_DEBUG)
ConVar<float> g_CV_AlmostBlackVoid("AlmostBlackVoid", 0.0f);
#endif

ConVar<int> g_CV_MoviesKeepAspectRatio("MoviesKeepAspectRatio", 1);

ConVar<int> g_CV_CacheTextures("CacheTextures", 0);

ConVar<int> g_CV_TraceConsole("TraceConsole", 1);

ConVar<int> g_CV_Saturate("Saturate", 1);

ConVar<float> g_CV_MipMapBias("MipMapBias", 0.0f);
ConVar<int> g_CV_Bilinear("Bilinear", 1);
ConVar<int> g_CV_Anisotropic("Anisotropic", D3D11_MAX_MAXANISOTROPY >> 1);
ConVar<int> g_CV_FilterUnoptimized("FilterUnoptimized", 0);
ConVar<int> g_CV_FilterOptimized("FilterOptimized", 0);

ConVar<float> g_CV_ModelSunVariance("ModelSunVariance", 0.05f);

ConVar<float> g_CV_ModelZoomScale("ModelZoomScale", 1.0f);
ConVar<int> g_CV_ModelLODOffset("ModelLODOffset", 0);
ConVar<int> g_CV_ModelNoLODs("ModelNoLODs", 1);

ConVar<int> g_CV_MaxModelShadows("MaxModelShadows", 1);
ConVar<float> g_CV_ModelShadowLightScale("ModelShadowLightScale", 4.0f);

ConVar<int> g_CV_TextureModels("TextureModels", 1);

ConVar<float> g_CV_ExtraFOVXOffset("ExtraFOVXOffset", 0.0f);
ConVar<float> g_CV_ExtraFOVYOffset("ExtraFOVYOffset", 0.0f);

ConVar<float> g_CV_NearZ("NearZ", 7.0f);
ConVar<float> g_CV_ReallyCloseNearZ("ReallyCloseNearZ", 0.01f);

ConVar<float> g_CV_RCSpriteFOVOffset("RCSpriteFOVOffset", 0.0f);
ConVar<float> g_CV_RCCanvasFOVOffset("RCCanvasFOVOffset", 0.0f);
ConVar<float> g_CV_RCParticlesFOVOffset("RCParticlesFOVOffset", 0.0f);

ConVar<int> g_CV_Wireframe("Wireframe", 0);

ConVar<int> g_CV_InvertHack("InvertHack", 0);
ConVar<int> g_CV_LMFullBright("LMFullBright", 0);

ConVar<int> g_CV_UnlimitedVS("UnlimitedVS", 1);

ConVar<int> g_CV_DynamicLight("DynamicLight", 1);
ConVar<int> g_CV_BlackDynamicLight("BlackDynamicLight", 0);

ConVar<float> g_CV_SkyScale("SkyScale", 1.0f);
ConVar<float> g_CV_SkyFarZ("SkyFarZ", 300000.0f);

ConVar<int> g_CV_EnvMapPolyGrids("EnvMapPolyGrids", 1);

ConVar<int> g_CV_DetailTextures("DetailTextures", 1);
ConVar<float> g_CV_DetailTextureScale("DetailTextureScale", 0.2f);
ConVar<int> g_CV_DetailTextureAdd("DetailTextureAdd", 1);

ConVar<int> g_CV_EnvMapWorld("EnvMapWorld", 1);
ConVar<int> g_CV_EnvMapEnable("EnvMapEnable", 1);
ConVar<float> g_CV_EnvScale("EnvScale", 1.0f);
ConVar<float> g_CV_EnvPanSpeed("EnvPanSpeed", 0.0005f);

ConVar<int> g_CV_DrawGuns("DrawGuns", 1);
ConVar<int> g_CV_DrawSorted("DrawSorted", 1);

ConVar<int> g_CV_DrawModels("DrawModels", 1);
ConVar<int> g_CV_DrawWorldModels("DrawWorldModels", 1);
ConVar<int> g_CV_DrawWorld("DrawWorld", 1);
ConVar<int> g_CV_DrawSky("DrawSky", 1);
ConVar<int> g_CV_DrawLineSystems("DrawLineSystems", 1);
ConVar<int> g_CV_DrawSprites("DrawSprites", 1);
ConVar<int> g_CV_DrawParticles("DrawParticles", 1);
ConVar<int> g_CV_DrawCanvases("DrawCanvases", 1);
ConVar<int> g_CV_DrawPolyGrids("DrawPolyGrids", 1);

ConVar<int> g_CV_ShowRenderedObjectCounts("ShowRenderedObjectCounts", 0);

ConVar<int> g_CV_DynamicLightSprites("DynamicLightSprites", 1);
ConVar<float> g_CV_ModelVertexBufferTTL("ModelVertexBufferTTL", 360.0f);
ConVar<float> g_CV_WorldModelVertexBufferTTL("WorldModelVertexBufferTTL", 360.0f);
ConVar<float> g_CV_SpriteVertexBufferTTL("SpriteVertexBufferTTL", 180.0f);
ConVar<float> g_CV_ParticleSystemVertexBufferTTL("ParticleSystemBufferTTL", 180.0f);
ConVar<float> g_CV_CanvasVertexBufferTTL("CanvasVertexBufferTTL", 180.0f);
ConVar<float> g_CV_LineSystemVertexBufferTTL("LineSystemVertexBufferTTL", 180.0f);
ConVar<float> g_CV_PolyGridVertexBufferTTL("PolyGridVertexBufferTTL", 180.0f);

ConVar<int> g_CV_FogEnable("FogEnable", 0);
ConVar<int> g_CV_FogColorR("FogR", 255);
ConVar<int> g_CV_FogColorG("FogG", 255);
ConVar<int> g_CV_FogColorB("FogB", 255);
ConVar<float> g_CV_FogNearZ("FogNearZ", 0.0f);
ConVar<float> g_CV_FogFarZ("FogFarZ", 2000.0f);

ConVar<float> g_CV_SkyFogNearZ("SkyFogNearZ", 0.0f);
ConVar<float> g_CV_SkyFogFarZ("SkyFogFarZ", 2000.0f);

ConVar<int> g_CV_VFog("VFog", 0);
ConVar<float> g_CV_VFogMinY("VFogMinY", 0.0f);
ConVar<float> g_CV_VFogMaxY("VFogMaxY", 1300.0f);
ConVar<float> g_CV_VFogMinYVal("VFogMinYVal", 0.5f);
ConVar<float> g_CV_VFogMaxYVal("VFogMaxYVal", 0.0f);
ConVar<float> g_CV_VFogMax("VFogMax", 180.0f);
ConVar<float> g_CV_VFogDensity("VFogDensity", 1800.0f);

ConVar<int> g_CV_UseMipMapsInUse("UseMipMapsInUse", 1);

ConVar<int> g_CV_LockPVS("LockPVS", 0);
ConVar<int> g_CV_FixTJunc("FixTJunc", 1);

ConVar<int> g_CV_CacheObjects("CacheObjects", 1);
#if defined(DEBUG) || defined(_DEBUG)
ConVar<int> g_CV_VisBoxTest("VisBoxTest", 0);
#endif
ConVar<int> g_CV_FrustumTest("FrustumTest", 0);
ConVar<float> g_CV_VisQueryViewRadius("VisQueryViewRadius", 10000.0f);
ConVar<int> g_CV_SkyPortalHack("SkyPortalHack", 2);
ConVar<float> g_CV_RBExtraBBoxMargin("RBExtraBBoxMargin", 1000.0f);
ConVar<int> g_CV_RBExtraBBoxFrustumTest("RBExtraBBoxFrustumTest", 0);
ConVar<int> g_CV_TwoPassSkyAlpha("TwoPassSkyAlpha", 0);
ConVar<float> g_CV_UntexturedTWMAlphaMod("UntexturedTWMAlphaMod", 0.33f);

ConVar<int> g_CV_BatchOptimizedSurfaces("BatchOptimizedSurfaces", 1);
ConVar<int> g_CV_BatchCanvases("BatchCanvases", 1);
ConVar<int> g_CV_BatchSprites("BatchSprites", 1);

ConVar<float> g_CV_BloomThreshold("BloomThreshold", 0.7f); 
ConVar<float> g_CV_BloomBlurSize("BloomBlurSize", 5.0f);
ConVar<float> g_CV_BloomBlurBrightness("BloomBlurBrightness", 1.1f);
ConVar<float> g_CV_BloomIntensity("BloomIntensity", 1.1f);
ConVar<float> g_CV_BloomBaseIntensity("BloomBaseIntensity", 1.0f);
ConVar<float> g_CV_BloomSaturation("BloomSaturation", 1.0f);
ConVar<float> g_CV_BloomBaseSaturation("BloomBaseSaturation", 1.0f);

#if defined(DEBUG) || defined(_DEBUG)
ConVar<float> g_CV_ShowPosStats("ShowPosStats", 0.0f);
ConVar<int> g_CV_WorldTweaksFile("WorldTweaksFile", 0);
#endif
