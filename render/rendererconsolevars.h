#ifndef __RENDERERCONSOLEVARS_H__
#define __RENDERERCONSOLEVARS_H__

#include "d3d_convar.h"

enum SolidDrawingMode
{
	SDM_DoubleCopy = 0,
	SDM_SingleCopy = 1,
	SDM_Fast,
	SDM_DoubleMenuFastGame,
	SDM_SingleMenuFastGame,
	SDM_Max,
};

enum QueueObjectsMode
{
	QOM_VisNodes = 0,
	QOM_VisQuery = 1,
	QOM_VisQueryEx = 2,
	QOM_Max
};

extern ConVar<int> g_CV_D3D11_ModeScaling;
extern ConVar<int> g_CV_D3D11_Format;
extern ConVar<float> g_CV_D3D11_RefreshRate;
extern ConVar<float> g_CV_D3D11_GammaLevel;

extern ConVar<int> g_CV_LoseFocusNoShutdown;
extern ConVar<int> g_CV_NoLockOnFlip;

extern ConVar<int> g_CV_TripleBuffer;
extern ConVar<int> g_CV_VSync;

extern ConVar<int> g_CV_MaxFPS;
extern ConVar<int> g_CV_SetCurrentFPS;

extern ConVar<int> g_CV_SolidDrawingMode;
extern ConVar<int> g_CV_QueueObjectsMode;
extern ConVar<int> g_CV_VQE_AlwaysDrawSprites;

extern ConVar<int> g_CV_RenderDebug;
extern ConVar<int> g_CV_RenderDebugFile;

#if defined(DEBUG) || defined(_DEBUG)
extern ConVar<float> g_CV_AlmostBlackVoid;
#endif

extern ConVar<int> g_CV_MoviesKeepAspectRatio;

extern ConVar<int> g_CV_CacheTextures;

extern ConVar<int> g_CV_TraceConsole;

extern ConVar<int> g_CV_Saturate;

extern ConVar<float> g_CV_MipMapBias;
extern ConVar<int> g_CV_Bilinear;
extern ConVar<int> g_CV_Anisotropic;
extern ConVar<int> g_CV_FilterUnoptimized;
extern ConVar<int> g_CV_FilterOptimized;

extern ConVar<float> g_CV_ModelSunVariance;

extern ConVar<float> g_CV_ModelZoomScale;
extern ConVar<int> g_CV_ModelLODOffset;
extern ConVar<int> g_CV_ModelNoLODs;

extern ConVar<int> g_CV_MaxModelShadows;
extern ConVar<float> g_CV_ModelShadowLightScale;

extern ConVar<int> g_CV_TextureModels;

extern ConVar<float> g_CV_ExtraFOVXOffset;
extern ConVar<float> g_CV_ExtraFOVYOffset;

extern ConVar<float> g_CV_NearZ;
extern ConVar<float> g_CV_ReallyCloseNearZ;

extern ConVar<float> g_CV_RCSpriteFOVOffset;
extern ConVar<float> g_CV_RCCanvasFOVOffset;
extern ConVar<float> g_CV_RCParticlesFOVOffset;

extern ConVar<int> g_CV_Wireframe;

extern ConVar<int> g_CV_InvertHack;
extern ConVar<int> g_CV_LMFullBright;

extern ConVar<int> g_CV_UnlimitedVS;

extern ConVar<int> g_CV_DynamicLight;
extern ConVar<int> g_CV_BlackDynamicLight;

extern ConVar<float> g_CV_SkyScale;
extern ConVar<float> g_CV_SkyFarZ;

extern ConVar<int> g_CV_EnvMapPolyGrids;

extern ConVar<int> g_CV_DetailTextures;
extern ConVar<float> g_CV_DetailTextureScale;
extern ConVar<int> g_CV_DetailTextureAdd;

extern ConVar<int> g_CV_EnvMapWorld;
extern ConVar<int> g_CV_EnvMapEnable;
extern ConVar<float> g_CV_EnvScale;
extern ConVar<float> g_CV_EnvPanSpeed;

extern ConVar<int> g_CV_DrawGuns;
extern ConVar<int> g_CV_DrawSorted;

extern ConVar<int> g_CV_DrawModels;
extern ConVar<int> g_CV_DrawWorldModels;
extern ConVar<int> g_CV_DrawWorld;
extern ConVar<int> g_CV_DrawSky;
extern ConVar<int> g_CV_DrawLineSystems;
extern ConVar<int> g_CV_DrawSprites;
extern ConVar<int> g_CV_DrawParticles;
extern ConVar<int> g_CV_DrawCanvases;
extern ConVar<int> g_CV_DrawPolyGrids;

extern ConVar<int> g_CV_ShowRenderedObjectCounts;

extern ConVar<int> g_CV_DynamicLightSprites;
extern ConVar<float> g_CV_ModelVertexBufferTTL;
extern ConVar<float> g_CV_WorldModelVertexBufferTTL;
extern ConVar<float> g_CV_SpriteVertexBufferTTL;
extern ConVar<float> g_CV_ParticleSystemVertexBufferTTL;
extern ConVar<float> g_CV_CanvasVertexBufferTTL;
extern ConVar<float> g_CV_LineSystemVertexBufferTTL;
extern ConVar<float> g_CV_PolyGridVertexBufferTTL;

extern ConVar<int> g_CV_FogEnable;
extern ConVar<int> g_CV_FogColorR;
extern ConVar<int> g_CV_FogColorG;
extern ConVar<int> g_CV_FogColorB;
extern ConVar<float> g_CV_FogNearZ;
extern ConVar<float> g_CV_FogFarZ;

extern ConVar<float> g_CV_SkyFogNearZ;
extern ConVar<float> g_CV_SkyFogFarZ;

extern ConVar<int> g_CV_VFog;
extern ConVar<float> g_CV_VFogMinY;
extern ConVar<float> g_CV_VFogMaxY;
extern ConVar<float> g_CV_VFogMinYVal;
extern ConVar<float> g_CV_VFogMaxYVal;
extern ConVar<float> g_CV_VFogMax;
extern ConVar<float> g_CV_VFogDensity;

extern ConVar<int> g_CV_UseMipMapsInUse;

extern ConVar<int> g_CV_LockPVS;
extern ConVar<int> g_CV_FixTJunc;

extern ConVar<int> g_CV_CacheObjects;
#if defined(DEBUG) || defined(_DEBUG)
extern ConVar<int> g_CV_VisBoxTest;
#endif
extern ConVar<int> g_CV_FrustumTest;
extern ConVar<float> g_CV_VisQueryViewRadius;
extern ConVar<int> g_CV_SkyPortalHack;
extern ConVar<float> g_CV_RBExtraBBoxMargin;
extern ConVar<int> g_CV_RBExtraBBoxFrustumTest;
extern ConVar<int> g_CV_TwoPassSkyAlpha;
extern ConVar<float> g_CV_UntexturedTWMAlphaMod;

extern ConVar<int> g_CV_BatchOptimizedSurfaces;
extern ConVar<int> g_CV_BatchCanvases;
extern ConVar<int> g_CV_BatchSprites;

extern ConVar<float> g_CV_BloomThreshold;
extern ConVar<float> g_CV_BloomBlurSize;
extern ConVar<float> g_CV_BloomBlurBrightness;
extern ConVar<float> g_CV_BloomIntensity;
extern ConVar<float> g_CV_BloomBaseIntensity;
extern ConVar<float> g_CV_BloomSaturation;
extern ConVar<float> g_CV_BloomBaseSaturation;

#if defined(DEBUG) || defined(_DEBUG)
extern ConVar<float> g_CV_ShowPosStats;
extern ConVar<int> g_CV_WorldTweaksFile;
#endif

#endif