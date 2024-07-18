#include "pch.h"

bool AnimTimeRef::IsValid()
{
	return m_pModel &&
		m_Prev.m_wAnim < m_pModel->GetAnimCount() &&
		m_Cur.m_wAnim < m_pModel->GetAnimCount() &&
		m_Prev.m_wFrame < m_pModel->GetAnim(m_Prev.m_wAnim)->GetKeyFrameCount() &&
		m_Cur.m_wFrame < m_pModel->GetAnim(m_Cur.m_wAnim)->GetKeyFrameCount() &&
		m_fPercent >= 0.0f && m_fPercent <= 1.0f;
};

uint32 ModelPiece::GetLODIndexFromDist(int nBias, float fDist)
{
	if (!m_LODs.m_dwElements)
		return 0;

	int nLOD = 0;
	int nNumLODS = m_LODs.m_dwElements;

	if (fDist > 0.0f)
	{
		nLOD = nNumLODS;

		for (int i = 1; i < nNumLODS; i++)
		{
			if (fDist < m_pModel->GetLODInfo(nLOD - 1)->m_fDist)
			{
				nLOD = i - 1;
				break;
			}
		}
	}

	return LTCLAMP(nLOD + nBias, 0, nNumLODS);
}

bool ModelInstance::IsPieceHidden(uint32 index)
{ 
	return !!(m_adwHiddenPiece[index / 32] & (1 << (index % 32)));
}
