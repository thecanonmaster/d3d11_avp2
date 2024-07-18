#ifndef _D3D_VIEW_PARAMS_H_
#define _D3D_VIEW_PARAMS_H_

#ifndef __DIRECT_X_MATH_H__
#include <DirectXMath.h>
#define __DIRECT_X_MATH_H__
#endif

#include "d3d_mathhelpers.h"
#include "aabb.h"

struct ViewBoxDef
{
	DirectX::XMFLOAT3	m_vCOP;

	float	m_fWindowSize[2];

	float	m_fNearZ;
	float	m_fFarZ;
};

class ViewParams
{

public:

	ViewBoxDef	m_ViewBox;

	LTRect	m_Rect;

	float	m_fScreenWidth;
	float	m_fScreenHeight;

	float	m_fNearZ;
	float	m_fFarZ;

	DirectX::XMFLOAT4X4	m_mInvView;
	DirectX::XMFLOAT4X4	m_mInvViewTransposed;
	DirectX::XMFLOAT4X4	m_mView;
	DirectX::XMFLOAT4X4	m_mViewTransposed;

	DirectX::XMFLOAT4X4	m_mProjection;
	DirectX::XMFLOAT4X4	m_mProjectionTransposed;
	//DirectX::XMFLOAT4X4	m_mSkyFarZProjectionTransposed;

	DirectX::XMFLOAT4X4	m_mWorldEnvMap;
	DirectX::XMFLOAT4X4	m_mDeviceTimesProjection;
	DirectX::XMFLOAT4X4	m_mFullTransform;

	DirectX::XMFLOAT4X4	m_mIdentity;
	DirectX::XMFLOAT4X4	m_mInvWorld;

	DirectX::XMFLOAT3	m_avViewPoints[8];

	DirectX::XMFLOAT3	m_vViewAABBMin;
	DirectX::XMFLOAT3	m_vViewAABBMax;

	XMPLANE	m_aClipPlanes[NUM_CLIPPLANES];

	AABBCorner	m_eAABBPlaneCorner[NUM_CLIPPLANES];

	DirectX::XMFLOAT3	m_vUp;
	DirectX::XMFLOAT3	m_vRight;
	DirectX::XMFLOAT3	m_vForward;

	DirectX::XMFLOAT3	m_vPos;
	DirectX::XMFLOAT3	m_vSkyViewPos;
};

#endif