#include "pch.h"

#include "setup_model.h"
#include "common_draw.h"
#include "rendererconsolevars.h"
#include "common_stuff.h"
#include "3d_ops.h"
#include "draw_model.h"
#include "draw_light.h"
#include "tagnodes.h"
#include "full_intersectline.h"
#include "globalmgr.h"

using namespace DirectX;

ModelSetup g_ModelSetup;
uint32 g_aCurrPrimaryNode[MAX_MODEL_TRANSFORMS];

struct StaticLightCallbackData
{
	ModelInstance*		m_pInstance;
	QueuedModelInfo*	m_pInfo;
};

bool QueuedPieceInfo::operator==(const QueuedPieceInfo& other) const
{
	return !memcmp(this, &other, sizeof(QueuedPieceInfo));
}

bool QueuedPieceInfo::operator<(const QueuedPieceInfo& other) const
{
	if (m_bReallyClose != other.m_bReallyClose)
		return other.m_bReallyClose;

	if (m_pTexture != other.m_pTexture)
		return m_pTexture < other.m_pTexture;

	if (m_pModel != other.m_pModel)
		return m_pModel < other.m_pModel;

	if (m_dwPieceIndex != other.m_dwPieceIndex)
		return m_dwPieceIndex < other.m_dwPieceIndex;

	if (m_dwLODIndex != other.m_dwLODIndex)
		return m_dwLODIndex < other.m_dwLODIndex;

	return false;
}

bool QueuedPieceInfo::LessThan_Translucent(const QueuedPieceInfo& left, const QueuedPieceInfo& right)
{
	if (left.m_bReallyClose != right.m_bReallyClose)
		return right.m_bReallyClose;

	if (left.m_pModel != right.m_pModel)
		return left.m_pModel < right.m_pModel;
	
	if (left.m_fDistanceSqr != right.m_fDistanceSqr)
		return left.m_fDistanceSqr > right.m_fDistanceSqr;

	return false;
}

void ModelSetup::CallModelInstanceHook(ModelInstance* pInstance, ModelInstanceHookData* pInstanceHookData)
{
	if (pInstance->m_pInstanceHookFn == nullptr)
		return;

	pInstance->m_pInstanceHookFn(&pInstance->m_Base, pInstanceHookData, 0);
	NotImplementedMessage("ModelSetup::CallModelInstanceHook");
}

void ModelSetup::CallModelHook(ModelInstance* pInstance, InternalModelHookData* pHookData)
{
	if (g_pSceneDesc->m_ModelHookFn == nullptr)
		return;

	ModelHookData tempHookData;

	tempHookData.m_dwObjectFlags = pInstance->m_Base.m_dwFlags;
	tempHookData.m_dwHookFlags = MHF_USETEXTURE;
	tempHookData.m_pObject = &pInstance->m_Base;

	pHookData->m_vLightAdd = g_pSceneDesc->m_vModelLightAdd;
	tempHookData.m_pLightAdd = &pHookData->m_vLightAdd;
	tempHookData.m_pObjectColor = &pHookData->m_vObjectColor;

	tempHookData.m_pObjectColor->x = (float)pInstance->m_Base.m_nColorR;
	tempHookData.m_pObjectColor->y = (float)pInstance->m_Base.m_nColorG;
	tempHookData.m_pObjectColor->z = (float)pInstance->m_Base.m_nColorB;

	if (g_pSceneDesc->m_ModelHookFn != nullptr)
		g_pSceneDesc->m_ModelHookFn(&tempHookData, g_pSceneDesc->m_pModelHookUser);

	pHookData->m_dwHookFlags = tempHookData.m_dwHookFlags;
	pHookData->m_dwObjectFlags = tempHookData.m_dwObjectFlags;
}

void ModelSetup::CacheModelInstanceTransforms(ModelInstance* pInstance)
{
	// TODO - child models? node controller?
	ModelInstance_VTable* pVTable = (ModelInstance_VTable*)pInstance->m_Base.m_Base.m_pVTable;

	pVTable->ModelInstance__GetNodeTransforms(pInstance);
}

void ModelSetup::QueueModelInfo(ModelInstance* pInstance)
{
	// TODO - are unordered_maps too slow? use vectors?
	Array_QueuedModelInfo& modelInfo = (*m_QueuedModelInfo.try_emplace(pInstance->GetModel()).first).second;

	if (FindQueuedModelInfo(modelInfo, pInstance) == nullptr)
		modelInfo.emplace_back(pInstance);
}

QueuedModelInfo* ModelSetup::FindQueuedModelInfo(ModelInstance* pInstance)
{
	auto iter = m_QueuedModelInfo.find(pInstance->GetModel());

	return iter != m_QueuedModelInfo.end() ? FindQueuedModelInfo(iter->second, pInstance) : nullptr;
}

QueuedModelInfo* ModelSetup::FindQueuedModelInfo(Array_QueuedModelInfo& aInfo, ModelInstance* pInstance)
{
	Array_QueuedModelInfo::iterator iter = std::find_if(aInfo.begin(), aInfo.end(),
		[pInstance](QueuedModelInfo& info) { return info.m_pInstance == pInstance; });

	return iter != aInfo.end() ? &(*iter) : nullptr;
}

void ModelSetup::StaticLightCB(WorldTreeObj* pObj, void* pUser)
{
	QueuedModelInfo* pModelInfo = ((StaticLightCallbackData*)pUser)->m_pInfo;
	StaticLight* pStaticLight = (StaticLight*)pObj;

	/*if (pStaticLight->m_vInnerColor.x < 1.0f &&
		pStaticLight->m_vInnerColor.y < 1.0f &&
		pStaticLight->m_vInnerColor.z < 1.0f)
	{
		return;
	}*/

	if (pModelInfo->m_dwStaticLightCount == MAX_STATIC_LIGHTS_PER_MODEL)
		return;

	// TODO - prioritize based on a score system?
	pModelInfo->m_apStaticLight[pModelInfo->m_dwStaticLightCount] = pStaticLight;
	pModelInfo->m_dwStaticLightCount++;
}

static bool CastRayAtSky_Internal(DirectX::XMFLOAT3* pFrom, DirectX::XMFLOAT3* pDir)
{
	XMIntersectQuery iQuery;
	XMIntersectInfo iInfo;

	iQuery.m_vFrom = *pFrom;
	DirectX::XMStoreFloat3(&iQuery.m_vTo, DirectX::XMLoadFloat3(pFrom) + DirectX::XMLoadFloat3(pDir));
	iQuery.m_dwFlags = INTERSECT_HPOLY;

	MainWorld* pMainWorld = g_pSceneDesc->m_pRenderContext->m_pMainWorld;

	if (i_IntersectSegment(&iQuery, &iInfo, &pMainWorld->m_WorldTree))
	{
		WorldPoly* pPoly = pMainWorld->GetPolyFromHPoly(iInfo.m_hPoly);
		//if (pPoly == nullptr)
		//	return false;

		return (pPoly->m_pSurface->m_dwFlags & SURF_SKY);
	}
	else
	{
		return false;
	}

	return true;
}

static bool CastRayAtSky_Original(DirectX::XMFLOAT3* pFrom, DirectX::XMFLOAT3* pDir)
{
	IntersectQuery iQuery;
	IntersectInfo iInfo;

	iQuery.m_vFrom = *PXMFLOAT3_TO_PLTVECTOR(pFrom);
	DirectX::XMStoreFloat3(PLTVECTOR_TO_PXMFLOAT3(&iQuery.m_vTo), DirectX::XMLoadFloat3(pFrom) + 
		DirectX::XMLoadFloat3(pDir));

	iQuery.m_dwFlags = INTERSECT_HPOLY;

	MainWorld* pMainWorld = g_pSceneDesc->m_pRenderContext->m_pMainWorld;

	if (g_GlobalMgr.GetEngineHacks()->i_IntersectSegment(&iQuery, &iInfo, &pMainWorld->m_WorldTree))
	{
		WorldPoly* pPoly = pMainWorld->GetPolyFromHPoly(iInfo.m_hPoly);
		//if (pPoly == nullptr)
		//	return false;

		return (pPoly->m_pSurface->m_dwFlags & SURF_SKY);
	}
	else
	{
		return false;
	}

	return true;
}

float ModelSetup::GetDirLightAmount(ModelInstance* pInstance, DirectX::XMFLOAT3* pInstancePos, 
	DirectX::XMFLOAT4X4* pTransform)
{
	WorldTreeNode* pWTRoot = &g_pSceneDesc->m_pRenderContext->m_pMainWorld->m_WorldTree.m_RootNode;

	float fLongestDist = DirectX::XMVectorGetX(DirectX::XMVector3Length(
		DirectX::XMLoadFloat3(PLTVECTOR_TO_PXMFLOAT3(&pWTRoot->m_vBBoxMax)) -
		DirectX::XMLoadFloat3(PLTVECTOR_TO_PXMFLOAT3(&pWTRoot->m_vBBoxMin))));

	DirectX::XMVECTOR vGlobalLightDir = DirectX::XMLoadFloat3(PLTVECTOR_TO_PXMFLOAT3(&g_pStruct->m_vGlobalLightDir));
	
	DirectX::XMFLOAT3 vDir;
	DirectX::XMStoreFloat3(&vDir, vGlobalLightDir * -fLongestDist);

	if (!g_CV_ModelSunVariance.m_Val)
		return CastRayAtSky_Original(pInstancePos, &vDir) ? 1.0f : 0.0f;

	DirectX::XMFLOAT3 vUp, vForward, vRight;
	Matrix_GetBasisVectorsLT(pTransform, &vRight, &vUp, &vForward);

	DirectX::XMVECTOR vLightUp = DirectX::XMVector3Cross(DirectX::XMLoadFloat3(&vRight), vGlobalLightDir);

	vLightUp *= (pInstance->GetModel()->m_fVisRadius *
		LTMAX(pInstance->m_Base.m_vScale.x, LTMAX(pInstance->m_Base.m_vScale.y, pInstance->m_Base.m_vScale.z)));

	DirectX::XMVECTOR vInstancePos = DirectX::XMLoadFloat3(pInstancePos);
	DirectX::XMFLOAT3 vTopIn, vBottomIn;
	DirectX::XMStoreFloat3(&vTopIn, vInstancePos + vLightUp);
	DirectX::XMStoreFloat3(&vBottomIn, vInstancePos - vLightUp);

	bool bTopInLight = CastRayAtSky_Original(&vTopIn, &vDir);
	bool bBottomInLight = CastRayAtSky_Original(&vBottomIn, &vDir);

	if (bTopInLight == bBottomInLight)
		return (bTopInLight) ? 1.0f : 0.0f;

	bool bMiddleInLight;
	float fVariance = 1.0f;
	float fTop = 1.0f;
	float fBottom = -1.0f;
	float fLight = 1.0f;
	float fMiddle;

	while ((fVariance > g_CV_ModelSunVariance.m_Val) && (bBottomInLight != bTopInLight))
	{
		fMiddle = (fTop + fBottom) * 0.5f;

		DirectX::XMFLOAT3 vMiddleIn;
		DirectX::XMStoreFloat3(&vMiddleIn, vInstancePos + (vLightUp * fMiddle));
		bMiddleInLight = CastRayAtSky_Original(&vMiddleIn, &vDir);

		fVariance *= 0.5f;

		if (!bMiddleInLight)
			fLight -= fVariance;

		if (bMiddleInLight == bTopInLight)
		{
			fTop = fMiddle;
			bTopInLight = bMiddleInLight;
		}
		else
		{
			fBottom = fMiddle;
			bBottomInLight = bMiddleInLight;
		}
	}

	return fLight;
}

void ModelSetup::SetupModelLight(ModelInstance* pInstance, QueuedModelInfo& modelInfo)
{
	modelInfo.m_dwDynamicLightCount = 0;
	modelInfo.m_dwStaticLightCount = 0;

	// TODO - R2/R3 env map cloak has FLAG_NOLIGHT but affected by lighting
	if (modelInfo.m_ModelHookData.m_dwObjectFlags & FLAG_NOLIGHT)
	{
		modelInfo.m_vAmbientLight = { 1.0f, 1.0f, 1.0f };
		modelInfo.m_vDirLightColor = { };

		return;
	}

	Model* pModel = pInstance->GetModel();
	uint32 dwRootNode = pModel->m_pRootNode->m_wNodeIndex;
	DirectX::XMVECTOR vInstancePos;

	if (pModel->GetNode(dwRootNode)->GetChildCount() > 0)
	{
		uint32 dwFirstChild = pModel->GetNode(dwRootNode)->GetChild(0)->m_wNodeIndex;
		DirectX::XMFLOAT4X4* pTransform = PLTMATRIX_TO_PXMFLOAT4X4(pInstance->GetCachedTransform(dwFirstChild));

		DirectX::XMFLOAT3 vNodePos;
		Matrix_GetTranslationLT(pTransform, &vNodePos);

		vInstancePos = DirectX::XMVector4Transform(
			DirectX::XMVectorSet(vNodePos.x, vNodePos.y, vNodePos.z, 1.0f),
			DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&modelInfo.m_sTransforms.m_mWorld)));
	}
	else
	{
		vInstancePos = DirectX::XMVectorSet(pInstance->m_Base.m_vPos.x,
			pInstance->m_Base.m_vPos.y, pInstance->m_Base.m_vPos.z, 1.0f);
	}

	bool bReallyClose = (modelInfo.m_ModelHookData.m_dwObjectFlags & FLAG_REALLYCLOSE);

	if (bReallyClose)
	{
		vInstancePos = DirectX::XMVector4Transform(vInstancePos,
			DirectX::XMLoadFloat4x4(&g_ViewParams.m_mInvViewTransposed));
	}

	float fDirLightAmount = 0.0f;
	modelInfo.m_vDirLightColor = { };

	if (g_bHaveWorld)
	{
		if (g_pStruct->m_vGlobalLightColor.x || g_pStruct->m_vGlobalLightColor.y || g_pStruct->m_vGlobalLightColor.z)
		{
			if (pInstance->m_fLastDirLightAmount >= 0.0f && !(pInstance->m_Base.m_dwFlags2 & FLAG2_DYNAMICDIRLIGHT))
			{
				fDirLightAmount = pInstance->m_fLastDirLightAmount;

				goto LABEL_SetupModelLight_SetupDirLight;
			}
			else
			{
				float fSunVarianceEx = g_CV_ModelSunVariance.m_Val + 0.001f;
				DirectX::XMVECTOR vDiff = vInstancePos -
					DirectX::XMLoadFloat3(PLTVECTOR_TO_PXMFLOAT3(&pInstance->m_vLastDirLightPos));

				if (DirectX::XMVectorGetX(DirectX::XMVector3Dot(vDiff, vDiff)) <= fSunVarianceEx * fSunVarianceEx)
				{
					fDirLightAmount = pInstance->m_fLastDirLightAmount;

					goto LABEL_SetupModelLight_SetupDirLight;
				}

				/*DirectX::XMVector3NearEqual(vInstancePos, 
				DirectX::XMLoadFloat3(PLTVECTOR_TO_PXMFLOAT3(&pInstance->m_vLastDirLightPos)),
				DirectX::XMVectorSet(fSunVariance, fSunVariance, fSunVariance, 0.0f)))*/
			}

			DirectX::XMStoreFloat3(PLTVECTOR_TO_PXMFLOAT3(&pInstance->m_vLastDirLightPos), vInstancePos);
			fDirLightAmount = GetDirLightAmount(pInstance, PLTVECTOR_TO_PXMFLOAT3(&pInstance->m_vLastDirLightPos),
				&modelInfo.m_sTransforms.m_mWorld);

			pInstance->m_fLastDirLightAmount = fDirLightAmount;

		LABEL_SetupModelLight_SetupDirLight:

			modelInfo.m_vDirLightColor =
			{
				g_pStruct->m_vGlobalLightColor.x * MATH_ONE_OVER_255 * fDirLightAmount,
				g_pStruct->m_vGlobalLightColor.y * MATH_ONE_OVER_255 * fDirLightAmount,
				g_pStruct->m_vGlobalLightColor.z * MATH_ONE_OVER_255 * fDirLightAmount,
			};
		}

		DirectX::XMFLOAT3 vPos;
		DirectX::XMStoreFloat3(&vPos, vInstancePos);

		d3d_LightLookup_Original(&g_pSceneDesc->m_pRenderContext->m_pMainWorld->m_LightTable,
			&vPos, &modelInfo.m_vAmbientLight);

		WorldTree* pWorldTree = &g_pSceneDesc->m_pRenderContext->m_pMainWorld->m_WorldTree;
		WorldTree_VTable* pVTable = (WorldTree_VTable*)pWorldTree->m_pVTable;

		StaticLightCallbackData callbackData;
		callbackData.m_pInstance = pInstance;
		callbackData.m_pInfo = &modelInfo;

		DirectX::XMVECTOR vVisRadius = DirectX::XMVectorSet(pModel->m_fVisRadius, pModel->m_fVisRadius, pModel->m_fVisRadius, 0.0f);

		FindObjInfo foInfo;
		foInfo.m_eObjArray = NOA_Lights;
		DirectX::XMStoreFloat3(PLTVECTOR_TO_PXMFLOAT3(&foInfo.m_vMin), vInstancePos - vVisRadius);
		DirectX::XMStoreFloat3(PLTVECTOR_TO_PXMFLOAT3(&foInfo.m_vMax), vInstancePos + vVisRadius);
		foInfo.m_CB = &ModelSetup::StaticLightCB;
		foInfo.m_pCBUser = &callbackData;

		pVTable->WorldTree__FindObjectsInBox2(pWorldTree, 0, &foInfo);
	}
	else
	{
		modelInfo.m_vAmbientLight = { 1.0f, 1.0f, 1.0f };
	}

	AllocSet* pSet = d3d_GetVisibleSet()->GetLights();

	for (uint32 i = 0; i < pSet->GetObjectCount(); i++)
	{
		DynamicLight* pDynamicLight = pSet->GetObjectByIndex(i)->ToDynamicLight();

		if (pDynamicLight->m_Base.m_dwFlags & FLAG_ONLYLIGHTWORLD)
			continue;

		float fDistSqr = DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(vInstancePos -
			DirectX::XMLoadFloat3(PLTVECTOR_TO_PXMFLOAT3(&pDynamicLight->m_Base.m_vPos))));

		// TODO - global radius?
		float fSumRadius = pModel->m_fVisRadius + pDynamicLight->m_fLightRadius;

		// TODO - prioritize based on a score system?
		if (fDistSqr < fSumRadius * fSumRadius)
		{
			modelInfo.m_aDynamicLightIndex[modelInfo.m_dwDynamicLightCount] = i;
			modelInfo.m_dwDynamicLightCount++;

			if (modelInfo.m_dwDynamicLightCount == MAX_DYNAMIC_LIGHTS_PER_MODEL)
				return;
		}
	}
}

void ModelSetup::FillModelInfo(QueuedModelInfo* pInfo, ModelInstance* pInstance)
{
	ModelInstanceHookData instanceHookData = { };
	g_ModelSetup.CallModelInstanceHook(pInstance, &instanceHookData);

	CallModelHook(pInstance, &pInfo->m_ModelHookData);

	DirectX::XMFLOAT4X4 mModelTransform;
	LTObject* pObject = &pInstance->m_Base;
	d3d_SetupTransformation(&pObject->m_vPos, (float*)&pObject->m_rRot, &pObject->m_vScale, &mModelTransform);

	DirectX::XMFLOAT4X4 mReallyCloseProj;
	pInfo->m_sTransforms.m_mWorld = mModelTransform;

	if (pInfo->m_ModelHookData.m_dwObjectFlags & FLAG_REALLYCLOSE)
	{
		float fFOVOffsetX = 0.0f;
		float fFOVOffsetY = 0.0f;

		Model* pModel = pInstance->GetModel();

		if (pModel->m_bFOVOffset)
		{
			fFOVOffsetX = pModel->m_fFOVOffsetX;
			fFOVOffsetY = pModel->m_fFOVOffsetY;

			d3d_CalcReallyCloseMatrix(&mReallyCloseProj,
				MATH_DEGREES_TO_RADIANS(g_CV_ExtraFOVXOffset.m_Val) + fFOVOffsetX,
				MATH_DEGREES_TO_RADIANS(g_CV_ExtraFOVYOffset.m_Val) + fFOVOffsetY, true);
		}
		else
		{
			d3d_CalcReallyCloseMatrix(&mReallyCloseProj, fFOVOffsetX, fFOVOffsetY, false);
		}

		d3d_GetFinalModelTransform(&pInfo->m_sTransforms.m_mWorldViewProj,
			&pInfo->m_sTransforms.m_mWorldView,
			&mModelTransform, &mReallyCloseProj);
	}
	else
	{
		d3d_GetFinalModelTransform(&pInfo->m_sTransforms.m_mWorldViewProj,
			&pInfo->m_sTransforms.m_mWorldView,
			&mModelTransform, nullptr);
	}

	CacheModelInstanceTransforms(pInstance);

	SetupModelLight(pInstance, *pInfo);
}

void ModelSetup::QueueAllModelInfo()
{	
	for (auto& modelInstanceInfo : m_QueuedModelInfo)
	{
		Array_QueuedModelInfo& aQueuedInfo = modelInstanceInfo.second;

		for (QueuedModelInfo& queuedInfo : aQueuedInfo)
		{
			ModelInstance* pInstance = queuedInfo.m_pInstance;
			
			ModelInstanceHookData instanceHookData = { };
			g_ModelSetup.CallModelInstanceHook(pInstance, &instanceHookData);

			CallModelHook(pInstance, &queuedInfo.m_ModelHookData);

			DirectX::XMFLOAT4X4 mModelTransform;
			LTObject* pObject = &pInstance->m_Base;
			d3d_SetupTransformation(&pObject->m_vPos, (float*)&pObject->m_rRot, &pObject->m_vScale, &mModelTransform);

			DirectX::XMFLOAT4X4 mReallyCloseProj;
			queuedInfo.m_sTransforms.m_mWorld = mModelTransform;

			if (queuedInfo.m_ModelHookData.m_dwObjectFlags & FLAG_REALLYCLOSE)
			{
				float fFOVOffsetX = 0.0f;
				float fFOVOffsetY = 0.0f;

				Model* pModel = pInstance->GetModel();

				if (pModel->m_bFOVOffset)
				{
					fFOVOffsetX = pModel->m_fFOVOffsetX;
					fFOVOffsetY = pModel->m_fFOVOffsetY;

					d3d_CalcReallyCloseMatrix(&mReallyCloseProj,
						MATH_DEGREES_TO_RADIANS(g_CV_ExtraFOVXOffset.m_Val) + fFOVOffsetX,
						MATH_DEGREES_TO_RADIANS(g_CV_ExtraFOVYOffset.m_Val) + fFOVOffsetY, true);
				}
				else
				{
					d3d_CalcReallyCloseMatrix(&mReallyCloseProj, fFOVOffsetX, fFOVOffsetY, false);
				}

				d3d_GetFinalModelTransform(&queuedInfo.m_sTransforms.m_mWorldViewProj,
					&queuedInfo.m_sTransforms.m_mWorldView,
					&mModelTransform, &mReallyCloseProj);
			}
			else
			{
				d3d_GetFinalModelTransform(&queuedInfo.m_sTransforms.m_mWorldViewProj,
					&queuedInfo.m_sTransforms.m_mWorldView,
					&mModelTransform, nullptr);
			}

			CacheModelInstanceTransforms(pInstance);

			SetupModelLight(pInstance, queuedInfo);
		}
	}
}

void ModelSetup::QueueModel(ModelInstance* pInstance)
{	
	QueuedModelInfo* pInfo = FindQueuedModelInfo(pInstance);

	if (pInfo != nullptr)
		QueueModelPieces(pInstance, pInfo);
}

void ModelSetup::QueueModel(ModelInstance* pInstance, QueuedModelInfo* pInfo)
{
	QueueModelPieces(pInstance, pInfo);
}

void ModelSetup::FreeQueuedData()
{
	m_QueuedModelInfo.clear();

	Array_QueuedPieceInfo emptyPieceList;
	emptyPieceList.swap(m_aQueuedPieceInfo);

	m_ModelPrimaryNodes.clear();
}

float ModelSetup::CalcModelPieceDistSqr(QueuedPieceInfo* pPieceInfo, XMFloat4x4Trinity* pTransforms, 
	Array_UInt32& aModelPrimaryNodes)
{
	DirectX::XMFLOAT4X4* pTransform;
	uint32 dwPrimaryNode = aModelPrimaryNodes[pPieceInfo->m_dwPieceIndex];

	if (dwPrimaryNode == UINT32_MAX)
	{
		memset(g_aCurrPrimaryNode, 0, sizeof(g_aCurrPrimaryNode));

		ModelPiece* pPiece = pPieceInfo->m_pModel->GetPiece(pPieceInfo->m_dwPieceIndex);

		for (uint32 dwVert = 0; dwVert < pPiece->GetVertCount(); dwVert++)
		{
			ModelVert* pVert = pPiece->GetVert(dwVert);

			for (uint32 dwWeight = 0; (dwWeight < pVert->m_wWeights) && (dwWeight < MODEL_MAX_WEIGHTS_PER_VERTEX); dwWeight++)
				g_aCurrPrimaryNode[pVert->m_pWeights[dwWeight].m_dwNode]++;
		}

		uint32 dwBestIndex = 0;
		for (uint32 i = 1; i < pPieceInfo->m_pModel->GetTransformCount(); i++)
		{
			if (g_aCurrPrimaryNode[i] > g_aCurrPrimaryNode[dwBestIndex])
				dwBestIndex = i;
		}

		pTransform = PLTMATRIX_TO_PXMFLOAT4X4(pPieceInfo->m_pInstance->GetCachedTransform(dwBestIndex));
	}
	else
	{
		pTransform = PLTMATRIX_TO_PXMFLOAT4X4(pPieceInfo->m_pInstance->GetCachedTransform(dwPrimaryNode));
	}

	// TODO - for scaled models?
	/*DirectX::XMFLOAT3 v0, v1, v2;
	Matrix_GetBasisVectorsLT(pTransform, &v0, &v1, &v2);

	DirectX::XMStoreFloat3(&v0, DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&v0)));
	DirectX::XMStoreFloat3(&v1, DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&v1)));
	DirectX::XMStoreFloat3(&v2, DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&v2)));

	Matrix_SetBasisVectors2LT(pTransform, &v0, &v1, &v2)*/;

	DirectX::XMFLOAT3 vSignificantNodePos;
	Matrix_GetTranslationLT(pTransform, &vSignificantNodePos);

	DirectX::XMVECTOR vCameraPos = DirectX::XMLoadFloat3(PLTVECTOR_TO_PXMFLOAT3(&g_ViewParams.m_vPos));
	DirectX::XMVECTOR vPiecePos =
		DirectX::XMVectorSet(vSignificantNodePos.x, vSignificantNodePos.y, vSignificantNodePos.z, 1.0f);

	if (pPieceInfo->m_bReallyClose)
	{
		vPiecePos = DirectX::XMVector4Transform(vPiecePos, 
			DirectX::XMLoadFloat4x4(&g_ViewParams.m_mInvViewTransposed));
	}

	vPiecePos = DirectX::XMVector4Transform(vPiecePos,
		DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&pTransforms->m_mWorld)));

	return DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(vCameraPos - vPiecePos));
}

void ModelSetup::QueueModelPieces(ModelInstance* pInstance, QueuedModelInfo* pModelInfo)
{
	float fDistToModel = 0.0f;
	if (!(pModelInfo->m_ModelHookData.m_dwObjectFlags & FLAG_REALLYCLOSE))
	{
		DirectX::XMVECTOR vCameraPos = DirectX::XMLoadFloat3(PLTVECTOR_TO_PXMFLOAT3(&g_ViewParams.m_vPos));
		DirectX::XMVECTOR vPos = DirectX::XMLoadFloat3(PLTVECTOR_TO_PXMFLOAT3(&pInstance->m_Base.m_vPos));

		fDistToModel = DirectX::XMVectorGetX(DirectX::XMVector3Length(vCameraPos - vPos));
	}
	else
	{
		if (!g_CV_DrawGuns.m_Val)
			return;
	}

	bool bTexture = g_CV_TextureModels && (pModelInfo->m_ModelHookData.m_dwHookFlags & MHF_USETEXTURE);

	Model* pModel = pInstance->GetModel();
	Array_UInt32& aModelPrimaryNodes = m_ModelPrimaryNodes[pModel];

	if (!aModelPrimaryNodes.size())
		aModelPrimaryNodes.resize(pModel->GetPieceCount(), UINT32_MAX);

	for (uint32 dwCurrPiece = 0; dwCurrPiece < pModel->GetPieceCount(); dwCurrPiece++)
	{
		if (pInstance->IsPieceHidden(dwCurrPiece))
			continue;

		ModelPiece* pPiece = pModel->GetPiece(dwCurrPiece);
		uint32 dwLODIndex = 0;
		
#ifdef MODEL_PIECELESS_RENDERING
		if (!g_CV_ModelNoLODs.m_Val)
			dwLODIndex = pPiece->GetLODIndexFromDist(g_CV_ModelLODOffset.m_Val, fDistToModel / g_CV_ModelZoomScale.m_Val);
#endif

		if (dwLODIndex && !pPiece->GetLOD(dwLODIndex - 1)->GetTriCount())
			continue;

		QueueModelPiece(pInstance, pModel, dwCurrPiece, dwLODIndex, bTexture, pModelInfo, fDistToModel, 
			aModelPrimaryNodes);
	}
}

void ModelSetup::QueueModelPiece(ModelInstance* pInstance, Model* pModel, uint32 dwPieceIndex, uint32 dwLODIndex, 
	bool bTexture, QueuedModelInfo* pModelInfo, float fDistToModel,
	Array_UInt32& aModelPrimaryNodes)
{
	QueuedPieceInfo queuedInfo;

	queuedInfo.m_bReallyClose = (pModelInfo->m_ModelHookData.m_dwObjectFlags & FLAG_REALLYCLOSE);
	queuedInfo.m_pInstance = pInstance;
	queuedInfo.m_pModel = pModel;
	queuedInfo.m_dwPieceIndex = dwPieceIndex;
	queuedInfo.m_dwLODIndex = dwLODIndex;
	queuedInfo.m_pModelInfo = pModelInfo;
	queuedInfo.m_pTexture = nullptr;

	if (pModel->GetPieceCount() > 1 && d3d_IsTranslucentModel(&pInstance->m_Base))
	{
		queuedInfo.m_fDistanceSqr = CalcModelPieceDistSqr(&queuedInfo, &pModelInfo->m_sTransforms, aModelPrimaryNodes);
	}
	else
	{
		queuedInfo.m_fDistanceSqr = fDistToModel * fDistToModel;
	}
	 
	if (bTexture)
	{
		SharedTexture* pTexture = pInstance->m_pSkins[pModel->GetPiece(dwPieceIndex)->m_wTextureIndex];
		if (pTexture != nullptr && pTexture->m_pRenderData != nullptr)
			queuedInfo.m_pTexture = pTexture;
	}

	m_aQueuedPieceInfo.push_back(queuedInfo);
}
