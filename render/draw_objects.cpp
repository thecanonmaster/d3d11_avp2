#include "pch.h"

#include "draw_objects.h"
#include "common_stuff.h"
#include "d3d_mathhelpers.h"
#include "draw_model.h"
#include "draw_worldmodel.h"
#include "draw_sprite.h"
#include "draw_particles.h"
#include "draw_canvas.h"
#include "draw_linesystem.h"
#include "draw_polygrid.h"
#include "draw_light.h"
#include "d3d_viewparams.h"
#include "d3d_draw.h"
#include "rendererconsolevars.h"
#include "renderstatemgr.h"
#include "setup_model.h"
#include "globalmgr.h"

using namespace DirectX;

static DirectX::XMFLOAT3 d3d_GetDims_Generic(LTObject* pObject)
{
	return *PLTVECTOR_TO_PXMFLOAT3(&pObject->m_vDims);
}

static DirectX::XMFLOAT3 d3d_GetDims_Model(LTObject* pObject)
{
	ModelInstance* pModelInstance = pObject->ToModel();
	Model* pModel = pModelInstance->GetModel();

	// TODO - global radius not vis radius?
	float fVisRadius = LTMAX(pModel->m_fVisRadius, pModel->m_fGlobalRadius);

	DirectX::XMFLOAT3 vResult =
	{
		pModelInstance->m_Base.m_vScale.x * fVisRadius,
		pModelInstance->m_Base.m_vScale.y * fVisRadius,
		pModelInstance->m_Base.m_vScale.z * fVisRadius,
	};

	return vResult;
}

static DirectX::XMFLOAT3 d3d_GetDims_ParticleSystem(LTObject* pObject)
{
	ParticleSystem* pSystem = pObject->ToParticleSystem();

	DirectX::XMVECTOR vPos = DirectX::XMLoadFloat3(PLTVECTOR_TO_PXMFLOAT3(&pSystem->m_Base.m_vPos));
	DirectX::XMVECTOR vSystemCenter = DirectX::XMLoadFloat3(PLTVECTOR_TO_PXMFLOAT3(&pSystem->m_vSystemCenter));

	float fDistToSystemCenter = DirectX::XMVectorGetX(DirectX::XMVector3Length(vPos - vSystemCenter));

	DirectX::XMVECTOR vScale = DirectX::XMLoadFloat3(PLTVECTOR_TO_PXMFLOAT3(&pSystem->m_Base.m_vScale));
	DirectX::XMVECTOR vDims = vScale * (pSystem->m_fSystemRadius + fDistToSystemCenter);

	DirectX::XMFLOAT3 vResult;
	DirectX::XMStoreFloat3(&vResult, vDims);

	return vResult;
}

static DirectX::XMFLOAT3 d3d_GetDims_PolyGrid(LTObject* pObject)
{
	DirectX::XMFLOAT3 vResultFinal;
	PolyGrid* pGrid = pObject->ToPolyGrid();
	
	if (Rotation_IsIdentity(PLTROTATION_TO_PXMFLOAT4(&pObject->m_rRot)))
	{
		vResultFinal =
		{
			pObject->m_vScale.x * pGrid->m_dwWidth,
			pObject->m_vScale.y * 2.0f,
			pObject->m_vScale.z * pGrid->m_dwHeight
		};

		return vResultFinal;
	}

	DirectX::XMFLOAT3 vUp;
	Rotation_Up(PLTROTATION_TO_PXMFLOAT4(&pObject->m_rRot), &vUp);

	DirectX::XMFLOAT3 vRight;
	Rotation_Right(PLTROTATION_TO_PXMFLOAT4(&pObject->m_rRot), &vRight);

	DirectX::XMFLOAT3 vForward;
	Rotation_Forward(PLTROTATION_TO_PXMFLOAT4(&pObject->m_rRot), &vForward);

	static const float anSign[] = { -1.0f, 1.0f };

	DirectX::XMVECTOR vResult = DirectX::XMVectorZero();

	for (uint32 dwCurrPt = 0; dwCurrPt < 8; dwCurrPt++)
	{
		DirectX::XMVECTOR vTempRight = DirectX::XMLoadFloat3(&vRight) * 
			(pObject->m_vScale.x * anSign[(dwCurrPt / 1) % 2]);
		DirectX::XMVECTOR vTempUp = DirectX::XMLoadFloat3(&vUp) * 
			(pObject->m_vScale.y * anSign[(dwCurrPt / 2) % 2]);
		DirectX::XMVECTOR vTempForward = DirectX::XMLoadFloat3(&vForward) * 
			(pObject->m_vScale.z * anSign[(dwCurrPt / 4) % 2]);

		DirectX::XMVECTOR vPt = vTempRight + vTempUp + vTempForward;

		//DirectX::XMVectorSetX(vPt, fabsf(DirectX::XMVectorGetX(vPt)));
		//DirectX::XMVectorSetY(vPt, fabsf(DirectX::XMVectorGetY(vPt)));
		//DirectX::XMVectorSetZ(vPt, fabsf(DirectX::XMVectorGetZ(vPt)));

		vPt = DirectX::XMVectorAbs(vPt);
		DirectX::XMVectorMax(vResult, vPt);
	}

	DirectX::XMStoreFloat3(&vResultFinal, vResult);

	return vResultFinal;
}

ObjectHandler g_ObjectHandlers[NUM_OBJECT_TYPES] =
{
	OT_NORMAL, nullptr, nullptr, false, nullptr, nullptr, false, d3d_GetDims_Generic,
	OT_MODEL, nullptr, nullptr, false, d3d_ModelPreFrame, d3d_ProcessModel, true, d3d_GetDims_Model,
	OT_WORLDMODEL, nullptr, nullptr, false, nullptr, d3d_ProcessWorldModel, true, d3d_GetDims_Generic,
	OT_SPRITE, nullptr, nullptr, false, nullptr, d3d_ProcessSprite, false, d3d_GetDims_Generic,
	OT_LIGHT, nullptr, nullptr, false, nullptr, d3d_ProcessLight, false, d3d_GetDims_Generic,
	OT_CAMERA, nullptr, nullptr, false , nullptr, nullptr, false, d3d_GetDims_Generic,
	OT_PARTICLESYSTEM, nullptr, nullptr, false, nullptr, d3d_ProcessParticles, true, d3d_GetDims_ParticleSystem,
	OT_POLYGRID, nullptr, nullptr, false, nullptr, d3d_ProcessPolyGrid, true, d3d_GetDims_PolyGrid,
	OT_LINESYSTEM, nullptr, nullptr, false, nullptr, d3d_ProcessLineSystem, false, d3d_GetDims_Generic,
	OT_CONTAINER, nullptr, nullptr, false, nullptr, d3d_ProcessWorldModel, false, d3d_GetDims_Generic,
	OT_CANVAS, nullptr, nullptr, false, nullptr, d3d_ProcessCanvas, false, d3d_GetDims_Generic
};

void d3d_InitObjectModules()
{
	for (uint32 i = 0; i < NUM_OBJECT_TYPES; i++)
	{
		if (g_ObjectHandlers[i].m_pModuleInit != nullptr)
			g_ObjectHandlers[i].m_pModuleInit();

		g_ObjectHandlers[i].m_bModuleInitted = true;
	}
}

void d3d_TermObjectModules()
{
	for (uint32 i = 0; i < NUM_OBJECT_TYPES; i++)
	{
		if (g_ObjectHandlers[i].m_pModuleTerm != nullptr && g_ObjectHandlers[i].m_bModuleInitted)
			g_ObjectHandlers[i].m_pModuleTerm();

		g_ObjectHandlers[i].m_bModuleInitted = false;
	}
}

void d3d_InitObjectQueues()
{
	for (uint32 i = 0; i < NUM_OBJECT_TYPES; i++)
	{
		if (g_ObjectHandlers[i].m_pPreFrameFn != nullptr)
			g_ObjectHandlers[i].m_pPreFrameFn();
	}
}

static void d3d_CheckForDuplicateObjectsInSet(AllocSet* pSet, const char* szErrorMsg)
{
	for (uint32 dwCurrObj = 1; dwCurrObj < pSet->GetObjects().size(); dwCurrObj++)
	{
		for (uint32 dwTestObj = 0; dwTestObj < dwCurrObj; dwTestObj++)
		{
			if (pSet->GetObjectByIndex(dwTestObj) == pSet->GetObjectByIndex(dwCurrObj))
				AddConsoleMessage("WARNING! d3d_CheckForDuplicateObjectsInSet: %s!", szErrorMsg);
		}
	}
}

static void d3d_CheckForDuplicateObjects(VisibleSet* pSet)
{
	d3d_CheckForDuplicateObjectsInSet(pSet->GetLineSystems(), "Duplicate line system queued");
	d3d_CheckForDuplicateObjectsInSet(pSet->GetSprites(), "Duplicate translucent sprite queued");
	d3d_CheckForDuplicateObjectsInSet(pSet->GetLights(), "Duplicate light queued");
	d3d_CheckForDuplicateObjectsInSet(pSet->GetParticleSystems(), "Duplicate particle system queued");
	d3d_CheckForDuplicateObjectsInSet(pSet->GetCanvases(), "Duplicate solid canvas queued");
	d3d_CheckForDuplicateObjectsInSet(pSet->GetModels(), "Duplicate solid model queued");
	d3d_CheckForDuplicateObjectsInSet(pSet->GetPolyGrids(), "Duplicate solid polygrid queued");
	d3d_CheckForDuplicateObjectsInSet(pSet->GetWorldModels(), "Duplicate solid world model queued");
	d3d_CheckForDuplicateObjectsInSet(pSet->GetChromakeyModels(), "Duplicate chromakey model queued");
	d3d_CheckForDuplicateObjectsInSet(pSet->GetChromakeyWorldModels(), "Duplicate chromakey world model queued");
	d3d_CheckForDuplicateObjectsInSet(pSet->GetTranslucentModels(), "Duplicate translucent model queued");
	d3d_CheckForDuplicateObjectsInSet(pSet->GetTranslucentWorldModels(), "Duplicate translucent world modelqueued");
	d3d_CheckForDuplicateObjectsInSet(pSet->GetNoZSprites(), "Duplicate No Z sprite queued");
	d3d_CheckForDuplicateObjectsInSet(pSet->GetTranslucentCanvases(), "Duplicate translucent canvas queued");
	d3d_CheckForDuplicateObjectsInSet(pSet->GetTranslucentPolyGrids(), "Duplicate translucent polygrid queued");
}

void d3d_FlushObjectQueues(ViewParams* pParams)
{
	static ObjectDrawList translucentObjList;

#ifndef MODEL_PIECELESS_RENDERING
	g_ModelSetup.QueueAllModelInfo();
#endif

	d3d_DrawSolidWorldModels(pParams);

	d3d_DrawSolidModels(pParams);

	d3d_DrawSolidPolyGrids(pParams);

	d3d_DrawSolidCanvases(pParams);
	g_GlobalMgr.GetCanvasBatchMgr()->RenderBatch();

	d3d_DrawChromakeyWorldModels(pParams);

	d3d_DrawChromakeyModels(pParams);
	
 	d3d_SetTranslucentObjectStates(false);

		d3d_QueueTranslucentParticles(pParams, &translucentObjList);

		d3d_QueueTranslucentPolyGrids(pParams, &translucentObjList);

		d3d_QueueLineSystems(pParams, &translucentObjList);

		d3d_QueueTranslucentWorldModels(pParams, &translucentObjList);

		d3d_QueueTranslucentModels(pParams, &translucentObjList);

		d3d_QueueTranslucentCanvases(pParams, &translucentObjList);

		d3d_QueueTranslucentSprites(pParams, &translucentObjList);

		translucentObjList.Draw(pParams);
		g_GlobalMgr.GetCanvasBatchMgr()->RenderBatch();
		g_GlobalMgr.GetSpriteBatchMgr()->RenderBatch();

		d3d_DrawNoZSprites(pParams);
		g_GlobalMgr.GetSpriteBatchMgr()->RenderBatch();

	d3d_UnsetTranslucentObjectStates(true);

	if (g_CV_ShowRenderedObjectCounts.m_Val)
	{
		VisibleSet* pVisibleSet = d3d_GetVisibleSet();

		if (g_CV_ShowRenderedObjectCounts.m_Val == 2)
			d3d_CheckForDuplicateObjects(pVisibleSet);

		AddConsoleMessage("Line Systems: %d", pVisibleSet->GetLineSystems()->GetObjectCount());
		AddConsoleMessage("No Z Sprites: %d", pVisibleSet->GetNoZSprites()->GetObjectCount());
		AddConsoleMessage("Lights: %d", pVisibleSet->GetLights()->GetObjectCount());
		AddConsoleMessage("Particle Systems: %d", pVisibleSet->GetParticleSystems()->GetObjectCount());
		AddConsoleMessage("Solid Canvases: %d", pVisibleSet->GetCanvases()->GetObjectCount());
		AddConsoleMessage("Solid Models: %d", pVisibleSet->GetModels()->GetObjectCount());
		AddConsoleMessage("Solid Poly Grids: %d", pVisibleSet->GetPolyGrids()->GetObjectCount());
		AddConsoleMessage("Solid World Models: %d", pVisibleSet->GetWorldModels()->GetObjectCount());
		AddConsoleMessage("Translucent Canvases: %d", pVisibleSet->GetTranslucentCanvases()->GetObjectCount());
		AddConsoleMessage("Translucent Models: %d", pVisibleSet->GetTranslucentModels()->GetObjectCount());
		AddConsoleMessage("Translucent Poly Grids: %d", pVisibleSet->GetTranslucentPolyGrids()->GetObjectCount()); //+ pVisibleSet->m_EarlyTranslucentPolyGrids.m_nObjects);
		AddConsoleMessage("Translucent Sprites: %d", pVisibleSet->GetSprites()->GetObjectCount());
		AddConsoleMessage("Translucent World Models: %d", pVisibleSet->GetTranslucentWorldModels()->GetObjectCount());
	}
}

float ObjectDrawList::CalcDistance(LTObject* pObject, ViewParams* pParams)
{
	if (!(pObject->m_dwFlags & FLAG_REALLYCLOSE))
	{
		DirectX::XMVECTOR vTemp = DirectX::XMLoadFloat3(PLTVECTOR_TO_PXMFLOAT3(&pObject->m_vPos)) -
			DirectX::XMLoadFloat3(PLTVECTOR_TO_PXMFLOAT3(&pParams->m_vPos));

		return DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(vTemp));
	}
	else
	{
		return Vector_MagSqr(PLTVECTOR_TO_PXMFLOAT3(&pObject->m_vPos));
	}
}

void ObjectDrawList::Add(ViewParams* pParams, LTObject* pObject, DrawObjectFn pDrawFn)
{
	if (!g_CV_DrawSorted.m_Val)
	{
		pDrawFn(pParams, pObject);
		return;
	}

	//m_ObjectDrawers.push(ObjectDrawer(pObject, pDrawFn, CalcDistance(pObject, pParams)));
	m_ObjectDrawers.emplace(pObject, pDrawFn, CalcDistance(pObject, pParams));
}

void ObjectDrawList::AddEx(ViewParams* pParams, LTObject* pObject, DrawObjectFn pDrawFn, 
	DrawObjectExFn pDrawExFn)
{
	if (!g_CV_DrawSorted.m_Val)
	{
		pDrawFn(pParams, pObject);
		return;
	}

	//m_ObjectDrawers.push(ObjectDrawer(pObject, pDrawFn, pDrawExFn, CalcDistance(pObject, pParams)));
	m_ObjectDrawers.emplace(pObject, pDrawFn, pDrawExFn, CalcDistance(pObject, pParams));
}

void ObjectDrawList::Draw(ViewParams* pParams)
{
	LTObject* pPrevObject = nullptr;
	for (; !m_ObjectDrawers.empty(); m_ObjectDrawers.pop())
	{
		const ObjectDrawer& drawer = m_ObjectDrawers.top();

		/*HRENDERSTATE hPrevBlendState = INVALID_BLEND_STATE;
		if (drawer.GetLTObject()->m_nObjectType == OT_MODEL)
			hPrevBlendState = g_RenderStateMgr.GetBlendState();*/

		LTObject* pCurrObject = drawer.GetLTObject();

		if (drawer.GetDrawExFn() == nullptr)
			drawer.Draw(pParams);
		else
			drawer.DrawEx(pParams, pPrevObject);

		/*if (drawer.GetLTObject()->m_nObjectType == OT_MODEL)
			g_RenderStateMgr.SetBlendState(hPrevBlendState);*/

		pPrevObject = drawer.GetLTObject();
	}
}