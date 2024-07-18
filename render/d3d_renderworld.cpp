#include "pch.h"

#include "d3d_renderworld.h"
#include "d3d_viewparams.h"
#include "draw_sky.h"
#include "draw_worldmodel.h"

CRenderWorld g_RenderWorld;

void CRenderWorld::StartFrame(ViewParams* pParams)
{
	m_FrameFrustumOccluder.InitFrustum(pParams);
}

void CRenderWorld::DrawSkyExtents(ViewParams* pParams)
{
	/*float fSkyMinX = FLT_MAX;
	float fSkyMinY = FLT_MAX;
	float fSkyMaxX = -FLT_MAX;
	float fSkyMaxY = -FLT_MAX;

	d3d_ExtendSkyBounds(pParams, fSkyMinX, fSkyMinY, fSkyMaxX, fSkyMaxY);*/

	float fSkyMinX = (float)pParams->m_Rect.m_nLeft;
	float fSkyMinY = (float)pParams->m_Rect.m_nTop;
	float fSkyMaxX = (float)pParams->m_Rect.m_nRight;
	float fSkyMaxY = (float)pParams->m_Rect.m_nBottom;

	//if (((fSkyMaxX - fSkyMinX) > 0.9f) && ((fSkyMaxY - fSkyMinY) > 0.9f))
		d3d_DrawSkyExtents(pParams, fSkyMinX, fSkyMinY, fSkyMaxX, fSkyMaxY);
}

bool CRenderWorld::IsAABBVisible(ViewParams* pParams, DirectX::XMFLOAT3* pMin, DirectX::XMFLOAT3* pMax)
{	
	if ((pMin->x <= pParams->m_vPos.x) && (pMin->y <= pParams->m_vPos.y) && (pMin->z <= pParams->m_vPos.z) &&
		(pMax->x >= pParams->m_vPos.x) && (pMax->y >= pParams->m_vPos.y) && (pMax->z >= pParams->m_vPos.z))
	{
		return true;
	}

	PolySide eBoxSide = m_FrameFrustumOccluder.ClassifyAABB(pMin, pMax, pParams->m_fFarZ);
	
	return (eBoxSide != PS_BackSide);

	// TODO - extra occluders?
	//if (!bUseOccluders)
	//	return true;
}
