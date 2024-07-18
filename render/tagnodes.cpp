#include "pch.h"

#include "tagnodes.h"
#include "common_stuff.h"
#include "rendererconsolevars.h"
#include "draw_objects.h"
#include "common_draw.h"
#include "globalmgr.h"
#include "d3d_viewparams.h"
#include "d3d_draw.h"

using namespace DirectX;

static VisibleSet g_VisibleSet;
ViewParams* g_pCurrViewParams;

DirectX::XMFLOAT3 g_vCameraPosVisBoxMin;
DirectX::XMFLOAT3 g_vCameraPosVisBoxMax;

AllocSet::AllocSet()
{
	m_szSetName = nullptr;
	m_dwMaxObjects = 0; 
}

AllocSet::~AllocSet()
{
	Term();
}

void AllocSet::Init(char* szSetName, uint32 dwDefaultMax)
{
	Term();

	uint32 dwMaxObjects;
	HLTPARAM hParam = g_pStruct->GetParameter(szSetName);

	if (hParam)
		dwMaxObjects = (uint32)g_pStruct->GetParameterValueFloat(hParam);
	else
		dwMaxObjects = dwDefaultMax;

	m_Objects.reserve(dwMaxObjects);
	m_dwMaxObjects = !g_CV_UnlimitedVS.m_Val ? dwMaxObjects : UINT_MAX;
	m_szSetName = szSetName;
}

void AllocSet::Term()
{
	m_Objects.clear();
	m_dwMaxObjects = 0;
}

#define IFLAG_IN_VISIBILITY_SET	(1<<31)

void AllocSet::ClearSet()
{
	for (LTObject* pObject : m_Objects)
		pObject->m_dwInternalFlags &= ~IFLAG_IN_VISIBILITY_SET;

	m_Objects.clear();
	//m_ObjectFlags.clear();
}

void AllocSet::Add(LTObject* pObject)
{
	if (pObject->m_dwInternalFlags & IFLAG_IN_VISIBILITY_SET)
		return;

	// TODO - duplicates check shouldn't be here, should be handled upper with frame code?
	//if (std::find(m_Objects.begin(), m_Objects.end(), pObject) != m_Objects.end())
	//	return;

	//if (m_ObjectFlags.find(pObject) != m_ObjectFlags.end())
	//	return;
	//m_ObjectFlags[pObject] = true;

	if (m_Objects.size() < m_dwMaxObjects)
	{
		pObject->m_dwInternalFlags |= IFLAG_IN_VISIBILITY_SET;
		m_Objects.push_back(pObject);
	}
	else
	{
		AddDebugMessage(0, "Set '%s' is overflowed", m_szSetName);
	}
}

void AllocSet::Draw(ViewParams* pParams, DrawObjectFn pDrawFn)
{
	for (LTObject* pObject : m_Objects)
	{
		if (!(pObject->m_dwFlags & FLAG_VISIBLE))
			AddDebugMessage(0, "Invisible object is drawn");

		//if (pObject->m_nColorA == 0.0f)
		//	AddDebugMessage(0, "Object with A = 0 is drawn");

		pDrawFn(pParams, pObject);
	}
}

void AllocSet::Queue(ObjectDrawList* pDrawList, ViewParams* pParams, DrawObjectFn pDrawFn)
{
	for (LTObject* pObject : m_Objects)
	{
		if (!(pObject->m_dwFlags & FLAG_VISIBLE))
			AddDebugMessage(0, "Invisible object is queued");

		//if (pObject->m_nColorA == 0.0f)
		//	AddDebugMessage(0, "Object with A = 0 is drawn");

		pDrawList->Add(pParams, pObject, pDrawFn);
	}
}

void AllocSet::Queue(ObjectDrawList* pDrawList, ViewParams* pParams, DrawObjectFn pDrawFn, DrawObjectExFn pDrawExFn)
{
	for (LTObject* pObject : m_Objects)
	{
		if (!(pObject->m_dwFlags & FLAG_VISIBLE))
			AddDebugMessage(0, "Invisible object is queued");

		//if (pObject->m_nColorA == 0.0f)
		//	AddDebugMessage(0, "Object with A = 0 is drawn");

		pDrawList->AddEx(pParams, pObject, pDrawFn, pDrawExFn);
	}
}

VisibleSet::VisibleSet()
{
	ClearSet();
}

void VisibleSet::Init()
{
	m_SolidModels.Init((char*)"VS_MODELS", MAX_VISIBLE_MODELS);
	m_Sets.push_back(&m_SolidModels);

	m_TranslucentModels.Init((char*)"VS_MODELS_TRANSLUCENT", MAX_VISIBLE_MODELS);
	m_Sets.push_back(&m_TranslucentModels);

	m_ChromakeyModels.Init((char*)"VS_MODELS_CHROMAKEY", MAX_VISIBLE_MODELS);
	m_Sets.push_back(&m_ChromakeyModels);

	m_TranslucentSprites.Init((char*)"VS_SPRITES", MAX_VISIBLE_SPRITES);
	m_Sets.push_back(&m_TranslucentSprites);

	m_NoZSprites.Init((char*)"VS_SPRITES_NOZ", MAX_VISIBLE_SPRITES);
	m_Sets.push_back(&m_NoZSprites);

	m_SolidWorldModels.Init((char*)"VS_WORLDMODELS", MAX_VISIBLE_WORLDMODELS);
	m_Sets.push_back(&m_SolidWorldModels);

	m_TranslucentWorldModels.Init((char*)"VS_WORLDMODELS_TRANSLUCENT", MAX_VISIBLE_WORLDMODELS);
	m_Sets.push_back(&m_TranslucentWorldModels);

	m_ChromakeyWorldModels.Init((char*)"VS_WORLDMODELS_CHROMAKEY", MAX_VISIBLE_WORLDMODELS);
	m_Sets.push_back(&m_ChromakeyWorldModels);

	m_Lights.Init((char*)"VS_LIGHTS", MAX_VISIBLE_LIGHTS);
	m_Sets.push_back(&m_Lights);

	m_SolidPolyGrids.Init((char*)"VS_POLYGRIDS", MAX_VISIBLE_POLYGRIDS);
	m_Sets.push_back(&m_SolidPolyGrids);

	m_TranslucentPolyGrids.Init((char*)"VS_POLYGRIDS_TRANSLUCENT", MAX_VISIBLE_POLYGRIDS);
	m_Sets.push_back(&m_TranslucentPolyGrids);

	m_LineSystems.Init((char*)"VS_LINESYSTEMS", MAX_VISIBLE_LINESYSTEMS);
	m_Sets.push_back(&m_LineSystems);

	m_ParticleSystems.Init((char*)"VS_PARTICLESYSTEMS", MAX_VISIBLE_PARTICLESYSTEMS);
	m_Sets.push_back(&m_ParticleSystems);

	m_SolidCanvases.Init((char*)"VS_CANVASES", MAX_VISIBLE_CANVASES);
	m_Sets.push_back(&m_SolidCanvases);

	m_TranslucentCanvases.Init((char*)"VS_CANVASES_TRANSLUCENT", MAX_VISIBLE_CANVASES);
	m_Sets.push_back(&m_TranslucentCanvases);
}

void VisibleSet::Term()
{
	m_Sets.clear();
}

void VisibleSet::ClearSet()
{
	for (auto pSet : m_Sets)
		pSet->ClearSet();
}

VisibleSet* d3d_GetVisibleSet()
{
	return &g_VisibleSet;
}

static bool d3d_IsSkyObject(LTObject* pObject)
{
	for (uint32 i = 0; i < (uint32)g_pSceneDesc->m_nSkyObjects; i++)
	{
		if (g_pSceneDesc->m_pSkyObjects[i] == pObject)
			return true;
	}

	return false;
}

static inline bool d3d_ShouldProcessObject(LTObject* pObject)
{
	return (pObject->m_Base.m_dwWTFrameCode != g_dwCurObjectFrameCode) &&
		g_ObjectHandlers[pObject->m_nObjectType].m_pProcessObjectFn &&
		pObject->m_dwFlags & FLAG_VISIBLE && // TODO - portals related flags?
		!(pObject->m_dwFlags2 & FLAG2_SKYOBJECT) &&
		!d3d_IsSkyObject(pObject); // TODO - unnecessary?
}

static bool d3d_CheckObjectVisibility(LTObject* pObject)
{
	// TODO - are BBoxes enough?
#if defined(DEBUG) || defined(_DEBUG)
	if (g_CV_VisBoxTest.m_Val &&
		!Vector_DoBoxesTouch(PLTVECTOR_TO_PXMFLOAT3(&pObject->m_vBBoxMin), PLTVECTOR_TO_PXMFLOAT3(&pObject->m_vBBoxMax),
		&g_vCameraPosVisBoxMin, &g_vCameraPosVisBoxMax))
	{
		return false;
	}
#endif

#ifdef WORLD_TWEAKS_ENABLED
	if ((g_GlobalMgr.GetWorldTweakMgr()->m_dwAllFlags & WORLD_TWEAK_OBJ_DIST_TEST) &&
		!Vector_DoBoxesTouch(PLTVECTOR_TO_PXMFLOAT3(&pObject->m_vBBoxMin), PLTVECTOR_TO_PXMFLOAT3(&pObject->m_vBBoxMax),
			&g_GlobalMgr.GetWorldTweakMgr()->m_vObj_BBoxMin, &g_GlobalMgr.GetWorldTweakMgr()->m_vObj_BBoxMax))
	{
		return false;
	}
#endif

	if (!g_CV_FrustumTest.m_Val)
		return true;
	
	if (g_ObjectHandlers[pObject->m_nObjectType].m_bCheckWorldVisibility)
	{
		DirectX::XMFLOAT3 vDims = g_ObjectHandlers[pObject->m_nObjectType].m_pGetDims(pObject);
		DirectX::XMVECTOR vDimsTemp = DirectX::XMLoadFloat3(&vDims);
		DirectX::XMVECTOR vPosTemp = DirectX::XMLoadFloat3(PLTVECTOR_TO_PXMFLOAT3(&pObject->m_vPos));

		DirectX::XMFLOAT3 vPosMinusDims, vPosPlusDims;

		DirectX::XMStoreFloat3(&vPosMinusDims, vPosTemp - vDimsTemp);
		DirectX::XMStoreFloat3(&vPosPlusDims, vPosTemp + vDimsTemp);

		return g_RenderWorld.IsAABBVisible(&g_ViewParams, &vPosMinusDims, &vPosPlusDims);
	}

	return true;
}

static void d3d_ProcessAttachments(LTObject* pObject, uint32 dwDepth)
{
	Attachment* pCurr = pObject->m_pAttachments;
	while (pCurr)
	{
		LTObject* pAttachedObject = g_pStruct->ProcessAttachment(pObject, pCurr);

		// TODO - ProcessAttachment always resets m_dwWTFrameCode? Therefore duplicates are possible?
		if (pAttachedObject != nullptr && (pAttachedObject->m_Base.m_dwWTFrameCode != g_dwCurObjectFrameCode))
		{
			d3d_ProcessAttachments(pAttachedObject, dwDepth + 1);

			if (d3d_ShouldProcessObject(pAttachedObject) && d3d_CheckObjectVisibility(pAttachedObject))
			{
				g_ObjectHandlers[pAttachedObject->m_nObjectType].m_pProcessObjectFn(pAttachedObject);

				pAttachedObject->m_Base.m_dwWTFrameCode = g_dwCurObjectFrameCode;
			}
		}

		pCurr = pCurr->m_pNext;
	}
}

static void d3d_ReallyProcessObject(LTObject* pObject)
{
	if (g_bHaveWorld && !(pObject->m_dwFlags & FLAG_REALLYCLOSE))
	{
		if (d3d_CheckObjectVisibility(pObject))
			g_ObjectHandlers[pObject->m_nObjectType].m_pProcessObjectFn(pObject);
	}
	else
	{
		g_ObjectHandlers[pObject->m_nObjectType].m_pProcessObjectFn(pObject);
	}	

	if (pObject->m_pAttachments != nullptr)
		d3d_ProcessAttachments(pObject, 0);
}

static void d3d_CheckAndProcessObject(LTObject* pObject, bool bUpdateFrameCode)
{
	if (d3d_ShouldProcessObject(pObject))
	{
		if (bUpdateFrameCode)
			pObject->m_Base.m_dwWTFrameCode = g_dwCurObjectFrameCode;

		d3d_ReallyProcessObject(pObject);	
	}
}

static inline bool d3d_IsWorldNodeVisible(ViewParams* pParams, DirectX::XMFLOAT3* pBoxMin,
	DirectX::XMFLOAT3* pBoxMax)
{
	DirectX::XMFLOAT3& vViewMin = pParams->m_vViewAABBMin;
	DirectX::XMFLOAT3& vViewMax = pParams->m_vViewAABBMax;

	if ((pBoxMin->x > vViewMax.x) ||
		(pBoxMin->z > vViewMax.z) ||
		(pBoxMax->x < vViewMin.x) ||
		(pBoxMax->z < vViewMin.z))
	{
		return false;
	}

	for (uint32 nPlaneLoop = 0; nPlaneLoop < NUM_CLIPPLANES; ++nPlaneLoop)
	{
		if (GetAABBPlaneSideBack(pParams->m_eAABBPlaneCorner[nPlaneLoop],
			&pParams->m_aClipPlanes[nPlaneLoop], pBoxMin, pBoxMax))
		{
			return false;
		}
	}

	return true;
}

template<typename F>
static void d3d_FilterWorldNodeR(ViewParams* pParams, WorldTreeNode* pNode, F& filterFunc, bool bUpdateFrameCode)
{
	LTLink* pListHead = (LTLink*)&pNode->m_Objects[NOA_Objects];
	for (LTLink* pCurrLink = pListHead->m_pNext; pCurrLink != pListHead; pCurrLink = pCurrLink->m_pNext)
	{		
		LTObject* pObject = (LTObject*)pCurrLink->m_pData;

		if (filterFunc(pObject))
			d3d_CheckAndProcessObject(pObject, bUpdateFrameCode);
	}

	if (pNode->m_apChildren != nullptr)
	{
		for (uint32 dwCurrChild = 0; dwCurrChild < MAX_WTNODE_CHILDREN; dwCurrChild++)
		{
			WorldTreeNode* pChild = pNode->m_apChildren[dwCurrChild];

			if (pChild == nullptr || !pChild->m_dwObjectsOnOrBelow)
				continue;

			/*if (g_CV_CullWorldTree.m_Val)
			{
				if (!d3d_IsWorldNodeVisible(pParams, PLTVECTOR_TO_PXMFLOAT3(&pChild->m_vBBoxMin),
					PLTVECTOR_TO_PXMFLOAT3(&pChild->m_vBBoxMax)))
				{
					continue;
				}
			}*/

			d3d_FilterWorldNodeR(pParams, pChild, filterFunc, bUpdateFrameCode);
		}
	}
}

static void d3d_QueueObjectsInFrustum_WorldTree(ViewParams* pParams)
{
	WorldTree& worldTree = g_pSceneDesc->m_pRenderContext->m_pMainWorld->m_WorldTree;

	auto lambdaFilter = [](LTObject* pObject) 
	{ 
		return !(pObject->m_nObjectType == OT_WORLDMODEL && (pObject->ToWorldModel()->m_pOriginalBsp->m_dwWorldInfoFlags & WIF_PHYSICSBSP)); 
	};

	d3d_FilterWorldNodeR(pParams, &worldTree.m_RootNode, lambdaFilter, true);

	LTLink* pListHead = (LTLink*)&worldTree.m_AlwaysVisObjects;
	for (LTLink* pCurrLink = pListHead->m_pNext; pCurrLink != pListHead; pCurrLink = pCurrLink->m_pNext)
	{
		d3d_CheckAndProcessObject((LTObject*)(pCurrLink->m_pData), true);
	}
}

template<typename F>
static void d3d_IterateBspNode(Node* pNode, F& filterFunc)
{
	if (pNode->m_pSides[0] != nullptr)
		d3d_IterateBspNode(pNode->m_pSides[0], filterFunc);

	if (pNode->m_pSides[1] != nullptr)
		d3d_IterateBspNode(pNode->m_pSides[1], filterFunc);

	LTLink* pListHead = (LTLink*)&pNode->m_Link;
	for (LTLink* pCurrLink = pListHead->m_pNext; pCurrLink != pListHead; pCurrLink = pCurrLink->m_pNext)
	{
		LTObject* pObject = (LTObject*)pCurrLink->m_pData;
		if (filterFunc(pObject))
			d3d_CheckAndProcessObject(pObject, true);
	}
}

static void d3d_QueueObjectsInFrustum_VisNodes(ViewParams* pParams)
{
	WorldBsp* pVisBsp = g_pSceneDesc->m_pRenderContext->m_pMainWorld->GetSpecificBsp(WIF_VISBSP);

	auto lambdaFilter = [](LTObject* pObject) { return (pObject->m_pSrvObjData == nullptr); };

	d3d_IterateBspNode(pVisBsp->m_pRootNode, lambdaFilter);
	
	WorldTree& worldTree = g_pSceneDesc->m_pRenderContext->m_pMainWorld->m_WorldTree;

	LTLink* pListHead = (LTLink*)&worldTree.m_AlwaysVisObjects;
	for (LTLink* pCurrLink = pListHead->m_pNext; pCurrLink != pListHead; pCurrLink = pCurrLink->m_pNext)
	{
		d3d_CheckAndProcessObject((LTObject*)pCurrLink->m_pData, true);
	}
}

static Node* d3d_FindCameraNode(Node* pNode, DirectX::XMFLOAT3* pCameraPos)
{
	if (pNode->m_pSides[0] != nullptr)
	{
		Node* pCameraNode = d3d_FindCameraNode(pNode->m_pSides[0], pCameraPos);
		if (pCameraNode != nullptr)
			return pCameraNode;
	}

	if (pNode->m_pSides[1] != nullptr)
	{
		Node* pCameraNode = d3d_FindCameraNode(pNode->m_pSides[1], pCameraPos);
		if (pCameraNode != nullptr)
			return pCameraNode;
	}

	LTLink* pListHead = (LTLink*)&pNode->m_Link;
	for (LTLink* pCurrLink = pListHead->m_pNext; pCurrLink != pListHead; pCurrLink = pCurrLink->m_pNext)
	{
		LTObject* pObject = (LTObject*)pCurrLink->m_pData;

		if (pObject->m_nObjectType == OT_CAMERA)
		{
			CameraInstance* pCamera = pObject->ToCamera();
			if (pObject->m_vPos.x == pCameraPos->x &&
				pObject->m_vPos.y == pCameraPos->y &&
				pObject->m_vPos.z == pCameraPos->z)
			{
				return pNode;
			}
		}
	}

	return nullptr;
}

static void d3d_VisQuery_ObjectCallback(WorldTreeObj* pObj, void* pData)
{
	d3d_CheckAndProcessObject((LTObject*)pObj, false);
}

static void d3d_VisQuery_AddObjects(LTLink* pLink, LTObject*** pppBuffer, uint32* pCounter)
{
	LTLink* pCurrLink = pLink->m_pPrev;
	while (pLink != pCurrLink)
	{
		LTObject* pObject = (LTObject*)pCurrLink->m_pData;

		if (pObject->m_pSrvObjData == nullptr &&
			g_ObjectHandlers[pObject->m_nObjectType].m_pProcessObjectFn &&
			pObject->m_nObjectType != OT_CANVAS && pObject->m_nObjectType != OT_POLYGRID &&
			pObject->m_dwFlags & FLAG_VISIBLE && // TODO - portals related flags?
			!(pObject->m_dwFlags2 & FLAG2_SKYOBJECT))
		{
			(*pppBuffer)[*pCounter] = pObject;
			(*pCounter)++;
		}

		pCurrLink = pCurrLink->m_pPrev;
	}
}

static void d3d_QueueObjectsInFrustum_VisQuery(ViewParams* pParams)
{
	WorldTree* pWorldTree = &g_pSceneDesc->m_pRenderContext->m_pMainWorld->m_WorldTree;

	VisQueryRequest request;

	request.m_eObjArray = NOA_Objects;
	request.m_vViewpoint = *PXMFLOAT3_TO_PLTVECTOR(&pParams->m_vPos);
	request.m_fViewRadius = g_CV_VisQueryViewRadius.m_Val;
	request.m_ObjectCB = d3d_VisQuery_ObjectCallback;
	request.m_AddObjects = d3d_VisQuery_AddObjects;
	request.m_pUserData = nullptr;

	WorldTree_VTable* pVTable = (WorldTree_VTable*)pWorldTree->m_pVTable;

	uint32 dwNotUsed = 0;
	pVTable->WorldTree__DoVisQuery(pWorldTree, &dwNotUsed, &request);

	// TODO - polygrids are invisible under certain angles? Canvases are bugged on certain maps?
	WorldBsp* pVisBsp = g_pSceneDesc->m_pRenderContext->m_pMainWorld->GetSpecificBsp(WIF_VISBSP);
	auto lambdaFilter = [](LTObject* pObject) 
		{ 
			return (pObject->m_nObjectType == OT_CANVAS || pObject->m_nObjectType == OT_POLYGRID); 
		};

	d3d_IterateBspNode(pVisBsp->m_pRootNode, lambdaFilter);
}

static void d3d_VisQueryEx_IterateLeaf(Leaf* pLeaf, void* pData)
{
	LTLink* pListHead = (LTLink*)&pLeaf->m_Link;
	for (LTLink* pCurrLink = pListHead->m_pNext; pCurrLink != pListHead; pCurrLink = pCurrLink->m_pNext)
	{
		LeafLinkData* pLeafLinkData = (LeafLinkData*)pCurrLink->m_pData;
		
		if (pLeafLinkData->m_pObject->m_nObjectType == OT_POLYGRID)
			d3d_CheckAndProcessObject(pLeafLinkData->m_pObject, true);
	}
}

static void d3d_VisQueryEx_ObjectCallback(WorldTreeObj* pObj, void* pData)
{
	LTObject* pObject = (LTObject*)pObj;

	if (!(pObject->m_dwInternalFlags & IFLAG_IGNORE_VIS_QUERY) &&
		pObject->m_nObjectType != OT_CANVAS && pObject->m_nObjectType != OT_POLYGRID)
	{
		d3d_CheckAndProcessObject(pObject, true);
	}
}

static void d3d_VisQueryEx_AddObjects(LTLink* pLink, LTObject*** pppBuffer, uint32* pCounter)
{
	LTLink* pCurrLink = pLink->m_pPrev;
	while (pLink != pCurrLink)
	{
		LTObject* pObject = (LTObject*)pCurrLink->m_pData;

		if (pObject->m_pSrvObjData == nullptr &&
			g_ObjectHandlers[pObject->m_nObjectType].m_pProcessObjectFn &&
			pObject->m_nObjectType != OT_CANVAS && pObject->m_nObjectType != OT_POLYGRID &&
			pObject->m_dwFlags & FLAG_VISIBLE && // TODO - portals related flags?
			!(pObject->m_dwFlags2 & FLAG2_SKYOBJECT))
		{
			(*pppBuffer)[*pCounter] = pObject;
			(*pCounter)++;
		}

		pCurrLink = pCurrLink->m_pPrev;
	}
}

static void d3d_QueueObjectsInFrustum_VisQueryEx(ViewParams* pParams)
{
	WorldTree* pWorldTree = &g_pSceneDesc->m_pRenderContext->m_pMainWorld->m_WorldTree;

	VisQueryRequest request;

	request.m_eObjArray = NOA_Objects;
	request.m_vViewpoint = *PXMFLOAT3_TO_PLTVECTOR(&pParams->m_vPos);
	request.m_fViewRadius = g_CV_VisQueryViewRadius.m_Val;
	request.m_LeafCB = d3d_VisQueryEx_IterateLeaf;
	request.m_ObjectCB = d3d_VisQueryEx_ObjectCallback;
	request.m_AddObjects = d3d_VisQueryEx_AddObjects;
	request.m_pUserData = nullptr;

	WorldTree_VTable* pVTable = (WorldTree_VTable*)pWorldTree->m_pVTable;

	//Timer_MeasurementStartMM();

	uint32 dwNotUsed = 0;
	pVTable->WorldTree__DoVisQuery(pWorldTree, &dwNotUsed, &request);

	//AddDebugMessage(0, "WorldTree__DoVisQuery = %f", Timer_MeasurementEndMM());

	auto lambdaFilter = !g_CV_VQE_AlwaysDrawSprites.m_Val ?
		[](LTObject* pObject) { return (pObject->m_nObjectType == OT_CANVAS); } :
		[](LTObject* pObject) { return (pObject->m_nObjectType == OT_SPRITE || pObject->m_nObjectType == OT_CANVAS); };

	d3d_FilterWorldNodeR(pParams, &pWorldTree->m_RootNode, lambdaFilter, false);
}

static void d3d_FindObj_ObjectCallback(WorldTreeObj* pObj, void* pData)
{
	d3d_CheckAndProcessObject((LTObject*)pObj, true);
}

static void d3d_QueueObjectsInFrustum_FindObj(ViewParams* pParams)
{
	WorldTree* pWorldTree = &g_pSceneDesc->m_pRenderContext->m_pMainWorld->m_WorldTree;

	FindObjInfo info;
	info.m_eObjArray = NOA_Objects;
	info.m_vViewpoint = *PXMFLOAT3_TO_PLTVECTOR(&pParams->m_vPos);

	float fRadius = g_CV_VisQueryViewRadius.m_Val;
	info.m_vMin = { -fRadius, -fRadius, fRadius };
	info.m_vMax = { fRadius, fRadius, fRadius };

	info.m_CB = d3d_FindObj_ObjectCallback;
	info.m_pCBUser = nullptr;

	uint32 dwNotUsed = 0;

	WorldTree_VTable* pVTable = (WorldTree_VTable*)pWorldTree->m_pVTable;
	pVTable->WorldTree__FindObjectsInBox2(pWorldTree, &dwNotUsed, &info);

	LTLink* pListHead = (LTLink*)&pWorldTree->m_AlwaysVisObjects;
	for (LTLink* pCurrLink = pListHead->m_pNext; pCurrLink != pListHead; pCurrLink = pCurrLink->m_pNext)
	{
		d3d_CheckAndProcessObject((LTObject*)(pCurrLink->m_pData), true);
	}
}

static void d3d_DrawBsp(ViewParams* pParams)
{
	MainWorld* pMainWorld = g_pSceneDesc->m_pRenderContext->m_pMainWorld;

	DirectX::XMVECTOR vParamsPos = DirectX::XMLoadFloat3(&pParams->m_vPos);
	DirectX::XMVECTOR vParamsForward = DirectX::XMLoadFloat3(&pParams->m_vForward);

	for (uint32 i = 0; i < pMainWorld->GetWorldModelCount(); i++)
	{
		WorldBsp* pBsp = pMainWorld->GetWorldModelData(i)->m_pOriginalBsp;

		DirectX::XMFLOAT3 vZero3 = { 0.0f, 0.0f, 0.0f };
		DirectX::XMFLOAT4 vOne4 = { 1.0f, 1.0f, 1.0f, 1.0f };
		
		if ((pBsp->m_dwWorldInfoFlags & WIF_PHYSICSBSP) || (pBsp->m_dwWorldInfoFlags & WIF_TERRAIN))
			d3d_DrawWorldModel(pParams, pBsp, &g_mIdentity, 0, 0, &vZero3, &vOne4, BLEND_STATE_Default, 0);
	}
}

void d3d_TagVisibleLeaves(ViewParams* pParams)
{
	g_pCurrViewParams = pParams;
	DirectX::XMFLOAT3& vPos = pParams->m_vPos;

#if defined(DEBUG) || defined(_DEBUG)
	if (g_CV_VisBoxTest.m_Val)
	{
		float fRadius = g_CV_VisQueryViewRadius.m_Val;

		g_vCameraPosVisBoxMin = { vPos.x - fRadius, vPos.y - fRadius, vPos.z - fRadius };
		g_vCameraPosVisBoxMax = { vPos.x + fRadius, vPos.y + fRadius, vPos.z + fRadius };
	}
#endif

	g_RenderWorld.StartFrame(pParams);
	
	if(!g_CV_LockPVS)
	{
		g_VisibleSet.ClearSet();

		switch (g_GlobalMgr.GetQueueObjectsMode())
		{
			case QOM_VisQuery: d3d_QueueObjectsInFrustum_VisQuery(pParams); break;

			case QOM_VisQueryEx: d3d_QueueObjectsInFrustum_VisQueryEx(pParams); break;

			default:
			case QOM_VisNodes: d3d_QueueObjectsInFrustum_VisNodes(pParams); break;
		}		
	}

	d3d_UploadDynamicLights();

#ifdef WORLD_TWEAKS_ENABLED
	uint32& dwWorldTweakFlags = g_GlobalMgr.GetWorldTweakMgr()->m_dwAllFlags;

	if (g_CV_DrawWorld)
	{
		g_GlobalMgr.GetWorldTweakMgr()->InitFrameParams(&vPos);

		if (!(dwWorldTweakFlags & WORLD_TWEAK_HIDE_SKY))
			g_RenderWorld.DrawSkyExtents(pParams);

		if (g_CV_SkyPortalHack.m_Val)
			d3d_DrawSkyPortals(pParams, &g_mIdentity);

		if (!(dwWorldTweakFlags & WORLD_TWEAK_HIDE_BSP))
			d3d_DrawBsp(pParams);
	}
#else
	if (g_CV_DrawWorld)
	{
		g_RenderWorld.DrawSkyExtents(pParams);

		if (g_CV_SkyPortalHack.m_Val > 1)
			d3d_DrawSkyPortals(pParams, &g_mIdentity);

		d3d_DrawBsp(pParams);
	}
#endif
}

template<typename F>
static void d3d_IterateBspNodeForCaching(Node* pNode, F& filterFunc)
{
	if (pNode->m_pSides[0] != nullptr)
		d3d_IterateBspNodeForCaching(pNode->m_pSides[0], filterFunc);

	if (pNode->m_pSides[1] != nullptr)
		d3d_IterateBspNodeForCaching(pNode->m_pSides[1], filterFunc);

	LTLink* pListHead = (LTLink*)&pNode->m_Link;
	for (LTLink* pCurrLink = pListHead->m_pNext; pCurrLink != pListHead; pCurrLink = pCurrLink->m_pNext)
	{
		LTObject* pObject = (LTObject*)pCurrLink->m_pData;
		if (filterFunc(pObject))
		{
			if (pObject->m_nObjectType == OT_WORLDMODEL)
				d3d_CacheWorldModel(pObject);
			if (pObject->m_nObjectType == OT_MODEL)
				d3d_CacheModel(pObject);

			if (pObject->m_pAttachments != nullptr)
			{
				Attachment* pCurr = pObject->m_pAttachments;
				while (pCurr)
				{
					LTObject* pAttachedObject = g_pStruct->ProcessAttachment(pObject, pCurr);

					if (pAttachedObject != nullptr)
					{			
						if (filterFunc(pAttachedObject) && pAttachedObject->m_nObjectType == OT_MODEL)
							d3d_CacheModel(pAttachedObject);
					}

					pCurr = pCurr->m_pNext;
				}
			}
		}
	}
}

void d3d_CacheObjects()
{
	WorldBsp* pVisBsp = nullptr;
	MainWorld* pMainWorld = g_pSceneDesc->m_pRenderContext->m_pMainWorld;

	for (uint32 i = 0; i < pMainWorld->GetWorldModelCount(); i++)
	{
		WorldBsp* pBsp = pMainWorld->GetWorldModelData(i)->m_pOriginalBsp;

		if (!(pBsp->m_dwWorldInfoFlags & WIF_VISBSP))
		{
			if ((pBsp->m_dwWorldInfoFlags & WIF_PHYSICSBSP) || (pBsp->m_dwWorldInfoFlags & WIF_TERRAIN))
				d3d_CacheWorldModel(pBsp);
		}
		else
		{
			pVisBsp = pBsp;
		}
	}

	if (pVisBsp != nullptr)
	{
		auto lambdaFilter = [](LTObject* pObject)
			{
				return (pObject->m_nObjectType == OT_WORLDMODEL || pObject->m_nObjectType == OT_MODEL ||
					pObject->m_pSrvObjData == nullptr || !(pObject->m_dwFlags & FLAG_REALLYCLOSE));
			};

		d3d_IterateBspNodeForCaching(pVisBsp->m_pRootNode, lambdaFilter);
	}
}
