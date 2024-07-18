#include "pch.h"

#include "draw_light.h"
#include "rendererconsolevars.h"
#include "d3d_mathhelpers.h"
#include "tagnodes.h"
#include "d3d_shader_base.h"
#include "globalmgr.h"
#include "common_draw.h"
#include "3d_ops.h"

using namespace DirectX;

Array_ModelShadowLight g_aModelShadowLight;

static bool CastRayAtGround_Original(DirectX::XMFLOAT3* pFrom, float fHeight, DirectX::XMFLOAT3* pResult)
{
	IntersectQuery iQuery;
	IntersectInfo iInfo;

	iQuery.m_vFrom = *PXMFLOAT3_TO_PLTVECTOR(pFrom);
	iQuery.m_vTo =
	{
		pFrom->x, pFrom->y - fHeight, pFrom->z
	};

	iQuery.m_dwFlags = INTERSECT_HPOLY | IGNORE_NONSOLID;

	MainWorld* pMainWorld = g_pSceneDesc->m_pRenderContext->m_pMainWorld;

	if (g_GlobalMgr.GetEngineHacks()->i_IntersectSegment(&iQuery, &iInfo, &pMainWorld->m_WorldTree))
	{
		WorldPoly* pPoly = pMainWorld->GetPolyFromHPoly(iInfo.m_hPoly);
		//if (pPoly == nullptr)
		//	return false;

		if (!(pPoly->m_pSurface->m_dwFlags & SURF_SKY))
		{
			*pResult = *PLTVECTOR_TO_PXMFLOAT3(&iInfo.m_vPoint);
			return true;
		}
		else
		{
			return false;
		}
	}

	*pResult = *PLTVECTOR_TO_PXMFLOAT3(&iQuery.m_vTo);
	return true;
}

void d3d_LightLookup_Internal(LightTable* pTable, DirectX::XMFLOAT3* pPos, DirectX::XMFLOAT3* pColor)
{
	DirectX::XMVECTOR vSamplePtTemp = (DirectX::XMLoadFloat3(pPos) -
		DirectX::XMLoadFloat3(PLTVECTOR_TO_PXMFLOAT3(&pTable->m_vLookupStart))) *
		DirectX::XMLoadFloat3(PLTVECTOR_TO_PXMFLOAT3(&pTable->m_vReciBlockSize));

	DirectX::XMFLOAT3 vSamplePt;
	DirectX::XMStoreFloat3(&vSamplePt, vSamplePtTemp);

	DirectX::XMINT3 vGridCoords =
	{
		LTCLAMP((int)vSamplePt.x, 0, pTable->m_anLookupSizeMinusOne[0]),
		LTCLAMP((int)vSamplePt.y, 0, pTable->m_anLookupSizeMinusOne[1]),
		LTCLAMP((int)vSamplePt.z, 0, pTable->m_anLookupSizeMinusOne[2])
	};

	DirectX::XMFLOAT3 vNormSamplePt;
	DirectX::XMFLOAT3 vRevNormSamplePt;

	if (vGridCoords.x == pTable->m_anLookupSizeMinusOne[0])
	{
		vNormSamplePt.x = 0.0f;
		vRevNormSamplePt.x = 1.0f;
		vGridCoords.x--;
	}
	else
	{
		vNormSamplePt.x = vSamplePt.x - floorf(vSamplePt.x);
		vRevNormSamplePt.x = 1.0f - vNormSamplePt.x;
	}

	if (vGridCoords.y == pTable->m_anLookupSizeMinusOne[1])
	{
		vNormSamplePt.y = 0.0f;
		vRevNormSamplePt.y = 1.0f;
		vGridCoords.y--;
	}
	else
	{
		vNormSamplePt.y = vSamplePt.y - floorf(vSamplePt.y);
		vRevNormSamplePt.y = 1.0f - vNormSamplePt.y;
	}

	if (vGridCoords.z == pTable->m_anLookupSizeMinusOne[2])
	{
		vNormSamplePt.z = 0.0f;
		vRevNormSamplePt.z = 1.0f;
		vGridCoords.z--;
	}
	else
	{
		vNormSamplePt.z = vSamplePt.z - floorf(vSamplePt.z);
		vRevNormSamplePt.z = 1.0f - vNormSamplePt.z;
	}

	DirectX::XMFLOAT3 vSamples[8];
	uint32 dwLookupSizeX = pTable->m_adwLookupSize[0];
	uint32 dwStep = sizeof(LTRGBColor) * dwLookupSizeX;

	uint8* pBase = (uint8*)&pTable->m_pLookup[vGridCoords.x +
		vGridCoords.y * dwLookupSizeX +
		vGridCoords.z * pTable->m_dwXSizeTimesYSize];

	vSamples[0] = { (float)pBase[dwStep + 2], (float)pBase[dwStep + 1], (float)pBase[dwStep] };
	vSamples[1] = { (float)pBase[dwStep + 6], (float)pBase[dwStep + 5], (float)pBase[dwStep + 4] };
	vSamples[2] = { (float)pBase[2], (float)pBase[1], (float)pBase[0] };
	vSamples[3] = { (float)pBase[6], (float)pBase[5], (float)pBase[4] };

	pBase = (uint8*)&pBase[sizeof(LTRGBColor) * pTable->m_dwXSizeTimesYSize];

	vSamples[4] = { (float)pBase[dwStep + 2], (float)pBase[dwStep + 1], (float)pBase[dwStep] };
	vSamples[5] = { (float)pBase[dwStep + 6], (float)pBase[dwStep + 5], (float)pBase[dwStep + 4] };
	vSamples[6] = { (float)pBase[2], (float)pBase[1], (float)pBase[0] };
	vSamples[7] = { (float)pBase[6], (float)pBase[5], (float)pBase[4] };

	float v17 = (vSamples[1].z * vNormSamplePt.y + vSamples[3].z * vRevNormSamplePt.y) * vNormSamplePt.x +
		(vSamples[0].z * vNormSamplePt.y + vSamples[2].z * vRevNormSamplePt.y) * vRevNormSamplePt.x;
	float v49 = (vSamples[1].y * vNormSamplePt.y + vSamples[3].y * vRevNormSamplePt.y) * vNormSamplePt.x +
		(vSamples[2].y * vRevNormSamplePt.y + vSamples[0].y * vNormSamplePt.y) * vRevNormSamplePt.x;
	float v50 = (vSamples[1].x * vNormSamplePt.y + vSamples[3].x * vRevNormSamplePt.y) * vNormSamplePt.x +
		(vSamples[0].x * vNormSamplePt.y + vSamples[2].x * vRevNormSamplePt.y) * vRevNormSamplePt.x;

	float v52 = (vSamples[5].z * vNormSamplePt.y + vSamples[7].z * vRevNormSamplePt.y) * vNormSamplePt.x +
		(vSamples[4].z * vNormSamplePt.y + vSamples[6].z * vRevNormSamplePt.y) * vRevNormSamplePt.x;
	float v54 = (vSamples[5].y * vNormSamplePt.y + vSamples[7].y * vRevNormSamplePt.y) * vNormSamplePt.x +
		(vSamples[4].y * vNormSamplePt.y + vSamples[6].y * vRevNormSamplePt.y) * vRevNormSamplePt.x;
	float v55 = (vSamples[5].x * vNormSamplePt.y + vSamples[7].x * vRevNormSamplePt.y) * vNormSamplePt.x +
		(vSamples[4].x * vNormSamplePt.y + vSamples[6].x * vRevNormSamplePt.y) * vRevNormSamplePt.x;

	pColor->z = (v52 * vNormSamplePt.z + v17 * vRevNormSamplePt.z) * MATH_ONE_OVER_255;
	pColor->y = (v54 * vNormSamplePt.z + v49 * vRevNormSamplePt.z) * MATH_ONE_OVER_255;
	pColor->x = (v55 * vNormSamplePt.z + v50 * vRevNormSamplePt.z) * MATH_ONE_OVER_255;

	// TODO - omg?
	/*float v32 = vSamples[2].z * vRevNormSamplePt.y;
	float v36 = vSamples[2].x * vRevNormSamplePt.y;
	float v33 = vSamples[0].z * vNormSamplePt.y + v32;
	float v43 = vSamples[3].x * vRevNormSamplePt.y;
	float v38 = vSamples[1].z * vNormSamplePt.y + vSamples[3].z * vRevNormSamplePt.y;
	float v40 = vSamples[1].y * vNormSamplePt.y + vSamples[3].y * vRevNormSamplePt.y;
	float v44 = vSamples[1].x * vNormSamplePt.y + v43;
	float v47 = v33 * vRevNormSamplePt.x;
	float v48 = (vSamples[2].y * vRevNormSamplePt.y + vSamples[0].y * vNormSamplePt.y) * vRevNormSamplePt.x;
	float v17 = v38 * vNormSamplePt.x + v47;
	float v49 = v40 * vNormSamplePt.x + v48;
	float v50 = v44 * vNormSamplePt.x + (vSamples[0].x * vNormSamplePt.y + v36) * vRevNormSamplePt.x;

	float v37 = vSamples[6].x * vRevNormSamplePt.y;
	float v34 = vSamples[4].z * vNormSamplePt.y + vSamples[6].z * vRevNormSamplePt.y;
	float v41 = vSamples[7].y * vRevNormSamplePt.y;
	float v45 = vSamples[7].x * vRevNormSamplePt.y;
	float v39 = vSamples[5].z * vNormSamplePt.y + vSamples[7].z * vRevNormSamplePt.y;
	float v42 = vSamples[5].y * vNormSamplePt.y + v41;
	float v46 = vSamples[5].x * vNormSamplePt.y + v45;
	float v51 = v34 * vRevNormSamplePt.x;
	float v53 = (vSamples[4].y * vNormSamplePt.y + vSamples[6].y * vRevNormSamplePt.y) * vRevNormSamplePt.x;
	float v52 = v39 * vNormSamplePt.x + v51;
	float v54 = v42 * vNormSamplePt.x + v53;
	float v55 = v46 * vNormSamplePt.x + (vSamples[4].x * vNormSamplePt.y + v37) * vRevNormSamplePt.x;
	float v27 = v49 * vRevNormSamplePt.z;
	float v30 = v50 * vRevNormSamplePt.z;
	float v28 = v54 * vNormSamplePt.z + v27;
	float v31 = v55 * vNormSamplePt.z + v30;

	pColor->z = (v52 * vNormSamplePt.z + v17 * vRevNormSamplePt.z);
	pColor->y = v28;
	pColor->x = v31;*/
}

void d3d_LightLookup_Original(LightTable* pTable, DirectX::XMFLOAT3* pPos, DirectX::XMFLOAT3* pColor)
{
	LTRGBColor dwRGB;
	g_GlobalMgr.GetEngineHacks()->w_DoLightLookup(pTable, PXMFLOAT3_TO_PLTVECTOR(pPos), &dwRGB);

	pColor->x = (float)dwRGB.rgb.r * MATH_ONE_OVER_255;
	pColor->y = (float)dwRGB.rgb.g * MATH_ONE_OVER_255;
	pColor->z = (float)dwRGB.rgb.b * MATH_ONE_OVER_255;
}

void d3d_CalcLightAdd(DirectX::XMFLOAT3* pPos, DirectX::XMFLOAT3* pLightAdd, uint32 dwMaxLights)
{
	uint32 dwCounter = 0;
	AllocSet* pSet = d3d_GetVisibleSet()->GetLights();
	
	for (uint32 i = 0; i < pSet->GetObjectCount(); i++)
	{
		DynamicLight* pLight = pSet->GetObjectByIndex(i)->ToDynamicLight();

		if (pLight->m_Base.m_dwFlags & FLAG_ONLYLIGHTWORLD)
			continue;

		//if (!g_CV_BlackDynamicLight.m_Val && pLight->IsBlackLight())
		//	continue;

		float fDistSqr = DirectX::XMVectorGetX(
			DirectX::XMVector3LengthSq(DirectX::XMLoadFloat3(pPos) -
			DirectX::XMLoadFloat3(PLTVECTOR_TO_PXMFLOAT3(&pLight->m_Base.m_vPos))));
	
		if (fDistSqr < (pLight->m_fLightRadius * pLight->m_fLightRadius))
		{
			float fPercent = 1.0f - ((float)sqrt(fDistSqr) / pLight->m_fLightRadius);
			fPercent *= 0.7f;

			int nRed = pLight->m_Base.m_nColorR;
			int nGreen = pLight->m_Base.m_nColorB;
			int nBlue = pLight->m_Base.m_nColorG;

			// TODO - what's better?
			//pLightAdd->x += (float)(nRed - (255 - nRed)) * fPercent * MATH_ONE_OVER_255;
			//pLightAdd->y += (float)(nGreen - (255 - nGreen)) * fPercent * MATH_ONE_OVER_255;
			//pLightAdd->z += (float)(nBlue - (255 - nBlue)) * fPercent * MATH_ONE_OVER_255;

			pLightAdd->x += (float)nRed * fPercent * MATH_ONE_OVER_255;
			pLightAdd->y += (float)nGreen * fPercent * MATH_ONE_OVER_255;
			pLightAdd->z += (float)nBlue * fPercent * MATH_ONE_OVER_255;

			dwCounter++;
			if (dwCounter == dwMaxLights)
				return;
		}
 	}
}

void d3d_ProcessLight(LTObject* pObj)
{
	if (!g_CV_DynamicLight.m_Val)
		return;

	AllocSet* pSet = d3d_GetVisibleSet()->GetLights();

	DynamicLight* pLight = pObj->ToDynamicLight();
	
	if (pLight->m_fLightRadius > 0.0f && pSet->GetObjectCount() < MAX_LIGHTS_PER_BUFF)
		d3d_GetVisibleSet()->GetLights()->Add(pObj);		
}

void d3d_ProcessModelShadowLight(ModelInstance* pInstance)
{
	if (!g_CV_MaxModelShadows.m_Val || !(pInstance->m_Base.m_dwFlags & FLAG_SHADOW) || !pInstance->GetCachedTransformCount())
		return;

	auto lambdaFind = [pInstance](ModelShadowLight& shadowLight) { return shadowLight.m_pInstance == pInstance; };
	Array_ModelShadowLight::iterator iter = std::find_if(g_aModelShadowLight.begin(), g_aModelShadowLight.end(), 
		lambdaFind);

	if (iter != g_aModelShadowLight.end())
		return;

	DirectX::XMFLOAT3 vInstancePos;
	DirectX::XMFLOAT3 vShadowPos;
	DirectX::XMFLOAT4X4 mTransform;

	LTObject* pObject = &pInstance->m_Base;
	d3d_SetupTransformation(&pObject->m_vPos, (float*)&pObject->m_rRot, &pObject->m_vScale, &mTransform);

	Model* pModel = pInstance->GetModel();
	uint32 dwRootNode = pModel->m_pRootNode->m_wNodeIndex;

	if (pModel->GetNode(dwRootNode)->GetChildCount() > 0)
	{
		uint32 dwFirstChild = pModel->GetNode(dwRootNode)->GetChild(0)->m_wNodeIndex;
		DirectX::XMFLOAT4X4* pTransform = PLTMATRIX_TO_PXMFLOAT4X4(pInstance->GetCachedTransform(dwFirstChild));

		DirectX::XMFLOAT3 vNodePos;
		Matrix_GetTranslationLT(pTransform, &vNodePos);

		DirectX::XMStoreFloat3(&vInstancePos, DirectX::XMVector4Transform(
			DirectX::XMVectorSet(vNodePos.x, vNodePos.y, vNodePos.z, 1.0f), 
			DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&mTransform))));
	}
	else
	{
		vInstancePos = *PLTVECTOR_TO_PXMFLOAT3(&pInstance->m_Base.m_vPos);
	}

	if (CastRayAtGround_Original(&vInstancePos, pInstance->m_Base.m_vDims.y * pInstance->m_Base.m_vScale.y, &vShadowPos))
	{				
		//vShadowPos.y -= 10.0f;
		g_aModelShadowLight.emplace_back(pInstance, &vShadowPos,
			pInstance->m_Base.m_vDims.x * pInstance->m_Base.m_vScale.x * g_CV_ModelShadowLightScale.m_Val);
	}
}