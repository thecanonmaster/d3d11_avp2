struct SpriteInstance;
struct SharedTexture;

struct SpriteControlImpl
{
	uint32*	m_pVTable;
	
	SpriteInstance*	m_pSprite;
};

struct SpriteEntry
{
    SharedTexture*	m_pTexture;
};

struct SpriteAnim
{
    char	m_szName[32]; // 0
	
    SpriteEntry*	m_pFrames; // 32
    uint32			m_nFrames; // 36
	
    uint32	m_dwAnimLengthMS; // 40
    uint32	m_dwFrameRateMS; // 44
	
    LTBOOL	m_bKeyed; // 48
    uint32	m_dwColourKey; // 52
    LTBOOL	m_bTranslucent; // 56
};

struct Sprite
{	
	LTLink	m_Link;
	
	SpriteAnim*	m_pAnims;
	uint32		m_nAnims;
};

struct SpriteTracker
{
    Sprite*			m_pSprite;
    SpriteAnim*		m_pCurAnim;
    SpriteEntry*	m_pCurFrame;  
	
    uint32	m_dwCurTimeMS;
    uint32	m_dwFlags;
};

struct SpriteInstance
{
	LTObject	m_Base;
	
	SpriteTracker	m_SpriteTracker;
	
	HPOLY	m_hClipperPoly;
	
	SpriteControlImpl	m_SCImpl;
};