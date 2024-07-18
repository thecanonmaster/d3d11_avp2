#ifndef __AABB_H__
#define __AABB_H__

#ifndef __DIRECT_X_MATH_H__
#include <DirectXMath.h>
#define __DIRECT_X_MATH_H__
#endif

#include "d3d_mathhelpers.h"

enum AABBCorner 
{
	AABB_NearTopLeft = 0,
	AABB_NearTopRight = 1,
	AABB_NearBottomLeft = 2,
	AABB_NearBottomRight = 3,
	AABB_FarTopLeft = 4,
	AABB_FarTopRight = 5,
	AABB_FarBottomLeft = 6,
	AABB_FarBottomRight = 7,
	AABB_None = 8,
};

AABBCorner GetAABBPlaneCorner(DirectX::XMFLOAT3* pNormal);

inline DirectX::XMFLOAT3 GetAABBCornerPos(AABBCorner eCorner, DirectX::XMFLOAT3* pBoxMin, DirectX::XMFLOAT3* pBoxMax)
{
	switch (eCorner)
	{
		case AABB_NearTopLeft:
			return DirectX::XMFLOAT3(pBoxMin->x, pBoxMax->y, pBoxMin->z);
		case AABB_NearTopRight:
			return DirectX::XMFLOAT3(pBoxMax->x, pBoxMax->y, pBoxMin->z);
		case AABB_NearBottomLeft:
			return DirectX::XMFLOAT3(pBoxMin->x, pBoxMin->y, pBoxMin->z);
		case AABB_NearBottomRight:
			return DirectX::XMFLOAT3(pBoxMax->x, pBoxMin->y, pBoxMin->z);
		case AABB_FarTopLeft:
			return DirectX::XMFLOAT3(pBoxMin->x, pBoxMax->y, pBoxMax->z);
		case AABB_FarTopRight:
			return DirectX::XMFLOAT3(pBoxMax->x, pBoxMax->y, pBoxMax->z);
		case AABB_FarBottomLeft:
			return DirectX::XMFLOAT3(pBoxMin->x, pBoxMin->y, pBoxMax->z);
		case AABB_FarBottomRight:
			return DirectX::XMFLOAT3(pBoxMax->x, pBoxMin->y, pBoxMax->z);
		default:
			return *pBoxMin;
	}
}

inline AABBCorner GetAABBOtherCorner(AABBCorner eCorner)
{
	static AABBCorner aeTransCorner[8] = {
		AABB_FarBottomRight,
		AABB_FarBottomLeft,
		AABB_FarTopRight,
		AABB_FarTopLeft,
		AABB_NearBottomRight,
		AABB_NearBottomLeft,
		AABB_NearTopRight,
		AABB_NearTopLeft
	};

	return aeTransCorner[eCorner];
}

inline PolySide GetAABBPlaneSide(AABBCorner eCorner, XMPLANE* pPlane, DirectX::XMFLOAT3* pBoxMin, DirectX::XMFLOAT3* pBoxMax)
{
	DirectX::XMFLOAT3 vCornerPos = GetAABBCornerPos(eCorner, pBoxMin, pBoxMax);
	float fNearDist = Plane_DistTo(pPlane, &vCornerPos);

	if (fNearDist < 0.001f)
		return PS_BackSide;

	vCornerPos = GetAABBCornerPos(GetAABBOtherCorner(eCorner), pBoxMin, pBoxMax);
	float fFarDist = Plane_DistTo(pPlane, &vCornerPos);

	if (fFarDist > 0.001f)
		return PS_FrontSide;
	else
		return PS_Intersect;
}

inline bool GetAABBPlaneSideBack(AABBCorner eCorner, XMPLANE* pPlane, DirectX::XMFLOAT3* pBoxMin,
	DirectX::XMFLOAT3* pBoxMax)
{
	DirectX::XMFLOAT3 vCornerPos = GetAABBCornerPos(eCorner, pBoxMin, pBoxMax);
	
	float fNearDist = Plane_DistTo(pPlane, &vCornerPos);
	return fNearDist < 0.001f;
}

inline bool GetAABBPlaneSideFront(AABBCorner eCorner, XMPLANE* pPlane, DirectX::XMFLOAT3* pBoxMin,
	DirectX::XMFLOAT3* pBoxMax)
{
	DirectX::XMFLOAT3 vCornerPos = GetAABBCornerPos(GetAABBOtherCorner(eCorner), pBoxMin, pBoxMax);

	float fFarDist = Plane_DistTo(pPlane, &vCornerPos);
	return fFarDist > 0.001f;
}

#endif