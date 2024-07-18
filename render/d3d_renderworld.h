#ifndef __D3D_RENDERWORLD_H__
#define __D3D_RENDERWORLD_H__

#ifndef __DIRECT_X_MATH_H__
#include <DirectXMath.h>
#define __DIRECT_X_MATH_H__
#endif

#include "d3d_renderworld_occluder.h"
#include "d3d_utils.h"

class ViewParams;
class CWorldModelData;

class CRenderWorld
{

public:

	CRenderWorld() 
	{ 
		m_FrameFrustumOccluder = { };
	};

	~CRenderWorld() { };

	void	StartFrame(ViewParams* pParams);

	void	DrawSkyExtents(ViewParams* pParams);

	bool	IsAABBVisible(ViewParams* pParams, DirectX::XMFLOAT3* pMin, DirectX::XMFLOAT3* pMax);

private:

	void	ExtendSkyBounds(ViewParams* pParams, float& fMinX, float& fMinY, float& fMaxX, float& fMaxY);

	COccluder_Frustum	m_FrameFrustumOccluder;
};

extern CRenderWorld g_RenderWorld;

#endif