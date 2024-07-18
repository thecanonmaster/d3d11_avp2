// Unverified
struct LineSystem;
struct StructBank;

struct LTLinePt
{
	LTVector	m_vPos;
	
	float	m_fR;
	float	m_fG;
	float	m_fB;
	float	m_fA;

	bool CompareTo(LTLinePt* pOther)
	{
		return m_vPos.x == pOther->m_vPos.x && m_vPos.y == pOther->m_vPos.y && m_vPos.z == pOther->m_vPos.z &&
			m_fR == pOther->m_fR && m_fG == pOther->m_fG && m_fB == pOther->m_fB && m_fA == pOther->m_fA;
	}
};

struct LTLine
{
	LTLinePt	m_Points[2];

	LineSystem	*m_pSystem; // 56

	LTLine*	m_pPrev; // 60
	LTLine*	m_pNext; // 64
};

struct LineSystem
{
	LTObject	m_Base;

	StructBank*	m_pLineBank; // 432
	
	LTBOOL	m_bChanged; // 436 ?

	LTLine	m_LineHead; // 440
	
    LTVector	m_vMinPos; // 508
	LTVector	m_vMaxPos; // 520
	
    LTVector	m_vSystemCenter; // 532
    float		m_fSystemRadius; // 544
};