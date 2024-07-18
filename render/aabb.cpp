#include "pch.h"

#include "aabb.h"
#include "d3d_mathhelpers.h"

static DirectX::XMFLOAT3 g_avCornerDir[8] = 
{
	DirectX::XMFLOAT3(-1.0f, 1.0f, -1.0f),
	DirectX::XMFLOAT3(1.0f, 1.0f, -1.0f),
	DirectX::XMFLOAT3(-1.0f, -1.0f, -1.0f),
	DirectX::XMFLOAT3(1.0f, -1.0f, -1.0f),
	DirectX::XMFLOAT3(-1.0f, 1.0f, 1.0f),
	DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f),
	DirectX::XMFLOAT3(-1.0f, -1.0f, 1.0f),
	DirectX::XMFLOAT3(1.0f, -1.0f, 1.0f),
};

AABBCorner GetAABBPlaneCorner(DirectX::XMFLOAT3* pNormal)
{
	int nBestCorner = 0;
	float fBestCornerDot = -1.0f;

	for (int nCornerLoop = 0; nCornerLoop < 8; nCornerLoop++)
	{
		float fCornerDot = Vector_Dot(pNormal, &g_avCornerDir[nCornerLoop]);

		if (fCornerDot > fBestCornerDot)
		{
			nBestCorner = nCornerLoop;
			fBestCornerDot = fCornerDot;
		}
	}

	return (AABBCorner)nBestCorner;
}
