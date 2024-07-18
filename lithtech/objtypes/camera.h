struct CameraInstance
{
	LTObject	m_Base;
	
    int	m_nLeft; // 432
	int	m_nTop;
	int	m_nRight;
	int	m_nBottom;
	
    float	m_fFovX; // 448
	float	m_fFovY; // 452
	
    LTBOOL	m_bFullScreen; // 456

	LTVector	m_vLightAdd; // 460 ?
};