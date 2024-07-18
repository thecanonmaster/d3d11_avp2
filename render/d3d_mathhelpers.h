#ifndef __D3D_MATH_HELPERS_H__
#define __D3D_MATH_HELPERS_H__

#ifndef __DIRECT_X_MATH_H__
#include <DirectXMath.h>
#define __DIRECT_X_MATH_H__
#endif

#define CPLANE_NEAR_INDEX		0
#define CPLANE_FAR_INDEX		1
#define CPLANE_LEFT_INDEX		2
#define CPLANE_TOP_INDEX		3
#define CPLANE_RIGHT_INDEX		4
#define CPLANE_BOTTOM_INDEX		5

#define NUM_CLIPPLANES			6

#define MIN_FARZ	3.0f
#define MAX_FARZ	500000.0f

#define PLTMATRIX_TO_PXMFLOAT4X4(V)		(DirectX::XMFLOAT4X4*)V
#define PLTVECTOR_TO_PXMFLOAT3(V)		(DirectX::XMFLOAT3*)V
#define PLTROTATION_TO_PXMFLOAT4(V)		(DirectX::XMFLOAT4*)V
#define PLTPLANE_TO_PXMPLANE(V)			(XMPLANE*)V

#define PXMFLOAT4X4_TO_PLTMATRIX(V)		(LTMatrix*)V
#define PXMFLOAT3_TO_PLTVECTOR(V)		(LTVector*)V
#define PXMFLOAT4_TO_PLTROTATION(V)		(LTRotation*)V
#define PXMPLANE_TO_PLTPLANE(V)			(LTPlane*)V

struct XMPLANE
{
	DirectX::XMFLOAT3	m_vNormal;
	float				m_fDist;
};

enum PolySide
{
	PS_BackSide = 0,
	PS_FrontSide,
	PS_Intersect
};

class XMIntersectQuery
{

public:

	XMIntersectQuery()
	{
		m_dwFlags = 0;
		m_FilterFn = nullptr;
		m_PolyFilterFn = nullptr;
		m_pUserData = nullptr;

		m_vFrom = { };
		m_vTo = { };
		m_vDirection = { };
	}

	DirectX::XMFLOAT3	m_vFrom;
	DirectX::XMFLOAT3	m_vTo;

	DirectX::XMFLOAT3	m_vDirection;

	uint32	m_dwFlags;

	ObjectFilterFn	m_FilterFn;
	PolyFilterFn	m_PolyFilterFn;

	void* m_pUserData;
};


struct XMIntersectInfo
{
	XMIntersectInfo()
	{
		m_vPoint = { };
		m_Plane.m_vNormal = { };
		m_Plane.m_fDist = 0.0f;
		m_pObject = nullptr;
		m_hPoly = INVALID_HPOLY;
		m_dwSurfaceFlags = 0;
	}

	DirectX::XMFLOAT3	m_vPoint;
	XMPLANE				m_Plane;

	LTObject* m_pObject;

	HPOLY	m_hPoly;

	uint32	m_dwSurfaceFlags;
};

inline void Matrix_GetTranslationLT(const DirectX::XMFLOAT4X4* pMatrix, DirectX::XMFLOAT3* pVector)
{
	pVector->x = pMatrix->m[0][3];
	pVector->y = pMatrix->m[1][3];
	pVector->z = pMatrix->m[2][3];
}

inline void Matrix_SetTranslationLT(DirectX::XMFLOAT4X4* pMatrix, float fX, float fY, float fZ)
{
	pMatrix->m[0][3] = fX;
	pMatrix->m[1][3] = fY;
	pMatrix->m[2][3] = fZ;
}

inline void Matrix_SetTranslationLT(DirectX::XMFLOAT4X4* pMatrix, const DirectX::XMFLOAT3* pVector)
{
	Matrix_SetTranslationLT(pMatrix, pVector->x, pVector->y, pVector->z);
}

void Matrix_PerspectiveBothFovLH(DirectX::XMFLOAT4X4* pMatrix, float fFovX, float fFovY, float fNearZ, float fFarZ, bool bUseTan);
void Matrix_FromQuaternionLT(DirectX::XMFLOAT4X4* pMatrix, const DirectX::XMFLOAT4* pQuat);
void Matrix_GetBasisVectorsLT(const DirectX::XMFLOAT4X4* pMatrix, DirectX::XMFLOAT3* R0, DirectX::XMFLOAT3* R1, DirectX::XMFLOAT3* R2);
void Matrix_SetBasisVectorsLT(DirectX::XMFLOAT4X4* pMatrix, const DirectX::XMFLOAT3* R0, const DirectX::XMFLOAT3* R1, const DirectX::XMFLOAT3* R2);
void Matrix_SetBasisVectors2LT(DirectX::XMFLOAT4X4* pMatrix, const DirectX::XMFLOAT3* R0, const DirectX::XMFLOAT3* R1, const DirectX::XMFLOAT3* R2);
void Matrix_Transpose3x3LT(DirectX::XMFLOAT4X4* pMatrix);
float Matrix_VMul_H(DirectX::XMFLOAT3* pDest, const DirectX::XMFLOAT4X4* pMat, const DirectX::XMFLOAT3* pSrc);
float Matrix_VMul_InPlace_H(const DirectX::XMFLOAT4X4* pMat, DirectX::XMFLOAT3* pSrc);
void Matrix_VMul_InPlace_Transposed3x3LT(const DirectX::XMFLOAT4X4* pMatrix, DirectX::XMFLOAT3* pSrc);
void Matrix_VMul_3x3(DirectX::XMFLOAT3* pDest, const DirectX::XMFLOAT4X4* pMatrix, const DirectX::XMFLOAT3* pSrc);
void Matrix_Apply3x3LT(const DirectX::XMFLOAT4X4* pMatrix, DirectX::XMFLOAT3* pVector);
void Matrix_Apply3x3LT(const DirectX::XMFLOAT4X4* pMatrix, const DirectX::XMFLOAT3* pV, DirectX::XMFLOAT3* pD);
void Matrix_NormalizeLT(DirectX::XMFLOAT4X4* pMatrix);

inline void Vector_Mult(DirectX::XMFLOAT3* pVector, float fValue)
{
	DirectX::XMStoreFloat3(pVector, DirectX::XMVectorScale(DirectX::XMLoadFloat3(pVector), fValue));
}

inline float Vector_Dot(const DirectX::XMFLOAT3* pVector1, const DirectX::XMFLOAT3* pVector2)
{
	return DirectX::XMVectorGetX(DirectX::XMVector3Dot(
		DirectX::XMLoadFloat3(pVector1), DirectX::XMLoadFloat3(pVector2)));
}

inline float Vector_Mag(const DirectX::XMFLOAT3* pVector)
{
	return DirectX::XMVectorGetX(DirectX::XMVector3Length(DirectX::XMLoadFloat3(pVector)));
}

inline float Vector_MagSqr(const DirectX::XMFLOAT3* pVector)
{
	return DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(DirectX::XMLoadFloat3(pVector)));
}

void Vector_Norm(DirectX::XMFLOAT3* pVector, float fVal);

inline void Vector_Lerp(DirectX::XMFLOAT3* pDest, DirectX::XMFLOAT3* pVector1, DirectX::XMFLOAT3* pVector2,
	float fFactor)
{
	DirectX::XMStoreFloat3(pDest, DirectX::XMVectorLerp(DirectX::XMLoadFloat3(pVector1),
		DirectX::XMLoadFloat3(pVector2), fFactor));
}

inline bool Rotation_IsIdentity(DirectX::XMFLOAT4* pRot)
{
	return	pRot->x == 0.0f && pRot->y == 0.0f &&
		pRot->z == 0.0f && pRot->w == 1.0f;
}

void Rotation_Right(DirectX::XMFLOAT4* pRot, DirectX::XMFLOAT3* pVector);
void Rotation_Up(DirectX::XMFLOAT4* pRot, DirectX::XMFLOAT3* pVector);
void Rotation_Forward(DirectX::XMFLOAT4* pRot, DirectX::XMFLOAT3* pVector);

using namespace DirectX;

inline float Plane_DistTo(XMPLANE* pPlane, DirectX::XMFLOAT3* pVector)
{
	return DirectX::XMVectorGetX(DirectX::XMVector3Dot(
		DirectX::XMLoadFloat3(&pPlane->m_vNormal), DirectX::XMLoadFloat3(pVector))) - pPlane->m_fDist;
}

inline bool Float_NearlyEquals(float fLeft, float fRight, float fTol)
{
	return fabs(fLeft - fRight) < fTol;
}

inline bool Vector_DoBoxesTouch(DirectX::XMFLOAT3* pMin1, DirectX::XMFLOAT3* pMax1, DirectX::XMFLOAT3* pMin2, 
	DirectX::XMFLOAT3* pMax2)
{
	return !(pMin1->x > pMax2->x || pMin1->y > pMax2->y || pMin1->z > pMax2->z ||
		pMax1->x < pMin2->x || pMax1->y < pMin2->y || pMax1->z < pMin2->z);
}

#endif