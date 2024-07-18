#include "pch.h"

#include "3d_ops.h"
#include "d3d_device.h"
#include "d3d_mathhelpers.h"
#include "rendererconsolevars.h"
#include "common_draw.h"
#include "globalmgr.h"

void d3d_SetupTransformation(const LTVector* pPos, float* pRotation, LTVector* pScale, DirectX::XMFLOAT4X4* pMatrix)
{
	if (pRotation[0] > ROTATION_MAX || pRotation[0] < -ROTATION_MAX)
		pRotation[0] = 0.0f;

	if (pRotation[1] > ROTATION_MAX || pRotation[1] < -ROTATION_MAX)
		pRotation[1] = 0.0f;

	if (pRotation[2] > ROTATION_MAX || pRotation[2] < -ROTATION_MAX)
		pRotation[2] = 0.0f;

	if (pRotation[3] > ROTATION_MAX || pRotation[3] < -ROTATION_MAX)
		pRotation[3] = 1.0f;

	Matrix_FromQuaternionLT(pMatrix, PLTROTATION_TO_PXMFLOAT4(pRotation));

	pMatrix->m[0][0] *= pScale->x;
	pMatrix->m[1][0] *= pScale->x;
	pMatrix->m[2][0] *= pScale->x;

	pMatrix->m[0][1] *= pScale->y;
	pMatrix->m[1][1] *= pScale->y;
	pMatrix->m[2][1] *= pScale->y;

	pMatrix->m[0][2] *= pScale->z;
	pMatrix->m[1][2] *= pScale->z;
	pMatrix->m[2][2] *= pScale->z;

	pMatrix->m[0][3] = pPos->x;
	pMatrix->m[1][3] = pPos->y;
	pMatrix->m[2][3] = pPos->z;
}

/*void EFO_Test(float fFovXOffset, float fFovYOffset, float& fFovXOut, float& fFovYOut)
{
	float fVar1 = 0.1745329f;
	float fVar2 = MATH_DEGREES_TO_RADIANS(g_CV_ExtraFOVXOffset.m_Val) + g_pSceneDesc->m_fFovX + fFovXOffset;

	if ((0.1745329f <= fVar2) && (fFovXOut = fVar2, 2.96706f < fVar2))
	{
		//fVar1 = 2.96706f;
		fFovXOut = 2.96706f;
	}

	fVar2 = MATH_DEGREES_TO_RADIANS(g_CV_ExtraFOVYOffset.m_Val) + g_pSceneDesc->m_fFovY + fFovYOffset;

	fFovYOut = 0.1745329f;
	if ((0.1745329f <= fVar2) && (fFovYOut = 2.96706f, fVar2 <= 2.96706f))
	{
		//fVar3 = fVar2;
		fFovYOut = fVar2;
	}
}*/

void d3d_SetReallyClose(CReallyCloseData* pData, DirectX::XMFLOAT4X4* pProjection, 
	float fFovXOffset, float fFovYOffset, bool bUseTan)
{
	d3d_SetReallyClose(pData);
	d3d_CalcReallyCloseMatrix(pProjection, fFovXOffset, fFovYOffset, bUseTan);
}

void d3d_SetReallyClose(CReallyCloseData* pData)
{
	uint32 dwNumViewPorts = 1;
	g_D3DDevice.GetDeviceContext()->RSGetViewports(&dwNumViewPorts, &pData->m_Viewport);

	D3D11_VIEWPORT newViewport;

	newViewport.Width = pData->m_Viewport.Width;
	newViewport.Height = pData->m_Viewport.Height;
	newViewport.TopLeftX = pData->m_Viewport.TopLeftX;
	newViewport.TopLeftY = pData->m_Viewport.TopLeftY;

	// TODO - min max depths? hack for MP model preview in the menu
	newViewport.MinDepth = VIEWPORT_REALLY_CLOSE_MIN_Z;
	newViewport.MaxDepth = 
		g_pSceneDesc->m_nDrawMode == DRAWMODE_NORMAL ? VIEWPORT_REALLY_CLOSE_MAX_Z : VIEWPORT_DEFAULT_MAX_Z;

	g_D3DDevice.GetDeviceContext()->RSSetViewports(1, &newViewport);
}

void d3d_CalcReallyCloseMatrix(DirectX::XMFLOAT4X4* pProjection, float fFovXOffset, float fFovYOffset, bool bUseTan)
{
	/*float fAspect = g_ViewParams.m_fScreenWidth / g_ViewParams.m_fScreenHeight;

	DirectX::XMMATRIX mNewProj = DirectX::XMMatrixTranspose(DirectX::XMMatrixPerspectiveFovLH(
		MATH_DEGREES_TO_RADIANS(g_CV_ExtraFOVYOffset.m_Val) + g_pSceneDesc->m_fFovY + fFovYOffset,
		fAspect, g_CV_ReallyCloseNearZ.m_Val, g_ViewParams.m_fFarZ));

	DirectX::XMStoreFloat4x4(pProjection, mNewProj);*/

	// TODO - Extra FOV offsets are not only for RC rendering
	Matrix_PerspectiveBothFovLH(pProjection,
		g_pSceneDesc->m_fFovX + fFovXOffset, g_pSceneDesc->m_fFovY + fFovYOffset,
		g_CV_ReallyCloseNearZ.m_Val, g_ViewParams.m_fFarZ, bUseTan);

	DirectX::XMMATRIX mNewProj = DirectX::XMLoadFloat4x4(pProjection);
	DirectX::XMStoreFloat4x4(pProjection, DirectX::XMMatrixTranspose(mNewProj));
}

void d3d_UnsetReallyClose(CReallyCloseData* pData)
{
	g_D3DDevice.GetDeviceContext()->RSSetViewports(1, &pData->m_Viewport);
}