#ifndef __COMMON_DRAW_H__
#define __COMMON_DRAW_H__

#ifndef __DIRECT_X_MATH_H__
#include <DirectXMath.h>
#define __DIRECT_X_MATH_H__
#endif

#include "d3d_mathhelpers.h"
#include "d3d_viewparams.h"

extern uint16 g_wCurFrameCode;
extern uint32 g_dwCurObjectFrameCode;
extern SceneDesc* g_pSceneDesc;
extern bool g_bHaveWorld;
extern bool g_bNewRenderContext;
extern ViewParams g_ViewParams;

void d3d_InitViewBox2(ViewBoxDef* pDef, float fNearZ, float fFarZ, ViewParams* pPrevParams,
	float fScreenMinX, float fScreenMinY, float fScreenMaxX, float fScreenMaxY);

bool d3d_InitFrustum2(ViewParams* pParams, ViewBoxDef* pViewBox, float fScreenMinX, float fScreenMinY,
	float fScreenMaxX, float fScreenMaxY, const DirectX::XMFLOAT4X4* pMatrix, const DirectX::XMFLOAT3* pScale);

bool d3d_InitFrame(ViewParams* pParams, SceneDesc* pDesc);

inline void d3d_SetFPState()
{
	uint16 wControl;
	_asm 
	{
		fstcw wControl
		and wControl, 0xFCFF
		fldcw wControl
	}
}

#endif