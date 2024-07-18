#include "pch.h"

#include "d3d_mathhelpers.h"

/*
void D3DXMatrixPerspectiveBothFovLH(DirectX::XMFLOAT4X4* pOut, float fFovX, float fFovY, float fNearZ, float fFarZ)
{
	DirectX::XMStoreFloat4x4(pOut, DirectX::XMMatrixIdentity());

	pOut->m[0][0] = 1.0f / tanf(fFovX / 2.0f);
	pOut->m[1][1] = 1.0f / tanf(fFovY / 2.0f);
	pOut->m[2][2] = fFarZ / (fFarZ - fNearZ);
	pOut->m[2][3] = 1.0f;
	pOut->m[3][2] = (fFarZ * fNearZ) / (fNearZ - fFarZ);
	pOut->m[3][3] = 0.0f;
}
*/

void Matrix_PerspectiveBothFovLH(DirectX::XMFLOAT4X4* pMatrix, float fFovX, float fFovY, float fNearZ, float fFarZ, bool bUseTan)
{
	float fSinFovX, fSinFovY;
	float fCosFovX, fCosFovY;
	DirectX::XMScalarSinCos(&fSinFovX, &fCosFovX, 0.5f * fFovX);
	DirectX::XMScalarSinCos(&fSinFovY, &fCosFovY, 0.5f * fFovY);

	float fHeight;
	float fWidth;
	if (bUseTan)
	{
		fHeight = fSinFovY / fCosFovY;
		fWidth = fSinFovX / fCosFovX;
	}
	else
	{
		fHeight = fCosFovY / fSinFovY;
		fWidth = fCosFovX / fSinFovX;
	}

	float fRange = fFarZ / (fFarZ - fNearZ);

	pMatrix->m[0][0] = fWidth;
	pMatrix->m[0][1] = 0.0f;
	pMatrix->m[0][2] = 0.0f;
	pMatrix->m[0][3] = 0.0f;

	pMatrix->m[1][0] = 0.0f;
	pMatrix->m[1][1] = fHeight;
	pMatrix->m[1][2] = 0.0f;
	pMatrix->m[1][3] = 0.0f;

	pMatrix->m[2][0] = 0.0f;
	pMatrix->m[2][1] = 0.0f;
	pMatrix->m[2][2] = fRange;
	pMatrix->m[2][3] = 1.0f;

	pMatrix->m[3][0] = 0.0f;
	pMatrix->m[3][1] = 0.0f;
	pMatrix->m[3][2] = -fRange * fNearZ;
	pMatrix->m[3][3] = 0.0f;
}

void Matrix_FromQuaternionLT(DirectX::XMFLOAT4X4* pMatrix, const DirectX::XMFLOAT4* pQuat)
{
	float s, xs, ys, zs, wx, wy, wz, xx, xy, xz, yy, yz, zz;

	s = 2.0f / ((pQuat->x * pQuat->x) + (pQuat->y * pQuat->y) + (pQuat->z * pQuat->z) + (pQuat->w * pQuat->w));

	xs = pQuat->x * s;
	ys = pQuat->y * s;
	zs = pQuat->z * s;

	wx = pQuat->w * xs;
	wy = pQuat->w * ys;
	wz = pQuat->w * zs;

	xx = pQuat->x * xs;
	xy = pQuat->x * ys;
	xz = pQuat->x * zs;

	yy = pQuat->y * ys;
	yz = pQuat->y * zs;

	zz = pQuat->z * zs;

	pMatrix->m[0][0] = 1.0f - (yy + zz);
	pMatrix->m[0][1] = xy - wz;
	pMatrix->m[0][2] = xz + wy;

	pMatrix->m[1][0] = xy + wz;
	pMatrix->m[1][1] = 1.0f - (xx + zz);
	pMatrix->m[1][2] = yz - wx;

	pMatrix->m[2][0] = xz - wy;
	pMatrix->m[2][1] = yz + wx;
	pMatrix->m[2][2] = 1.0f - (xx + yy);

	pMatrix->m[0][3] = pMatrix->m[1][3] = pMatrix->m[2][3] =
		pMatrix->m[3][0] = pMatrix->m[3][1] = pMatrix->m[3][2] = 0.0f;

	pMatrix->m[3][3] = 1.0f;
}

void Matrix_GetBasisVectorsLT(const DirectX::XMFLOAT4X4* pMatrix, DirectX::XMFLOAT3* R0, DirectX::XMFLOAT3* R1, DirectX::XMFLOAT3* R2)
{
	R0->x = pMatrix->m[0][0];
	R0->y = pMatrix->m[1][0];
	R0->z = pMatrix->m[2][0];

	R1->x = pMatrix->m[0][1];
	R1->y = pMatrix->m[1][1];
	R1->z = pMatrix->m[2][1];

	R2->x = pMatrix->m[0][2];
	R2->y = pMatrix->m[1][2];
	R2->z = pMatrix->m[2][2];
}

void Matrix_SetBasisVectorsLT(DirectX::XMFLOAT4X4* pMatrix, const DirectX::XMFLOAT3* R0, const DirectX::XMFLOAT3* R1, const DirectX::XMFLOAT3* R2)
{
	*pMatrix = 
	{ 
		R0->x, R1->x, R2->x, 0.0f,
		R0->y, R1->y, R2->y, 0.0f,
		R0->z, R1->z, R2->z, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f 
	};
}

void Matrix_SetBasisVectors2LT(DirectX::XMFLOAT4X4* pMatrix, const DirectX::XMFLOAT3* R0, const DirectX::XMFLOAT3* R1, const DirectX::XMFLOAT3* R2)
{
	pMatrix->m[0][0] = R0->x;
	pMatrix->m[1][0] = R0->y;
	pMatrix->m[2][0] = R0->z;

	pMatrix->m[0][1] = R1->x;
	pMatrix->m[1][1] = R1->y;
	pMatrix->m[2][1] = R1->z;

	pMatrix->m[0][2] = R2->x;
	pMatrix->m[1][2] = R2->y;
	pMatrix->m[2][2] = R2->z;
}

void Matrix_Transpose3x3LT(DirectX::XMFLOAT4X4* pMatrix)
{
	float fTemp;

	fTemp = pMatrix->m[0][1];
	pMatrix->m[0][1] = pMatrix->m[1][0];
	pMatrix->m[1][0] = fTemp;

	fTemp = pMatrix->m[0][2];
	pMatrix->m[0][2] = pMatrix->m[2][0];
	pMatrix->m[2][0] = fTemp;

	fTemp = pMatrix->m[2][1];
	pMatrix->m[2][1] = pMatrix->m[1][2];
	pMatrix->m[1][2] = fTemp;
}

float Matrix_VMul_H(DirectX::XMFLOAT3* pDest, const DirectX::XMFLOAT4X4* pMat, const DirectX::XMFLOAT3* pSrc)
{
	float fOneOverW = 1.0f / (pMat->m[3][0] * pSrc->x + pMat->m[3][1] * pSrc->y + pMat->m[3][2] * pSrc->z + pMat->m[3][3]);

	pDest->x = fOneOverW * (pMat->m[0][0] * pSrc->x + pMat->m[0][1] * pSrc->y + pMat->m[0][2] * pSrc->z + pMat->m[0][3]);
	pDest->y = fOneOverW * (pMat->m[1][0] * pSrc->x + pMat->m[1][1] * pSrc->y + pMat->m[1][2] * pSrc->z + pMat->m[1][3]);
	pDest->z = fOneOverW * (pMat->m[2][0] * pSrc->x + pMat->m[2][1] * pSrc->y + pMat->m[2][2] * pSrc->z + pMat->m[2][3]);

	return fOneOverW;
}

float Matrix_VMul_InPlace_H(const DirectX::XMFLOAT4X4* pMat, DirectX::XMFLOAT3* pSrc)
{
	float fOneOverW = 1.0f / (pMat->m[3][0] * pSrc->x + pMat->m[3][1] * pSrc->y + pMat->m[3][2] * pSrc->z + pMat->m[3][3]);

	DirectX::XMFLOAT3 vTemp;

	vTemp.x = fOneOverW * (pMat->m[0][0] * pSrc->x + pMat->m[0][1] * pSrc->y + pMat->m[0][2] * pSrc->z + pMat->m[0][3]);
	vTemp.y = fOneOverW * (pMat->m[1][0] * pSrc->x + pMat->m[1][1] * pSrc->y + pMat->m[1][2] * pSrc->z + pMat->m[1][3]);
	vTemp.z = fOneOverW * (pMat->m[2][0] * pSrc->x + pMat->m[2][1] * pSrc->y + pMat->m[2][2] * pSrc->z + pMat->m[2][3]);

	*pSrc = vTemp;

	return fOneOverW;
}

void Matrix_VMul_InPlace_Transposed3x3LT(const DirectX::XMFLOAT4X4* pMatrix, DirectX::XMFLOAT3* pSrc)
{
	DirectX::XMFLOAT3 vTemp;

	vTemp.x = pMatrix->m[0][0] * pSrc->x + pMatrix->m[1][0] * pSrc->y + pMatrix->m[2][0] * pSrc->z;
	vTemp.y = pMatrix->m[0][1] * pSrc->x + pMatrix->m[1][1] * pSrc->y + pMatrix->m[2][1] * pSrc->z;
	vTemp.z = pMatrix->m[0][2] * pSrc->x + pMatrix->m[1][2] * pSrc->y + pMatrix->m[2][2] * pSrc->z;

	*pSrc = vTemp;
}

void Matrix_VMul_3x3(DirectX::XMFLOAT3* pDest, const DirectX::XMFLOAT4X4* pMatrix, const DirectX::XMFLOAT3* pSrc)
{
	pDest->x = pMatrix->m[0][0] * pSrc->x + pMatrix->m[0][1] * pSrc->y + pMatrix->m[0][2] * pSrc->z;
	pDest->y = pMatrix->m[1][0] * pSrc->x + pMatrix->m[1][1] * pSrc->y + pMatrix->m[1][2] * pSrc->z;
	pDest->z = pMatrix->m[2][0] * pSrc->x + pMatrix->m[2][1] * pSrc->y + pMatrix->m[2][2] * pSrc->z;
}

void Matrix_Apply3x3LT(const DirectX::XMFLOAT4X4* pMatrix, DirectX::XMFLOAT3* pVector)
{
	DirectX::XMFLOAT3 vTemp;
	Matrix_Apply3x3LT(pMatrix, pVector, &vTemp);
	*pVector = vTemp;
}

void Matrix_Apply3x3LT(const DirectX::XMFLOAT4X4* pMatrix, const DirectX::XMFLOAT3* pV, DirectX::XMFLOAT3* pD)
{
	pD->x = pV->x * pMatrix->m[0][0] + pV->y * pMatrix->m[0][1] + pV->z * pMatrix->m[0][2];
	pD->y = pV->x * pMatrix->m[1][0] + pV->y * pMatrix->m[1][1] + pV->z * pMatrix->m[1][2];
	pD->z = pV->x * pMatrix->m[2][0] + pV->y * pMatrix->m[2][1] + pV->z * pMatrix->m[2][2];
}

#define VEC_MAGSQR(v) ((v).x*(v).x + (v).y*(v).y + (v).z*(v).z)
#define VEC_MAG(v) ((float)sqrt(VEC_MAGSQR(v)))

#define VEC_DIVSCALAR(d, v1, s) \
    {\
    (d).x = (v1).x / (s); \
    (d).y = (v1).y / (s); \
    (d).z = (v1).z / (s);\
    }

#define VEC_CROSS(dest, v1, v2) \
    {\
    (dest).x = ((v2).y*(v1).z - (v2).z*(v1).y);\
    (dest).y = ((v2).z*(v1).x - (v2).x*(v1).z);\
    (dest).z = ((v2).x*(v1).y - (v2).y*(v1).x);\
    }

void Matrix_NormalizeLT(DirectX::XMFLOAT4X4* pMatrix)
{
	DirectX::XMFLOAT3 vRight, vUp, vForward;
	
	Matrix_GetBasisVectorsLT(pMatrix, &vRight, &vUp, &vForward);

	float fLen = VEC_MAG(vRight);
	VEC_DIVSCALAR(vRight, vRight, fLen);

	fLen = VEC_MAG(vUp);
	VEC_DIVSCALAR(vUp, vUp, fLen);

	VEC_CROSS(vForward, vUp, vRight);

	Matrix_SetBasisVectors2LT(pMatrix, &vRight, &vUp, &vForward);
}

void Vector_Norm(DirectX::XMFLOAT3* pVector, float fVal)
{
	float fMag = Vector_Mag(pVector);

	if (fMag == 0.0f)
		return;

	float fInv = fVal / fMag;

	pVector->x *= fInv;
	pVector->y *= fInv;
	pVector->z *= fInv;
}

void Rotation_Right(DirectX::XMFLOAT4* pRot, DirectX::XMFLOAT3* pVector)
{
	pVector->x = 1.0f - 2.0f * (pRot->y * pRot->y + pRot->z * pRot->z);
	pVector->y = 2.0f * (pRot->x * pRot->y + pRot->w * pRot->z);
	pVector->z = 2.0f * (pRot->x * pRot->z - pRot->w * pRot->y);
}

void Rotation_Up(DirectX::XMFLOAT4* pRot, DirectX::XMFLOAT3* pVector)
{
	pVector->x = 2.0f * (pRot->x * pRot->y - pRot->w * pRot->z);
	pVector->y = 1.0f - 2.0f * (pRot->x * pRot->x + pRot->z * pRot->z);
	pVector->z = 2.0f * (pRot->y * pRot->z + pRot->w * pRot->x);
}

void Rotation_Forward(DirectX::XMFLOAT4* pRot, DirectX::XMFLOAT3* pVector)
{
	pVector->x = 2.0f * (pRot->x * pRot->z + pRot->w * pRot->y);
	pVector->y = 2.0f * (pRot->y * pRot->z - pRot->w * pRot->x);
	pVector->z = 1.0f - 2.0f * (pRot->x * pRot->x + pRot->y * pRot->y);
}

