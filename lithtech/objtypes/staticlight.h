struct StaticLight;

struct StaticLightListElement
{
	StaticLightListElement*	pNext;
	StaticLightListElement*	pPrev;
	
	StaticLight* pItem;
};

struct StaticLightListHead
{
	StaticLightListElement*	pNext;
	StaticLightListElement*	pPrev;
};

struct StaticLight
{
	WorldTreeObj	m_Base;

	StaticLightListElement	m_Link; // 92 ?

	LTVector	m_vPos; // 104

	float	m_fRadius; // 116

	LTVector	m_vInnerColor; // 120
	LTVector	m_vDir; // 132 - rotation
	float		m_fFOV; // 144
    LTVector	m_vOuterColor; // 148
};