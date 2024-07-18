struct DynamicLight
{
	LTObject	m_Base;
	
	float	m_fLightRadius;

	bool IsBlackLight()
	{
		return (m_Base.m_nColorR < 128 && m_Base.m_nColorG < 128 && m_Base.m_nColorB < 128);
	}
};