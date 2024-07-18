#include "pch.h"

static void GetMaskBounds(uint32 dwMask, uint32* pLeft, uint32* pRight)
{
	uint32 dwTestMask, i;

	*pRight = 0;
	dwTestMask = 1;
	for (i = 0; i < 32; i++)
	{
		if (dwTestMask & dwMask)
			break;

		dwTestMask <<= 1;
		(*pRight)++;
	}

	*pLeft = *pRight;
	for (i = 0; i < 32; i++)
	{
		if (!(dwTestMask & dwMask))
			break;

		dwTestMask <<= 1;
		(*pLeft)++;
	}
}

static void SetBitCountAndRightShift(PFormat* pFormat, uint32 dwPlane)
{
	uint32 dwLeft, dwRight;

	GetMaskBounds(pFormat->m_adwMasks[dwPlane], &dwLeft, &dwRight);
	pFormat->m_adwBits[dwPlane] = dwLeft - dwRight;
	pFormat->m_adwFirstBits[dwPlane] = dwRight;
}

void PFormat::Init(BPPIdent eType, uint32 dwAMask, uint32 dwRMask, uint32 dwGMask, uint32 dwBMask)
{
	m_eType = eType;

	m_adwMasks[CP_ALPHA] = dwAMask;
	m_adwMasks[CP_RED] = dwRMask;
	m_adwMasks[CP_GREEN] = dwGMask;
	m_adwMasks[CP_BLUE] = dwBMask;

	for (uint32 i = 0; i < NUM_COLORPLANES; i++)
		SetBitCountAndRightShift(this, i);
}
