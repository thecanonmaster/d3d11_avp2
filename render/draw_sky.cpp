#include "pch.h"

#include "draw_sky.h"
#include "rendererconsolevars.h"
#include "common_draw.h"
#include "renderstatemgr.h"
#include "d3d_draw.h"
#include "draw_sprite.h"
#include "draw_polygrid.h"
#include "draw_worldmodel.h"
#include "draw_model.h"
#include "setup_model.h"
#include "rendermodemgr.h"
#include "rendertargetmgr.h"
#include "rendertarget.h"
#include "d3d_postprocess.h"
#include "d3d_device.h"
#include "globalmgr.h"
#include "d3d_shader_worldmodel.h"

static void d3d_DrawSkyObjects(ViewParams* pSkyParams)
{
	//g_RenderStateMgr.SetStencilState(STENCIL_STATE_NoZ);

	for (uint32 i = 0; i < (uint32)g_pSceneDesc->m_nSkyObjects; i++)
	{
		LTObject* pSkyObject = g_pSceneDesc->m_pSkyObjects[i];

		if (pSkyObject->m_dwFlags & FLAG_VISIBLE)
		{
			switch (pSkyObject->m_nObjectType)
			{
				/*case OT_MODEL:
				{
					if (g_CV_DrawModels.m_Val)
					{
						QueuedModelInfo info = { };
						d3d_QueueModel(pSkyParams, pSkyObject, &info);

						if (d3d_IsTranslucentModel(pSkyObject))
						{
							d3d_SetTranslucentObjectStates(false);
							d3d_DrawModelPieceList(BLEND_STATE_Alpha, MODE_SKY_OBJECT);
						}
						else
						{
							d3d_UnsetTranslucentObjectStates(false);
							d3d_DrawModelPieceList(BLEND_STATE_Default, MODE_SKY_OBJECT);
						}
					}

					break;
				}*/

				case OT_WORLDMODEL: 
				{
					if (g_CV_DrawWorldModels.m_Val)
					{
						if (d3d_IsTranslucentWorldModel(pSkyObject))
						{
							d3d_SetTranslucentObjectStates(false);
							d3d_DrawWorldModel(pSkyParams, pSkyObject, BLEND_STATE_Alpha, MODE_SKY_OBJECT);
						}
						else
						{
							d3d_UnsetTranslucentObjectStates(false);
							d3d_DrawWorldModel(pSkyParams, pSkyObject, BLEND_STATE_Default, MODE_SKY_OBJECT);
						}
					}

					break;
				}
				
				case OT_POLYGRID:
				{
					if (g_CV_DrawPolyGrids.m_Val)
					{ 
						if (d3d_IsTranslucentPolyGrid(pSkyObject))
							d3d_SetTranslucentObjectStates(false);
						else
							d3d_UnsetTranslucentObjectStates(false);

						d3d_DrawPolyGrid(pSkyParams, pSkyObject);
					}

					break;
				}

				case OT_SPRITE:
				{
					if (g_CV_DrawSprites.m_Val)
					{
						d3d_SetTranslucentObjectStates(false);
						d3d_DrawSprite(pSkyParams, pSkyObject);
					}

					break;
				}
			}
		}
	}

	g_GlobalMgr.GetSpriteBatchMgr()->RenderBatch();
	g_RenderStateMgr.SetStencilState(STENCIL_STATE_Default);
}

/*void d3d_ExtendSkyBounds(ViewParams* pParams, float& fMinX, float& fMinY, float& fMaxX, float& fMaxY)
{
	uint32 dwLength = g_GlobalMgr.GetWorldModelMgr()->m_aSkyPortalVertexData.size() / 3;
	for (uint32 i = 0; i < dwLength; i++)
	{
		InternalSkyPortalVertex* pPoly = &g_GlobalMgr.GetWorldModelMgr()->m_aSkyPortalVertexData[i * 3];

		float fLocalMinX = FLT_MAX;
		float fLocalMinY = FLT_MAX;
		float fLocalMaxX = -FLT_MAX;
		float fLocalMaxY = -FLT_MAX;

		DirectX::XMFLOAT3 vPrevVert = pPoly[2].vPosition;
		float fPrevNearDist = Plane_DistTo(&pParams->m_aClipPlanes[CPLANE_NEAR_INDEX], &vPrevVert);
		bool bPrevClip = fPrevNearDist > 0.0f;

		for (uint32 j = 0; j < 3; j++)
		{
			DirectX::XMFLOAT3 vCurrVert = pPoly[j].vPosition;
			float fCurrNearDist = Plane_DistTo(&pParams->m_aClipPlanes[CPLANE_NEAR_INDEX], &vCurrVert);
			bool bCurClip = fCurrNearDist > 0.0f;

			if (bCurClip != bPrevClip)
			{
				float fInterpolant = (fCurrNearDist != fPrevNearDist) ? -fPrevNearDist /
					(fCurrNearDist - fPrevNearDist) : 0.0f;

				DirectX::XMStoreFloat3(&vCurrVert, DirectX::XMVectorLerp(DirectX::XMLoadFloat3(&vPrevVert),
					DirectX::XMLoadFloat3(&vCurrVert), fInterpolant));
			}

			Matrix_VMul_InPlace_H(&pParams->m_mFullTransform, &vCurrVert);

			fLocalMinX = LTMIN(fLocalMinX, vCurrVert.x);
			fLocalMinY = LTMIN(fLocalMinY, vCurrVert.y);
			fLocalMaxX = LTMAX(fLocalMaxX, vCurrVert.x);
			fLocalMaxY = LTMAX(fLocalMaxY, vCurrVert.y);

			vPrevVert = vCurrVert;
			fPrevNearDist = fCurrNearDist;
			bPrevClip = bCurClip;
		}

		if (fLocalMaxX < (float)pParams->m_Rect.m_nLeft ||
			fLocalMaxY < (float)pParams->m_Rect.m_nTop ||
			fLocalMinX > (float)pParams->m_Rect.m_nRight ||
			fLocalMinY > (float)pParams->m_Rect.m_nBottom)
		{
			continue;
		}

		fLocalMinX = LTMAX((float)pParams->m_Rect.m_nLeft, fLocalMinX);
		fLocalMinY = LTMAX((float)pParams->m_Rect.m_nTop, fLocalMinY);
		fLocalMaxX = LTMIN((float)pParams->m_Rect.m_nRight, fLocalMaxX);
		fLocalMaxY = LTMIN((float)pParams->m_Rect.m_nBottom, fLocalMaxY);

		fMinX = LTMIN(fMinX, fLocalMinX);
		fMinY = LTMIN(fMinY, fLocalMinY);
		fMaxX = LTMAX(fMaxX, fLocalMaxX);
		fMaxY = LTMAX(fMaxY, fLocalMaxY);
	}
}*/

static void d3d_SetSkyBBoxBuffersAndTopology()
{
	g_RenderShaderMgr.SetVertexResource(VRS_Main, g_GlobalMgr.GetWorldModelMgr()->m_pSkyPortalBBox_VB, 
		sizeof(InternalSkyPortalVertex), 0);
	g_RenderShaderMgr.SetIndexBuffer16(g_GlobalMgr.GetWorldModelMgr()->m_pSkyPortalBBox_IB, 0);
	g_RenderShaderMgr.SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

static void d3d_GetSkyBBoxFinalTransform(ViewParams* pParams, DirectX::XMFLOAT4X4* pTransform,
	DirectX::XMFLOAT4X4* pWorldModelTransform)
{
	DirectX::XMMATRIX mTransform;

	mTransform = DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(pWorldModelTransform)) *
		DirectX::XMLoadFloat4x4(&pParams->m_mViewTransposed) *
		DirectX::XMLoadFloat4x4(&pParams->m_mProjectionTransposed);

	DirectX::XMStoreFloat4x4(pTransform, mTransform);
}

static void d3d_DrawSkyBBox(ViewParams* pParams, DirectX::XMFLOAT4X4* pTransform)
{
	if (!g_GlobalMgr.GetWorldModelMgr()->GetSkyPortalVertexCount())
		return;

	g_RenderStateMgr.SavePrimaryStates();

	g_RenderStateMgr.SetStencilState(STENCIL_STATE_NoZ);
	g_RenderStateMgr.SetBlendState(BLEND_STATE_Default);
	g_RenderStateMgr.SetRasterState(g_CV_Wireframe.m_Val ? RASTER_STATE_Wireframe : RASTER_STATE_CullbackNoDepthClip);

	XMFloat4x4Trinity sTransforms;
	sTransforms.m_mWorld = *pTransform;

	d3d_GetSkyBBoxFinalTransform(pParams, &sTransforms.m_mWorldViewProj, &sTransforms.m_mWorld);

	d3d_SetSkyBBoxBuffersAndTopology();

	CRenderShader_SkyPortal* pRenderShader = g_RenderShaderMgr.GetRenderShader<CRenderShader_SkyPortal>();

	if (!pRenderShader->SetPerObjectParams(
		g_RenderTargetMgr.GetRenderTarget(RENDER_TARGET_Bloom1)->GetShaderResourceView(),
		&sTransforms, g_D3DDevice.GetModeInfo()->m_dwWidth, g_D3DDevice.GetModeInfo()->m_dwHeight))
	{
		return;
	}

	pRenderShader->Render(SKY_PORTAL_BBOX_INDICES);

	g_RenderStateMgr.RestorePrimaryStates();
}

void d3d_DrawSkyExtents(ViewParams* pParams, float fMinX, float fMinY, float fMaxX, float fMaxY)
{
	if (!g_CV_DrawSky.m_Val || !g_pSceneDesc->m_nSkyObjects)
		return;

	// TODO - original uses g_pSceneDesc->m_fFarZ?
	ViewBoxDef viewBox;
	d3d_InitViewBox2(&viewBox, 0.01f, g_CV_SkyFarZ.m_Val, pParams, fMinX, fMinY, fMaxX, fMaxY);

	DirectX::XMFLOAT4X4 mNewInvView = pParams->m_mInvView;
	Matrix_SetTranslationLT(&mNewInvView, &pParams->m_vSkyViewPos);

	ViewParams skyParams;
	DirectX::XMFLOAT3 vSkyScale = { g_CV_SkyScale.m_Val, g_CV_SkyScale.m_Val, g_CV_SkyScale.m_Val };
	d3d_InitFrustum2(&skyParams, &viewBox, fMinX, fMinY, fMaxX, fMaxY, &mNewInvView, &vSkyScale);

	//g_D3DDevice.SetupTempViewport(skyParams.m_Rect.m_nLeft, skyParams.m_Rect.m_nRight, skyParams.m_Rect.m_nBottom,
	//	skyParams.m_Rect.m_nTop, VIEWPORT_DEFAULT_MIN_Z, VIEWPORT_DEFAULT_MAX_Z);

	if (g_CV_SkyPortalHack.m_Val)
	{
		g_RenderTargetMgr.RT_InstallOnDevice(RENDER_TARGET_Bloom1);
		d3d_DrawSkyObjects(&skyParams);
		g_RenderTargetMgr.RT_InstallOnDevice(RENDER_TARGET_Main1);
		d3d_DrawSkyBBox(pParams, &g_mIdentity);
	}
	else
	{
		d3d_DrawSkyObjects(&skyParams);
	}
}
