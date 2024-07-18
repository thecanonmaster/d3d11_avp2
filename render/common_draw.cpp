#include "pch.h"

#include "common_draw.h"
#include "common_stuff.h"
#include "d3d_device.h"
#include "d3d_viewparams.h"
#include "rendererconsolevars.h"
#include "aabb.h"
#include "tagnodes.h"
#include "globalmgr.h"

using namespace DirectX;

uint16 g_wCurFrameCode = 0;
uint32 g_dwCurObjectFrameCode = 0;
uint32 g_dwCurTextureFrameCode = 0;
SceneDesc* g_pSceneDesc = nullptr;
bool g_bHaveWorld = false;
bool g_bNewRenderContext = false;
ViewParams g_ViewParams;

static void d3d_IncrementFrameCode(RenderContext* pContext)
{
	if (pContext->m_wCurFrameCode == UINT16_MAX)
		pContext->m_wCurFrameCode = 1;

	pContext->m_wCurFrameCode++;
}

static void d3d_SetupPerspectiveMatrix(DirectX::XMFLOAT4X4* pMatrix, float fNearZ, float fFarZ)
{
	*pMatrix = g_ViewParams.m_mIdentity;

	pMatrix->m[2][2] = fFarZ / (fFarZ - fNearZ);
	pMatrix->m[2][3] = (-fFarZ * fNearZ) / (fFarZ - fNearZ);
	pMatrix->m[3][2] = 1.0f;
	pMatrix->m[3][3] = 0.0f;
}

static void d3d_SetupSkyStuff(SkyDef* pSkyDef, ViewParams* pParams)
{
	DirectX::XMVECTOR vPercents;

	if (g_pSceneDesc->m_nDrawMode == DRAWMODE_NORMAL)
	{
		MainWorld* pMainWorld = g_pSceneDesc->m_pRenderContext->m_pMainWorld;

		// TODO - check extents
		LTVector& vMin = pMainWorld->m_vExtentsMin;
		LTVector& vMax = pMainWorld->m_vExtentsMax;

		vPercents = DirectX::XMVectorSet(
			(pParams->m_vPos.x - vMin.x) / (vMax.x - vMin.x),
			(pParams->m_vPos.y - vMin.y) / (vMax.y - vMin.y),
			(pParams->m_vPos.z - vMin.z) / (vMax.z - vMin.z),
			0.0f
		);
	}
	else
	{
		vPercents = DirectX::XMVectorReplicate(0.5f);
	}

	DirectX::XMVECTOR vViewMin = DirectX::XMLoadFloat3(PLTVECTOR_TO_PXMFLOAT3(&pSkyDef->m_vViewMin));
	DirectX::XMVECTOR vViewDiff = DirectX::XMLoadFloat3(PLTVECTOR_TO_PXMFLOAT3(&pSkyDef->m_vViewMax)) - vViewMin;

	vViewDiff *= vPercents;

	DirectX::XMStoreFloat3(&pParams->m_vSkyViewPos, vViewMin + vViewDiff);
}

static void d3d_InitViewBox(ViewBoxDef* pDef, float fNearZ, float fFarZ, float fFovX, float fFovY)
{
	pDef->m_fNearZ = fNearZ;
	pDef->m_fFarZ = fFarZ;

	float fTempX = DirectX::XM_PIDIV2 - (fFovX * 0.5f);
	float fTempY = DirectX::XM_PIDIV2 - (fFovY * 0.5f);

	pDef->m_fWindowSize[0] = (float)(cos(fTempX) / sin(fTempX));
	pDef->m_fWindowSize[1] = (float)(cos(fTempY) / sin(fTempY));

	pDef->m_vCOP = DirectX::XMFLOAT3(0.0f, 0.0f, 1.0f);
}

void d3d_InitViewBox2(ViewBoxDef* pDef, float fNearZ, float fFarZ, ViewParams* pPrevParams, 
	float fScreenMinX, float fScreenMinY, float fScreenMaxX, float fScreenMaxY)
{
	float fMinPrevX = pPrevParams->m_ViewBox.m_vCOP.x - pPrevParams->m_ViewBox.m_fWindowSize[0];
	float fMinPrevY = -pPrevParams->m_ViewBox.m_vCOP.y - pPrevParams->m_ViewBox.m_fWindowSize[1];
	float fMaxPrevX = pPrevParams->m_ViewBox.m_vCOP.x + pPrevParams->m_ViewBox.m_fWindowSize[0];
	float fMaxPrevY = -pPrevParams->m_ViewBox.m_vCOP.y + pPrevParams->m_ViewBox.m_fWindowSize[1];

	float fMinX = (fScreenMinX - (float)pPrevParams->m_Rect.m_nLeft) / (float)(pPrevParams->m_Rect.m_nRight - pPrevParams->m_Rect.m_nLeft);
	float fMinY = (fScreenMinY - (float)pPrevParams->m_Rect.m_nTop) / (float)(pPrevParams->m_Rect.m_nBottom - pPrevParams->m_Rect.m_nTop);
	float fMaxX = (fScreenMaxX - (float)pPrevParams->m_Rect.m_nLeft) / (float)(pPrevParams->m_Rect.m_nRight - pPrevParams->m_Rect.m_nLeft);
	float fMaxY = (fScreenMaxY - (float)pPrevParams->m_Rect.m_nTop) / (float)(pPrevParams->m_Rect.m_nBottom - pPrevParams->m_Rect.m_nTop);

	fMinX = LTLERP(fMinPrevX, fMaxPrevX, fMinX);
	fMinY = -LTLERP(fMinPrevY, fMaxPrevY, fMinY);
	fMaxX = LTLERP(fMinPrevX, fMaxPrevX, fMaxX);
	fMaxY = -LTLERP(fMinPrevY, fMaxPrevY, fMaxY);

	pDef->m_fNearZ = fNearZ;
	pDef->m_fFarZ = fFarZ;
	pDef->m_vCOP.z = 1.0f;
	pDef->m_vCOP.x = (fMinX + fMaxX) * 0.5f;
	pDef->m_vCOP.y = (fMinY + fMaxY) * 0.5f;
	pDef->m_fWindowSize[0] = (fMaxX - fMinX) * 0.5f;
	pDef->m_fWindowSize[1] = -(fMaxY - fMinY) * 0.5f;
}

bool d3d_InitFrustum2(ViewParams* pParams, ViewBoxDef* pViewBox, float fScreenMinX, float fScreenMinY,
	float fScreenMaxX, float fScreenMaxY, const DirectX::XMFLOAT4X4* pMatrix, const DirectX::XMFLOAT3* pScale)
{
	pParams->m_mIdentity = g_mIdentity;
	pParams->m_mInvView = *pMatrix;
	DirectX::XMStoreFloat4x4(&pParams->m_mInvViewTransposed,
		DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(pMatrix)));

	memcpy(&pParams->m_ViewBox, pViewBox, sizeof(pParams->m_ViewBox));
	Matrix_GetTranslationLT(pMatrix, &pParams->m_vPos);

	pParams->m_fFarZ = pViewBox->m_fFarZ;

	if (pParams->m_fFarZ < MIN_FARZ)
		pParams->m_fFarZ = MIN_FARZ;

	if (pParams->m_fFarZ > MAX_FARZ)
		pParams->m_fFarZ = MAX_FARZ;

	pParams->m_fNearZ = pViewBox->m_fNearZ;

	pParams->m_Rect.m_nLeft = RoundFloatToInt(fScreenMinX);
	pParams->m_Rect.m_nTop = RoundFloatToInt(fScreenMinY);
	pParams->m_Rect.m_nRight = RoundFloatToInt(fScreenMaxX);
	pParams->m_Rect.m_nBottom = RoundFloatToInt(fScreenMaxY);

	DirectX::XMFLOAT4X4 mRotation = *pMatrix;
	Matrix_SetTranslationLT(&mRotation, 0.0f, 0.0f, 0.0f);
	Matrix_GetBasisVectorsLT(&mRotation, &pParams->m_vRight, &pParams->m_vUp, &pParams->m_vForward);
	Matrix_Transpose3x3LT(&mRotation);

	DirectX::XMFLOAT4X4 mBackTranslate = pParams->m_mIdentity;
	mBackTranslate.m[0][3] = -pParams->m_vPos.x;
	mBackTranslate.m[1][3] = -pParams->m_vPos.y;
	mBackTranslate.m[2][3] = -pParams->m_vPos.z;
	DirectX::XMMATRIX mTempWorld = DirectX::XMLoadFloat4x4(&mRotation) * DirectX::XMLoadFloat4x4(&mBackTranslate);

	DirectX::XMMATRIX mScale = DirectX::XMMatrixScaling(pScale->x, pScale->y, pScale->z);
	DirectX::XMMATRIX mScaleTimesTempWorld = mScale * mTempWorld;
	DirectX::XMStoreFloat4x4(&pParams->m_mView, mScaleTimesTempWorld);
	DirectX::XMStoreFloat4x4(&pParams->m_mViewTransposed, DirectX::XMMatrixTranspose(mScaleTimesTempWorld));

	DirectX::XMFLOAT4X4 mShear = pParams->m_mIdentity;
	mShear.m[0][2] = -pViewBox->m_vCOP.x / pViewBox->m_vCOP.z;
	mShear.m[1][2] = -pViewBox->m_vCOP.y / pViewBox->m_vCOP.z;

	float fFovXScale = pViewBox->m_vCOP.z / pViewBox->m_fWindowSize[0];
	float fFovYScale = pViewBox->m_vCOP.z / pViewBox->m_fWindowSize[1];

	DirectX::XMFLOAT4X4 mFOVScale = pParams->m_mIdentity;
	mFOVScale.m[0][0] = fFovXScale;
	mFOVScale.m[1][1] = fFovYScale;

	DirectX::XMFLOAT4X4 mProjectionTransform;
	d3d_SetupPerspectiveMatrix(&mProjectionTransform, pViewBox->m_fNearZ, pParams->m_fFarZ);

	// TODO - fine without it?
	//DirectX::XMFLOAT4X4 mSkyFarZProjectionTransform;
	//if (g_CV_SkyPortalHack.m_Val)
	//	d3d_SetupPerspectiveMatrix(&mSkyFarZProjectionTransform, 0.01f, g_CV_SkyFarZ.m_Val);

	pParams->m_fScreenWidth = fScreenMaxX - fScreenMinX;
	pParams->m_fScreenHeight = fScreenMaxY - fScreenMinY;

	DirectX::XMFLOAT4X4 mDevice = pParams->m_mIdentity;
	mDevice.m[0][0] = pParams->m_fScreenWidth * 0.5f - 0.0001f;
	mDevice.m[0][3] = fScreenMinX + pParams->m_fScreenWidth * 0.5f;
	mDevice.m[1][1] = -(pParams->m_fScreenHeight * 0.5f - 0.0001f);
	mDevice.m[1][3] = fScreenMinY + pParams->m_fScreenHeight * 0.5f;

	DirectX::XMMATRIX mDeviceTimesProjection = DirectX::XMLoadFloat4x4(&mDevice) * DirectX::XMLoadFloat4x4(&mProjectionTransform);
	DirectX::XMStoreFloat4x4(&pParams->m_mDeviceTimesProjection, mDeviceTimesProjection);
	DirectX::XMMATRIX mFullTransform = mDeviceTimesProjection * DirectX::XMLoadFloat4x4(&mFOVScale) * 
		DirectX::XMLoadFloat4x4(&mShear) * DirectX::XMLoadFloat4x4(&pParams->m_mView);
	DirectX::XMStoreFloat4x4(&pParams->m_mFullTransform, mFullTransform);

	DirectX::XMMATRIX mProjection = DirectX::XMLoadFloat4x4(&mProjectionTransform) * DirectX::XMLoadFloat4x4(&mFOVScale) *
		DirectX::XMLoadFloat4x4(&mShear);
	DirectX::XMStoreFloat4x4(&pParams->m_mProjection, mProjection);
	DirectX::XMStoreFloat4x4(&pParams->m_mProjectionTransposed, DirectX::XMMatrixTranspose(mProjection));

	/*if (g_CV_SkyPortalHack.m_Val)
	{
		DirectX::XMMATRIX mSkyFarZProjection = DirectX::XMLoadFloat4x4(&mSkyFarZProjectionTransform) * DirectX::XMLoadFloat4x4(&mFOVScale) *
			DirectX::XMLoadFloat4x4(&mShear);
		DirectX::XMStoreFloat4x4(&pParams->m_mSkyFarZProjectionTransposed, DirectX::XMMatrixTranspose(mSkyFarZProjection));
	}*/

	float fNearZ_X = (pParams->m_fNearZ * pViewBox->m_fWindowSize[0]) / pViewBox->m_vCOP.z;
	float fNearZ_Y = (pParams->m_fNearZ * pViewBox->m_fWindowSize[1]) / pViewBox->m_vCOP.z;
	float fFarZ_X = (pParams->m_fFarZ * pViewBox->m_fWindowSize[0]) / pViewBox->m_vCOP.z;
	float fFarZ_Y = (pParams->m_fFarZ * pViewBox->m_fWindowSize[1]) / pViewBox->m_vCOP.z;

	pParams->m_avViewPoints[0] = DirectX::XMFLOAT3(-fNearZ_X, fNearZ_Y, pParams->m_fNearZ);
	pParams->m_avViewPoints[1] = DirectX::XMFLOAT3(fNearZ_X, fNearZ_Y, pParams->m_fNearZ);
	pParams->m_avViewPoints[2] = DirectX::XMFLOAT3(-fNearZ_X, -fNearZ_Y, pParams->m_fNearZ);
	pParams->m_avViewPoints[3] = DirectX::XMFLOAT3(fNearZ_X, -fNearZ_Y, pParams->m_fNearZ);

	pParams->m_avViewPoints[4] = DirectX::XMFLOAT3(-fFarZ_X, fFarZ_Y, pParams->m_fFarZ);
	pParams->m_avViewPoints[5] = DirectX::XMFLOAT3(fFarZ_X, fFarZ_Y, pParams->m_fFarZ);
	pParams->m_avViewPoints[6] = DirectX::XMFLOAT3(-fFarZ_X, -fFarZ_Y, pParams->m_fFarZ);
	pParams->m_avViewPoints[7] = DirectX::XMFLOAT3(fFarZ_X, -fFarZ_Y, pParams->m_fFarZ);

	for (uint32 i = 0; i < 8; i++)
	{
		Matrix_VMul_InPlace_Transposed3x3LT(&pParams->m_mView, &pParams->m_avViewPoints[i]);

		DirectX::XMVECTOR vViewPoint = DirectX::XMLoadFloat3(&pParams->m_avViewPoints[i]) + DirectX::XMLoadFloat3(&pParams->m_vPos);
		DirectX::XMStoreFloat3(&pParams->m_avViewPoints[i], vViewPoint);
	}

	pParams->m_vViewAABBMin = pParams->m_avViewPoints[0];
	pParams->m_vViewAABBMax = pParams->m_avViewPoints[0];

	for (uint32 i = 1; i < 8; i++)
	{
		DirectX::XMVECTOR vTemp = DirectX::XMVectorMin(DirectX::XMLoadFloat3(&pParams->m_vViewAABBMin), DirectX::XMLoadFloat3(&pParams->m_avViewPoints[i]));
		DirectX::XMStoreFloat3(&pParams->m_vViewAABBMin, vTemp);

		vTemp = DirectX::XMVectorMax(DirectX::XMLoadFloat3(&pParams->m_vViewAABBMax), DirectX::XMLoadFloat3(&pParams->m_avViewPoints[i]));
		DirectX::XMStoreFloat3(&pParams->m_vViewAABBMax, vTemp);
	}

	float fLeftX = pViewBox->m_vCOP.x - pViewBox->m_fWindowSize[0];
	float fRightX = pViewBox->m_vCOP.x + pViewBox->m_fWindowSize[0];
	float fTopY = pViewBox->m_vCOP.y + pViewBox->m_fWindowSize[1];
	float fBottomY = pViewBox->m_vCOP.y - pViewBox->m_fWindowSize[1];
	float fNormalZ = pViewBox->m_vCOP.z;

	XMPLANE CSClipPlanes[NUM_CLIPPLANES];
	CSClipPlanes[CPLANE_NEAR_INDEX].m_vNormal = DirectX::XMFLOAT3(0.0f, 0.0f, 1.0f);
	CSClipPlanes[CPLANE_NEAR_INDEX].m_fDist = 1.0f;

	CSClipPlanes[CPLANE_FAR_INDEX].m_vNormal = DirectX::XMFLOAT3(0.0f, 0.0f, -1.0f);
	CSClipPlanes[CPLANE_FAR_INDEX].m_fDist = -pParams->m_fFarZ;

	DirectX::XMFLOAT3 vTemp = DirectX::XMFLOAT3(fNormalZ, 0.0f, -fLeftX);
	DirectX::XMVECTOR vTempNormal = DirectX::XMLoadFloat3(&vTemp);
	DirectX::XMStoreFloat3(&CSClipPlanes[CPLANE_LEFT_INDEX].m_vNormal, DirectX::XMVector3Normalize(vTempNormal));
	
	vTemp = DirectX::XMFLOAT3(-fNormalZ, 0.0f, fRightX);
	vTempNormal = DirectX::XMLoadFloat3(&vTemp);
	DirectX::XMStoreFloat3(&CSClipPlanes[CPLANE_RIGHT_INDEX].m_vNormal, DirectX::XMVector3Normalize(vTempNormal));

	vTemp = DirectX::XMFLOAT3(0.0f, -fNormalZ, fTopY);
	vTempNormal = DirectX::XMLoadFloat3(&vTemp);
	DirectX::XMStoreFloat3(&CSClipPlanes[CPLANE_TOP_INDEX].m_vNormal, DirectX::XMVector3Normalize(vTempNormal));

	vTemp = DirectX::XMFLOAT3(0.0f, fNormalZ, -fBottomY);
	vTempNormal = DirectX::XMLoadFloat3(&vTemp);
	DirectX::XMStoreFloat3(&CSClipPlanes[CPLANE_BOTTOM_INDEX].m_vNormal, DirectX::XMVector3Normalize(vTempNormal));

	CSClipPlanes[CPLANE_LEFT_INDEX].m_fDist = CSClipPlanes[CPLANE_RIGHT_INDEX].m_fDist = 0.0f;
	CSClipPlanes[CPLANE_TOP_INDEX].m_fDist = CSClipPlanes[CPLANE_BOTTOM_INDEX].m_fDist = 0.0f;

	DirectX::XMFLOAT4X4 mBackTransform = pParams->m_mView;
	Matrix_Transpose3x3LT(&mBackTransform);

	for (uint32 i = 0; i < NUM_CLIPPLANES; i++)
	{
		if (i != CPLANE_NEAR_INDEX && i != CPLANE_FAR_INDEX)
		{
			Matrix_VMul_3x3(&pParams->m_aClipPlanes[i].m_vNormal, &mBackTransform, &CSClipPlanes[i].m_vNormal);
			pParams->m_aClipPlanes[i].m_fDist = Vector_Dot(&pParams->m_aClipPlanes[i].m_vNormal, &pParams->m_vPos);
		}
	}

	DirectX::XMVECTOR vForwardVec = DirectX::XMVectorSet(mRotation.m[2][0], mRotation.m[2][1], mRotation.m[2][2], 0.0f);

	DirectX::XMVECTOR vZPlanePos = vForwardVec * pViewBox->m_fNearZ;
	vZPlanePos += DirectX::XMLoadFloat3(&pParams->m_vPos);

	Matrix_VMul_3x3(&pParams->m_aClipPlanes[CPLANE_NEAR_INDEX].m_vNormal, &mBackTransform, &CSClipPlanes[CPLANE_NEAR_INDEX].m_vNormal);

	DirectX::XMFLOAT3 vZPlanePosTemp;
	DirectX::XMStoreFloat3(&vZPlanePosTemp, vZPlanePos);
	pParams->m_aClipPlanes[CPLANE_NEAR_INDEX].m_fDist = Vector_Dot(&pParams->m_aClipPlanes[CPLANE_NEAR_INDEX].m_vNormal, &vZPlanePosTemp);

	vZPlanePos = vForwardVec * pParams->m_fFarZ;
	vZPlanePos += DirectX::XMLoadFloat3(&pParams->m_vPos);

	Matrix_VMul_3x3(&pParams->m_aClipPlanes[CPLANE_FAR_INDEX].m_vNormal, &mBackTransform, &CSClipPlanes[CPLANE_FAR_INDEX].m_vNormal);

	DirectX::XMStoreFloat3(&vZPlanePosTemp, vZPlanePos);
	pParams->m_aClipPlanes[CPLANE_FAR_INDEX].m_fDist = Vector_Dot(&pParams->m_aClipPlanes[CPLANE_FAR_INDEX].m_vNormal, &vZPlanePosTemp);

	for (uint32 dwPlaneLoop = 0; dwPlaneLoop < NUM_CLIPPLANES; ++dwPlaneLoop)
	{
		pParams->m_eAABBPlaneCorner[dwPlaneLoop] = (AABBCorner)GetAABBPlaneCorner(&pParams->m_aClipPlanes[dwPlaneLoop].m_vNormal);
	}

	pParams->m_mInvWorld = pParams->m_mIdentity;

	Matrix_SetBasisVectorsLT(&pParams->m_mWorldEnvMap, &pParams->m_vRight, &pParams->m_vUp, &pParams->m_vForward);

	d3d_SetupSkyStuff(&g_pSceneDesc->m_SkyDef, pParams);

	return true;
}

static bool d3d_InitFrustumOld(ViewParams* pParams, float fFovX, float fFovY, float fNearZ, float fFarZ, 
	float fScreenMinX, float fScreenMinY, float fScreenMaxX, float fScreenMaxY, const LTVector* pPos, const LTRotation* pRotation)
{
	DirectX::XMFLOAT4X4 mMatrix;

	Matrix_FromQuaternionLT(&mMatrix, PLTROTATION_TO_PXMFLOAT4(pRotation));
	Matrix_SetTranslationLT(&mMatrix, PLTVECTOR_TO_PXMFLOAT3(pPos));

	ViewBoxDef viewBox;
	d3d_InitViewBox(&viewBox, fNearZ, fFarZ, fFovX, fFovY);

	DirectX::XMFLOAT3 vScale = { 1.0f, 1.0f, 1.0f };
	return d3d_InitFrustum2(pParams, &viewBox, fScreenMinX, fScreenMinY, fScreenMaxX, fScreenMaxY, &mMatrix, &vScale);
}

static bool d3d_InitFrustum(ViewParams* pParams, float fFovX, float fFovY, float fNearZ, float fFarZ,
	float fScreenMinX, float fScreenMinY, float fScreenMaxX, float fScreenMaxY, const LTVector* pPos, const LTRotation* pRotation)
{
	DirectX::XMVECTOR rQuat = DirectX::XMVectorSet(pRotation->m_fQuat[0], pRotation->m_fQuat[1], pRotation->m_fQuat[2], pRotation->m_fQuat[3]);
	DirectX::XMMATRIX mMatrixTemp = DirectX::XMMatrixTranspose(DirectX::XMMatrixRotationQuaternion(rQuat));

	DirectX::XMFLOAT4X4 mMatrix;
	DirectX::XMStoreFloat4x4(&mMatrix, mMatrixTemp);
	Matrix_SetTranslationLT(&mMatrix, PLTVECTOR_TO_PXMFLOAT3(pPos));

	ViewBoxDef viewBox;
	d3d_InitViewBox(&viewBox, fNearZ, fFarZ, fFovX, fFovY);

	DirectX::XMFLOAT3 vScale = { 1.0f, 1.0f, 1.0f };
	return d3d_InitFrustum2(pParams, &viewBox, fScreenMinX, fScreenMinY, fScreenMaxX, fScreenMaxY, &mMatrix, &vScale);
}

bool d3d_InitFrame(ViewParams* pParams, SceneDesc* pDesc)
{
	d3d_SetFPState();

	d3d_ReadConsoleVariables();

	g_pSceneDesc = pDesc;

	RenderContext* pContext = pDesc->m_pRenderContext;

	if (pDesc->m_nDrawMode == DRAWMODE_NORMAL)
	{
		if (!pContext)
			return false;

		g_bHaveWorld = true;

		if (g_bNewRenderContext)
		{
			g_bNewRenderContext = false;

			g_GlobalMgr.GetLightMapMgr()->CreateLightmapPages(pContext->m_pMainWorld->m_LMAnims.m_pArray,
				pContext->m_pMainWorld->GetLMAnimCount());

			g_GlobalMgr.GetWorldModelMgr()->ReserveData();
#ifdef WORLD_TWEAKS_ENABLED
			g_GlobalMgr.GetWorldTweakMgr()->LoadWorldTweaks();
#endif

			if (g_CV_CacheObjects.m_Val)
				d3d_CacheObjects();
		}

		d3d_IncrementFrameCode(pContext);
		g_wCurFrameCode = pContext->m_wCurFrameCode;
	}
	else
	{
		g_bHaveWorld = false;
	}

	g_dwCurObjectFrameCode = g_pStruct->IncObjectFrameCode();
	g_dwCurTextureFrameCode = g_pStruct->IncCurTextureFrameCode();

	g_D3DDevice.SetDefaultRenderStates();

	g_D3DDevice.SetupViewport(pDesc->m_rcViewport.m_nLeft, pDesc->m_rcViewport.m_nRight, pDesc->m_rcViewport.m_nBottom, 
		pDesc->m_rcViewport.m_nTop, VIEWPORT_DEFAULT_MIN_Z, VIEWPORT_DEFAULT_MAX_Z);

	return d3d_InitFrustumOld(pParams, pDesc->m_fFovX, pDesc->m_fFovY, g_CV_NearZ.m_Val, pDesc->m_fFarZ,
		(float)pDesc->m_rcViewport.m_nLeft, (float)pDesc->m_rcViewport.m_nTop,
		(float)pDesc->m_rcViewport.m_nRight - 0.1f, (float)pDesc->m_rcViewport.m_nBottom - 0.1f,
		&pDesc->m_vPos, &pDesc->m_rRotation);
}
