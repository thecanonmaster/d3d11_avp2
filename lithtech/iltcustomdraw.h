#ifndef __ILTCUSTOMDRAW_H__
#define __ILTCUSTOMDRAW_H__

#include "basetypes.h"

#define LTBLEND_ZERO		1
#define LTBLEND_ONE         2
#define LTBLEND_SRCCOLOR    3
#define LTBLEND_INVSRCCOLOR 4
#define LTBLEND_SRCALPHA    5
#define LTBLEND_INVSRCALPHA 6

#define LTTEXADDR_WRAP		1
#define LTTEXADDR_CLAMP		2

#define LTOP_SELECTTEXTURE	1
#define LTOP_SELECTDIFFUSE	2
#define LTOP_MODULATE		3
#define LTOP_ADD			4
#define LTOP_ADDSIGNED		5

enum LTRState
{
	LTRSTATE_ALPHABLENDENABLE = 0,
	LTRSTATE_ZREADENABLE,
	LTRSTATE_ZWRITEENABLE,
	LTRSTATE_SRCBLEND,
	LTRSTATE_DESTBLEND,
	LTRSTATE_TEXADDR,
	LTRSTATE_COLOROP,
	LTRSTATE_ALPHAOP,
	NUM_LTRSTATES
};

class LTVertexColor
{
public:
	void	Init(uint8 r, uint8 g, uint8 b, uint8 a)
	{
		nR = r;
		nG = g;
		nB = b;
		nA = a;
	}

	uint8	nB, nG, nR, nA;

	bool CompareTo(LTVertexColor* pOther)
	{
		return *(uint32*)&nB == *(uint32*)&pOther->nB;
	}
};


class LTVertex
{
public:

	LTVector		m_vVec;
	float			m_fRHW;
	LTVertexColor	m_Color;
	LTVertexColor	m_Specular;
	float			m_fTU, m_fTV;

	bool CompareTo(LTVertex* pOther)
	{
		return m_vVec.x == pOther->m_vVec.x && m_vVec.y == pOther->m_vVec.y && m_vVec.z == pOther->m_vVec.z &&
			m_Color.CompareTo(&pOther->m_Color) && m_fTU == pOther->m_fTU && m_fTV == pOther->m_fTV;
	}
};


class ILTCustomDraw
{
public:

	virtual LTRESULT	DrawPrimitive(LTVertex* pVerts, uint32 dwVerts, uint32 dwFlags) = 0;

	virtual LTRESULT	SetState(LTRState eState, uint32 dwWal) = 0;
	virtual LTRESULT	GetState(LTRState eState, uint32& dwVal) = 0;

	virtual LTRESULT	SetTexture(const char* szTexture) = 0;

	virtual LTRESULT	GetTexelSize(float& fSizeU, float& fSizeV) = 0;
};

#endif
