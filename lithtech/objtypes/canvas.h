// Unverified
class ILTCustomDraw;
typedef void (*CanvasDrawFn)(ILTCustomDraw* pDraw, LTObject* pObj, void* pUser);

struct Canvas
{
	LTObject	m_Base;

	CanvasDrawFn    m_pFn; // 432
    void*			m_pFnUserData; // 436
	
    float	m_fCanvasRadius; // 440
};