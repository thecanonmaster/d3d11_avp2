#include "pch.h"

#include "d3d_renderworld_occluder.h"
#include "d3d_viewparams.h"

using namespace DirectX;

COccluder::COccluder()
{
	m_PolyPlane = { };
	m_ePolyPlaneCorner = AABB_None;
}

COccluder::~COccluder()
{
	m_aEdgePlanes.clear();
}

void COccluder::AddPlane(XMPLANE* pScreenPlane)
{
	m_aEdgePlanes.push_back(*pScreenPlane);
}

void COccluder_Frustum::InitFrustum(ViewParams* pParams)
{
	Init();

	const uint32 dwNumScreenPts = 4;
	DirectX::XMFLOAT3 avScreenPts[dwNumScreenPts];
	avScreenPts[0] = pParams->m_avViewPoints[2];
	avScreenPts[1] = pParams->m_avViewPoints[3];
	avScreenPts[2] = pParams->m_avViewPoints[1];
	avScreenPts[3] = pParams->m_avViewPoints[0];

	m_aEdgePlanes.reserve(dwNumScreenPts);
	m_aWorldEdgePlanes.reserve(dwNumScreenPts);

	m_PolyPlane = pParams->m_aClipPlanes[CPLANE_NEAR_INDEX];
	m_ePolyPlaneCorner = GetAABBPlaneCorner(&m_PolyPlane.m_vNormal);

	DirectX::XMFLOAT3 vPrevWorld = avScreenPts[3];
	DirectX::XMFLOAT3 vPrevScr;

	Matrix_VMul_H(&vPrevScr, &pParams->m_mFullTransform, &vPrevWorld);

	for (DirectX::XMFLOAT3* pCurVert = avScreenPts; pCurVert != &avScreenPts[dwNumScreenPts]; pCurVert++)
	{
		DirectX::XMFLOAT3& vNextWorld = *pCurVert;
		DirectX::XMFLOAT3 vNextScr = { };

		Matrix_VMul_H(&vNextScr, &pParams->m_mFullTransform, &vNextWorld);

		float fXDiff = (vNextScr.x - vPrevScr.x);
		float fYDiff = (vNextScr.y - vPrevScr.y);

		if ((fXDiff * fXDiff + fYDiff * fYDiff) > 0.001f)
		{
			XMPLANE screenPlane;
			DirectX::XMVECTOR vEdgeScr = DirectX::XMLoadFloat3(&vNextScr) - DirectX::XMLoadFloat3(&vPrevScr);
			DirectX::XMVECTOR vScreenPlaneNormal = DirectX::XMVector3Normalize(
				DirectX::XMVectorSet(DirectX::XMVectorGetY(vEdgeScr), -DirectX::XMVectorGetX(vEdgeScr), 0.0f, 0.0f));

			DirectX::XMStoreFloat3(&screenPlane.m_vNormal, vScreenPlaneNormal);

			screenPlane.m_fDist = screenPlane.m_vNormal.x * vNextScr.x + screenPlane.m_vNormal.y * vNextScr.y;

			XMPLANE worldPlane;
			DirectX::XMVECTOR vNextWorldTemp = DirectX::XMLoadFloat3(&vNextWorld);
			DirectX::XMVECTOR vParamsPosTemp = DirectX::XMLoadFloat3(&pParams->m_vPos);

			DirectX::XMVECTOR vEdgeWorld = vNextWorldTemp - DirectX::XMLoadFloat3(&vPrevWorld);
			DirectX::XMVECTOR vWorldPlaneNormal = DirectX::XMVector3Normalize(
				DirectX::XMVector3Cross(vNextWorldTemp - vParamsPosTemp, vEdgeWorld)); // LTVector cross product is reversed

			DirectX::XMStoreFloat3(&worldPlane.m_vNormal, vWorldPlaneNormal);
			
			worldPlane.m_fDist = DirectX::XMVectorGetX(DirectX::XMVector3Dot(vWorldPlaneNormal, vNextWorldTemp));

			AddPlane(&screenPlane, &worldPlane);
		}

		vPrevWorld = vNextWorld;
		vPrevScr = vNextScr;
	}
}

void COccluder_Frustum::AddPlane(XMPLANE* pScreenPlane, XMPLANE* pWorldPlane)
{ 
	COccluder::AddPlane(pScreenPlane);

	m_aWorldEdgePlanes.push_back(*pWorldPlane);
	m_aeEdgeCorners.push_back(GetAABBPlaneCorner(&pWorldPlane->m_vNormal));
}

PolySide COccluder_Frustum::ClassifyAABB(DirectX::XMFLOAT3* pMin, DirectX::XMFLOAT3* pMax, float fFarZ)
{
	uint32 dwFront = 0;

	if (m_ePolyPlaneCorner == AABB_None)
		return PS_FrontSide;

	PolySide eCurSide = GetAABBPlaneSide(m_ePolyPlaneCorner, &m_PolyPlane, pMin, pMax);

	if (eCurSide == PS_BackSide)
		return PS_BackSide;

	if (eCurSide == PS_FrontSide)
		dwFront++;

	XMPLANE newPlane = { m_PolyPlane.m_vNormal, m_PolyPlane.m_fDist + fFarZ };
	eCurSide = GetAABBPlaneSide(m_ePolyPlaneCorner, &newPlane, pMin, pMax);

	if (eCurSide == PS_FrontSide)
		return PS_BackSide;

	Array_XMPLANE::iterator iterCurrPlane = m_aWorldEdgePlanes.begin();
	Array_AABCorner::iterator iterCurrCorner = m_aeEdgeCorners.begin();

	for (; iterCurrPlane != m_aWorldEdgePlanes.end(); ++iterCurrPlane, ++iterCurrCorner)
	{
		eCurSide = GetAABBPlaneSide(*iterCurrCorner, &*iterCurrPlane, pMin, pMax);

		if (eCurSide == PS_BackSide)
			return PS_BackSide;

		if (eCurSide == PS_FrontSide)
			dwFront++;
	}

	if (dwFront == (m_aWorldEdgePlanes.size() + 1))
		return PS_FrontSide;

	return PS_Intersect;
}