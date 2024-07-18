#include "pch.h"

#include "d3d_draw.h"
#include "d3d_init.h"
#include "d3d_device.h"
#include "rendertargetmgr.h"
#include "rendertarget.h"
#include "common_stuff.h"
#include "rendererconsolevars.h"
#include "framelimiter.h"
#include "common_draw.h"
#include "renderstatemgr.h"
#include "d3d_postprocess.h"
#include "draw_objects.h"
#include "globalmgr.h"
#include "d3d_shader_base.h"
#include "draw_objects.h"

int g_nLastDrawMode = DRAWMODE_OBJECTLIST;
float g_fLastClientTime = 0.0f;
float g_fLastFrameTime = 0.0f;

void d3d_Clear(LTRect* pRect, uint32 dwFlags, LTVector* pClearColor)
{	
	/*if (g_D3DDevice.GetDevice() == nullptr)
		return;*/

	D3DModeInfo* pModeInfo = g_D3DDevice.GetModeInfo();
	
	if (pRect != nullptr && (pRect->m_nLeft != 0 || pRect->m_nTop != 0 ||
		pRect->m_nRight != (int)pModeInfo->m_dwWidth || pRect->m_nBottom != (int)pModeInfo->m_dwHeight))
	{
		NotImplementedMessage("d3d_Clear - partial");
		return;
	}

	//float afClearColor[4] = { 0.25f, 0.25f, 0.25f, 0.25f };
	//float afZeroColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	float afClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	if (pClearColor != nullptr)
	{
		afClearColor[0] = pClearColor->x * MATH_ONE_OVER_255;
		afClearColor[1] = pClearColor->y * MATH_ONE_OVER_255;
		afClearColor[2] = pClearColor->z * MATH_ONE_OVER_255;
	}

#if defined(DEBUG) || defined(_DEBUG)
	if (g_CV_AlmostBlackVoid.m_Val && afClearColor[0] == 0.0f &&
		afClearColor[1] == 0.0f && afClearColor[2] == 0.0f)
	{
		afClearColor[0] = g_CV_AlmostBlackVoid.m_Val;
		afClearColor[1] = g_CV_AlmostBlackVoid.m_Val;
		afClearColor[2] = g_CV_AlmostBlackVoid.m_Val;
	}
#endif

	if (dwFlags & CLEARSCREEN_SCREEN)
	{
		g_RenderTargetMgr.GetRenderTarget(RENDER_TARGET_Main1)->ClearRenderTargetView(afClearColor);
		g_RenderTargetMgr.GetRenderTarget(RENDER_TARGET_Bloom1)->ClearRenderTargetView(afClearColor);
		//g_RenderTargetMgr.GetRenderTarget(RENDER_TARGET_Default)->ClearRenderTargetView(afClearColor);
	}

	if (dwFlags & CLEARSCREEN_RENDER)
	{
		g_RenderTargetMgr.GetRenderTarget(RENDER_TARGET_Main1)->ClearDepthStencilView(D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
		//g_RenderTargetMgr.GetRenderTarget(RENDER_TARGET_Default)->ClearDepthStencilView(D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	}
}

void d3d_SetTranslucentObjectStates(bool bAdditive)
{
	g_RenderStateMgr.SetStencilState(STENCIL_STATE_NoZWrite);

	if (bAdditive)
		g_RenderStateMgr.SetBlendState(BLEND_STATE_Add);
	else
		g_RenderStateMgr.SetBlendState(BLEND_STATE_Alpha);
}

void d3d_UnsetTranslucentObjectStates(bool bChangeZ)
{
	g_RenderStateMgr.SetBlendState(BLEND_STATE_Default);

	if (bChangeZ)
		g_RenderStateMgr.SetStencilState(STENCIL_STATE_Default);
}

static void d3d_ProcessObjectList(LTObject** ppObjectList, int nObjectListSize)
{
	for (int i = 0; i < nObjectListSize; i++)
	{
		LTObject* pObject = ppObjectList[i];

		if (pObject)
		{		
			if (pObject->m_nObjectType >= OT_NORMAL && pObject->m_nObjectType < NUM_OBJECT_TYPES)
			{
				// TODO - ignore zero-alpha objects too? low on vertices zero-alpha + not ADDITIVE/MULTIPLY flagged?
				if (g_ObjectHandlers[pObject->m_nObjectType].m_pProcessObjectFn && (pObject->m_dwFlags & FLAG_VISIBLE))
					g_ObjectHandlers[pObject->m_nObjectType].m_pProcessObjectFn(pObject);
			}
			else
			{
				AddDebugMessage(0, "Unknown object type %d", pObject->m_nObjectType);
			}
		}
	}
}

static void d3d_DrawScreenFX(SceneDesc* pDesc, LTRect* pRect)
{
	if (g_CV_BloomThreshold.m_Val)
	{
		CRenderTarget* pRT_Main2 = g_RenderTargetMgr.GetRenderTarget(RENDER_TARGET_Main2);
		CRenderTarget* pRT_Bloom1 = g_RenderTargetMgr.GetRenderTarget(RENDER_TARGET_Bloom1);
		CRenderTarget* pRT_Bloom2 = g_RenderTargetMgr.GetRenderTarget(RENDER_TARGET_Bloom2);
		RenderTargetParams* pParams = pRT_Bloom1->GetInitParams();

		g_RenderTargetMgr.RT_InstallOnDevice(RENDER_TARGET_Main2);
		d3d_PostProcess_ScreenFX(g_RenderTargetMgr.GetRenderTarget(RENDER_TARGET_Main1)->GetShaderResourceView());
		
		g_RenderTargetMgr.RT_InstallOnDevice(RENDER_TARGET_Bloom1);
		d3d_PostProcess_BloomExtract(pRT_Main2->GetShaderResourceView());
		g_RenderShaderMgr.ClearShaderResourcesPS(SRS_PS_Primary, 1);

		g_RenderTargetMgr.RT_InstallOnDevice(RENDER_TARGET_Bloom2);	
		d3d_PostProcess_BloomBlur(pRT_Bloom1->GetShaderResourceView(), false, pParams->m_dwWidth, pParams->m_dwHeight);
		g_RenderShaderMgr.ClearShaderResourcesPS(SRS_PS_Primary, 1);

		g_RenderTargetMgr.RT_InstallOnDevice(RENDER_TARGET_Bloom1);
		d3d_PostProcess_BloomBlur(pRT_Bloom2->GetShaderResourceView(), true, pParams->m_dwWidth, pParams->m_dwHeight);
		g_RenderShaderMgr.ClearShaderResourcesPS(SRS_PS_Primary, 1);

		g_RenderTargetMgr.RT_InstallOnDevice(RENDER_TARGET_Main1);
		d3d_PostProcess_BloomCombine(pRT_Main2->GetShaderResourceView(), pRT_Bloom1->GetShaderResourceView());

		g_RenderShaderMgr.ClearShaderResourcesPS(SRS_PS_Other, 1);
	}
	else
	{
		g_RenderTargetMgr.RT_InstallOnDevice(RENDER_TARGET_Main2);
		d3d_PostProcess_ScreenFX(g_RenderTargetMgr.GetRenderTarget(RENDER_TARGET_Main1)->GetShaderResourceView());
	}

	g_RenderShaderMgr.ClearShaderResourcesPS(SRS_PS_Primary, 1);
}

static void d3d_FullDrawScene(ViewParams* pParams, SceneDesc* pDesc)
{
	d3d_InitObjectQueues();

	if (pDesc->m_nDrawMode == DRAWMODE_OBJECTLIST)
	{
		d3d_GetVisibleSet()->ClearSet();
		d3d_ProcessObjectList(pDesc->m_pObjectList, pDesc->m_nObjectListSize);
	}
	else
	{
		d3d_TagVisibleLeaves(pParams);
	}

	d3d_FlushObjectQueues(pParams);

	d3d_DrawScreenFX(pDesc, &pParams->m_Rect);
}

static void d3d_SetPerFrameParams_3D(SceneDesc* pDesc)
{	
	float fEnvScale = g_CV_EnvScale.m_Val;
	if (fabs(fEnvScale) > 0.001f)
		fEnvScale = -0.5f / fEnvScale;

	CRenderShader_Base::VSPerFrameParams_3D initStructVS;
	initStructVS.m_dwScreenWidth = g_D3DDevice.GetModeInfo()->m_dwWidth;
	initStructVS.m_dwScreenHeight = g_D3DDevice.GetModeInfo()->m_dwHeight;

	initStructVS.m_fFogStart = g_CV_FogNearZ.m_Val;
	initStructVS.m_fFogEnd = g_CV_FogFarZ.m_Val;

	initStructVS.m_pView = &g_ViewParams.m_mView;
	initStructVS.m_pInvView = &g_ViewParams.m_mInvView;

	initStructVS.m_pCameraPos = &g_ViewParams.m_vPos;

	initStructVS.m_fEnvScale = fEnvScale;
	initStructVS.m_fEnvPanSpeed = g_CV_EnvPanSpeed.m_Val;

	initStructVS.m_bVFogEnabled = g_CV_VFog.m_Val;
	initStructVS.m_fVFogDensity = g_CV_VFogDensity.m_Val;
	initStructVS.m_fVFogMinY = g_CV_VFogMinY.m_Val;
	initStructVS.m_fVFogMaxY = g_CV_VFogMaxY.m_Val;
	initStructVS.m_fVFogMinYVal = LTCLAMP(g_CV_VFogMinYVal.m_Val, 0.0f, 1.0f);
	initStructVS.m_fVFogMaxYVal = LTCLAMP(g_CV_VFogMaxYVal.m_Val, 0.0f, 1.0f);
	initStructVS.m_fVFogMax = LTCLAMP(g_CV_VFogMax.m_Val * MATH_ONE_OVER_255, 0.0f, 1.0f);

	initStructVS.m_fSkyFogNear = g_CV_SkyFogNearZ.m_Val;
	initStructVS.m_fSkyFogFar = g_CV_SkyFogFarZ.m_Val;

#ifndef MODEL_PIXEL_LIGHTING
	initStructVS.m_pDirLightDir = PLTVECTOR_TO_PXMFLOAT3(&g_pStruct->m_vGlobalLightDir);
#endif

	if (g_bHaveWorld && pDesc->m_pRenderContext != nullptr)
	{
		GlobalPanInfo& skyShadowInfo = g_pStruct->m_GlobalPanInfo[GLOBALPAN_SKYSHADOW];

		if (skyShadowInfo.m_pTexture != nullptr)
		{
			initStructVS.m_fCurSkyXOffset = skyShadowInfo.m_fCurSkyXOffset;
			initStructVS.m_fCurSkyZOffset = skyShadowInfo.m_fCurSkyZOffset;

			RTexture* pRenderTexture = (RTexture*)skyShadowInfo.m_pTexture->m_pRenderData;

			initStructVS.m_fPanSkyScaleX =
				1.0f / skyShadowInfo.m_fPanSkyScaleX * (1.0f / pRenderTexture->m_dwScaledWidth);
			initStructVS.m_fPanSkyScaleZ =
				1.0f / skyShadowInfo.m_fPanSkyScaleZ * (1.0f / pRenderTexture->m_dwScaledHeight);
		}

		initStructVS.m_pLMAnims = pDesc->m_pRenderContext->m_pMainWorld->m_LMAnims.m_pArray;
		initStructVS.m_dwLMAnims = pDesc->m_pRenderContext->m_pMainWorld->GetLMAnimCount();
	}

	g_RenderShaderMgr.SetPerFrameParamsVS_3D(&initStructVS);

	CRenderShader_Base::PSPerFrameParams_3D initStructPS;

	DirectX::XMFLOAT3 vFogColor =
	{ 
		(float)g_CV_FogColorR.m_Val * MATH_ONE_OVER_255,
		(float)g_CV_FogColorG.m_Val * MATH_ONE_OVER_255,
		(float)g_CV_FogColorB.m_Val * MATH_ONE_OVER_255
	};

	constexpr float c_fMultiPassScale = 255.0f * 0.001953125f;
	DirectX::XMFLOAT3 vFogColorMP =
	{
		sqrtf(vFogColor.x * c_fMultiPassScale),
		sqrtf(vFogColor.y * c_fMultiPassScale),
		sqrtf(vFogColor.z * c_fMultiPassScale),
	};

	initStructPS.m_pFogColor = &vFogColor;
	initStructPS.m_pFogColorMP = &vFogColorMP;

	SharedTexture* pGlobalEnvMapTexture = g_pStruct->m_pEnvMapTexture;
	if (pGlobalEnvMapTexture != nullptr)
		initStructPS.m_pGlobalEnvMapSRV = ((RTexture*)(pGlobalEnvMapTexture->m_pRenderData))->m_pResourceView;

	SharedTexture* pGlobalPanTexture = g_pStruct->m_GlobalPanInfo->m_pTexture;
	if (pGlobalPanTexture != nullptr)
		initStructPS.m_pGlobalPanSRV = ((RTexture*)(pGlobalPanTexture->m_pRenderData))->m_pResourceView;

	initStructPS.m_pLightAdd = PLTVECTOR_TO_PXMFLOAT3(&pDesc->m_vGlobalLightAdd);
	initStructPS.m_pLightScale = PLTVECTOR_TO_PXMFLOAT3(&pDesc->m_vGlobalLightScale);
	initStructPS.m_bInvertHack = g_CV_InvertHack.m_Val;

	initStructPS.m_pGlobalVertexTint = PLTVECTOR_TO_PXMFLOAT3(&pDesc->m_vGlobalVertexTint);

#ifdef MODEL_PIXEL_LIGHTING
	initStructPS.m_pDirLightDir = PLTVECTOR_TO_PXMFLOAT3(&g_pStruct->m_vGlobalLightDir);
#endif

	initStructPS.m_pLMAnims = initStructVS.m_pLMAnims;
	initStructPS.m_dwLMAnims = initStructVS.m_dwLMAnims;

	if (g_bHaveWorld && pDesc->m_pRenderContext != nullptr)
		initStructPS.m_pLMPages = g_GlobalMgr.GetLightMapMgr()->m_pPages;

	g_RenderShaderMgr.SetPerFrameParamsPS_3D(&initStructPS);
}

void d3d_UploadDynamicLights()
{
	g_RenderShaderMgr.SetDynamicLightsVPS_3D(d3d_GetVisibleSet()->GetLights()->GetObjects(), g_aModelShadowLight);
}

int d3d_RenderScene(SceneDesc* pDesc)
{
	if (/*g_D3DDevice.GetDevice() == nullptr || */ pDesc == nullptr || !g_D3DDevice.IsIn3D())
		return 0;

	//Timer_MeasurementStartMM();

	g_nLastDrawMode = pDesc->m_nDrawMode;

	g_fLastClientTime += pDesc->m_fFrameTime;
	g_fLastFrameTime = pDesc->m_fFrameTime;

	if (g_bInOptimized2D)
	{
		AddDebugMessage(0, "Error: tried to render 3D while in optimized 2D mode");
		return 0;
	}

	if (g_CV_MaxFPS.m_Val)
	{
		g_D3DDevice.GetFrameLimiter()->SetTargetFrameRate(g_CV_MaxFPS.m_Val);
		g_D3DDevice.GetFrameLimiter()->Update();

		//g_D3DDevice.GetFrameLimiter()->DebugOutFramerate();
	}

	if (g_CV_SetCurrentFPS.m_Val)
	{
		char szCurrFPS[64];
		sprintf_s(szCurrFPS, "CurrentFPS %d", g_D3DDevice.GetFrameLimiter()->GetLastFrameRate());
		g_pStruct->RunConsoleString(szCurrFPS);
	}

#if defined(DEBUG) || defined(_DEBUG)
	static float s_fPrevStatsTime = 0.0f;
	if (g_CV_ShowPosStats.m_Val > 0.0f && g_fLastClientTime - s_fPrevStatsTime > g_CV_ShowPosStats.m_Val)
	{
		s_fPrevStatsTime = g_fLastClientTime;
		AddDebugMessage(0, "[%f]  %f %f %f", g_fLastClientTime, pDesc->m_vPos.x, pDesc->m_vPos.y, pDesc->m_vPos.z);
	}
#endif

	if (d3d_InitFrame(&g_ViewParams, pDesc))
	{
		d3d_SetPerFrameParams_3D(pDesc);

		d3d_FullDrawScene(&g_ViewParams, pDesc);

		g_GlobalMgr.Update();

		g_RenderStateMgr.SetRasterState(RASTER_STATE_Cullback);
	}


	//AddDebugMessage(0, "FrameTime = %f", Timer_MeasurementEndMM());

	return 0;
}
