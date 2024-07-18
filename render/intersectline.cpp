#include "pch.h"

#include "intersectline.h"
#include "d3d_mathhelpers.h"

using namespace DirectX;

#define INTERSECT_EPSILON	0.01f

static bool InsideConvex(WorldPoly* pPoly, DirectX::XMFLOAT3* pPt)
{
	if (pPoly->m_pSurface->m_dwFlags & SURF_NONEXISTANT)
		return false;

	float fDistSqr = DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(
		DirectX::XMLoadFloat3(PLTVECTOR_TO_PXMFLOAT3(&pPoly->m_vCenter)) - DirectX::XMLoadFloat3(pPt)));
	
	if (fDistSqr > pPoly->m_fRadius * pPoly->m_fRadius)
		return false;

	DirectX::XMFLOAT3* pNormal = PLTVECTOR_TO_PXMFLOAT3(&pPoly->m_pPlane->m_vNormal);
	uint32 dwEnd = pPoly->m_wNumVerts;
	uint32 dwPrev = dwEnd - 1;

	for (uint32 dwCur = 0; dwCur != dwEnd; )
	{
		DirectX::XMFLOAT3* pCurrVec = PLTVECTOR_TO_PXMFLOAT3(&pPoly->m_Vertices[dwCur].m_pPoints->m_vVec);
		DirectX::XMFLOAT3* pPrevVec = PLTVECTOR_TO_PXMFLOAT3(&pPoly->m_Vertices[dwPrev].m_pPoints->m_vVec);
		DirectX::XMVECTOR vTemp = DirectX::XMLoadFloat3(pPrevVec) - DirectX::XMLoadFloat3(pCurrVec);

		DirectX::XMVECTOR vNormal = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(vTemp, 
			DirectX::XMLoadFloat3(pNormal)));

		float fDist = DirectX::XMVectorGetX(DirectX::XMVector3Dot(vNormal, DirectX::XMLoadFloat3(pCurrVec)));

		float fEdgeDot = DirectX::XMVectorGetX(DirectX::XMVector3Dot(vNormal, 
			DirectX::XMLoadFloat3(pPt))) - fDist;

		if (fEdgeDot < -INTERSECT_EPSILON)
			return false;

		dwPrev = dwCur;
		dwCur++;
	}

	return true;
}

bool InternalIntersectLineNode(Node* pRoot, IntersectRequest* pRequest, DirectX::XMFLOAT3 vPoint1,
	DirectX::XMFLOAT3& vPoint2)
{
	while (!(pRoot->m_nFlags & (NF_IN | NF_OUT)))
	{
		float fDot1 = Plane_DistTo(PLTPLANE_TO_PXMPLANE(&pRoot->m_pPoly->m_pPlane), &vPoint1);
		float fDot2 = Plane_DistTo(PLTPLANE_TO_PXMPLANE(&pRoot->m_pPoly->m_pPlane), &vPoint2);

		if (fDot1 > INTERSECT_EPSILON && fDot2 > INTERSECT_EPSILON)
		{
			pRoot = pRoot->m_pSides[PS_FrontSide];
		}
		else if (fDot1 < -INTERSECT_EPSILON && fDot2 < -INTERSECT_EPSILON)
		{
			pRoot = pRoot->m_pSides[PS_BackSide];
		}
		else
		{
			int nSide1;
			DirectX::XMFLOAT3 vIPoint;

			if ((fDot1 < -INTERSECT_EPSILON) || (fDot1 > INTERSECT_EPSILON))
				nSide1 = (int)(fDot1 > 0.0f);
			else
				nSide1 = (int)(fDot2 < 0.0f);

			float fIntersectionT = fDot2 - fDot1;
			bool bOnPlane = false;

			if (fIntersectionT != 0.0f)
				fIntersectionT = -fDot1 / fIntersectionT;
			else
				bOnPlane = true;

			if ((fDot1 < -INTERSECT_EPSILON) || (fDot1 > INTERSECT_EPSILON) || bOnPlane)
			{
				float fBackSideT = fIntersectionT + INTERSECT_EPSILON;
				fBackSideT = LTMIN(fBackSideT, 1.0f);

				Vector_Lerp(&vIPoint, &vPoint1, &vPoint2, fBackSideT);

				if (InternalIntersectLineNode(pRoot->m_pSides[nSide1], pRequest, vPoint1, 
					bOnPlane ? vPoint2 : vIPoint))
				{
					return true;
				}
			}

			if ((nSide1 == PS_FrontSide) && (pRoot->m_pPoly))
			{
				Vector_Lerp(&vIPoint, &vPoint1, &vPoint2, fIntersectionT);

				if (InsideConvex(pRoot->m_pPoly, &vIPoint))
				{
					LTBOOL bDone = LTTRUE;

					XMIntersectQuery* pQuery = pRequest->m_pQuery;
					if (pQuery && pQuery->m_PolyFilterFn && pRequest->m_pWorldBsp)
					{
						if (!pQuery->m_PolyFilterFn(pRequest->m_pWorldBsp->MakeHPoly(pRoot), pQuery->m_pUserData))
						{
							bDone = LTFALSE;
						}
					}

					if (bDone)
					{
						pRequest->m_pNodeHit = pRoot;
						*pRequest->m_pIPos = vIPoint;
						return true;
					}
				}
			}
			else if (fIntersectionT > (1.0f - INTERSECT_EPSILON))
				break;

			float fFontSideT = fIntersectionT - INTERSECT_EPSILON;
			fFontSideT = LTMAX(fFontSideT, 0.0f);

			Vector_Lerp(&vIPoint, &vPoint1, &vPoint2, fFontSideT);
			vPoint1 = vIPoint;

			pRoot = pRoot->m_pSides[!nSide1];
		}
	}

	return false;
}

bool IntersectLineNode(Node* pRoot, IntersectRequest* pRequest)
{
	return InternalIntersectLineNode(pRoot, pRequest, *pRequest->m_pPoints[0], *pRequest->m_pPoints[1]);
}

Node* IntersectLine(Node* pRoot, DirectX::XMFLOAT3* pPoint1, DirectX::XMFLOAT3* pPoint2,
	DirectX::XMFLOAT3* pIPos, XMPLANE* pIPlane)
{
	IntersectRequest req;

	req.m_pPoints[0] = pPoint1;
	req.m_pPoints[1] = pPoint2;
	req.m_pIPos = pIPos;

	if (IntersectLineNode(pRoot, &req))
	{
		*pIPlane = *PLTPLANE_TO_PXMPLANE(req.m_pNodeHit->m_pPoly->m_pPlane);
		return req.m_pNodeHit;
	}

	return nullptr;
}