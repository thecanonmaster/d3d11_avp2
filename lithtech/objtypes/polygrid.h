// Unverified
struct PolyGrid
{
	LTObject	m_Base;

	char*	m_pData; // 432
    uint16*	m_pIndices; // 436
	
	Sprite*			m_pSprite; // 440
    SpriteTracker	m_SpriteTracker; // 444
	SharedTexture*	m_pEnvMap; // 464

	float	m_fPanX; // 468
	float	m_fPanY; // 472
	float	m_fScaleX; // 476
	float	m_fScaleY; // 480

	uint32	m_dwTris; // 484
	uint32	m_dwIndices; // 488

	LTLink	m_LeafLinks; // 492

	uint32  m_dwWidth; // 504
	uint32	m_dwHeight; // 508

	PGColor	m_aColorTable[POLY_GRID_COLOR_TABLE_SIZE]; // 512
};
