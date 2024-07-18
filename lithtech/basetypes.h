#ifndef __LT_BASE_TYPES__
#define __LT_BASE_TYPES__

struct LTObject;

typedef char				int8;
typedef unsigned char		uint8;
typedef short				int16;
typedef unsigned short		uint16;
typedef int					int32; // long in SDK
typedef unsigned int		uint32; // unsigned long in SDK

typedef unsigned int LTBOOL;
typedef float LTFLOAT;
typedef uint32 LTRESULT;

typedef uint32 HLTCOLOR;
typedef unsigned int HPOLY; // unsigned long in SDK

union GenericColor
{
	GenericColor(uint32 dwSet) { dwVal = dwSet; }

    uint32  dwVal;
    uint16  wVal;
    uint8   bVal;
};

struct LTVector
{
	LTFLOAT x;
	LTFLOAT y;
	LTFLOAT z;
};

struct LTRotation
{
	float	m_fQuat[4];
};

struct LTLink
{
	LTLink*	m_pPrev;
	LTLink*	m_pNext;
	
	void*	m_pData;
};

struct CheapLTLink
{
	LTLink*	m_pPrev;
	LTLink*	m_pNext;
};

struct LTList
{
	uint32	m_nElements;
	LTLink	m_Head;
};

typedef void* HLTFileTree;
typedef void* HLTPARAM;
typedef void* HLTBUFFER;

struct LTransform
{
	LTVector	m_vPos;
	LTRotation	m_rRot;
};

struct LTRect
{
	int	m_nLeft;
	int	m_nTop;
	int	m_nRight;
	int m_nBottom;

	bool CompareTo(LTRect* pOther)
	{
		return m_nLeft == pOther->m_nLeft && m_nTop == pOther->m_nTop && m_nRight == pOther->m_nRight && m_nBottom == pOther->m_nBottom;
	}
};

struct LTWarpPt
{
	float	m_fSourceX;
	float	m_fSourceY;
	float	m_fDestX;
	float	m_fDestY;

	bool CompareTo(LTWarpPt* pOther)
	{
		return m_fSourceX == pOther->m_fSourceX && m_fSourceY == pOther->m_fSourceY && 
			m_fDestX == pOther->m_fDestX && m_fDestY == pOther->m_fDestY;
	}
};

struct FileIdentifier
{
    void	*m_pData;
    LTLink	m_Link;
	
    HLTFileTree	m_hFileTree;
	
    uint16	m_wFileID;
    uint16	m_wNameLen;
    uint8	m_nTypeCode;
    uint8	m_nFlags;
	
    char*	m_szFilename;
};

struct SkyDef
{
	LTVector	m_vMin;
	LTVector	m_vMax;
	LTVector	m_vViewMin;
	LTVector	m_vViewMax;
};

struct ModelHookData
{
	LTObject*	m_pObject;

	uint32	m_dwHookFlags;
	uint32	m_dwObjectFlags;

	LTVector*	m_pLightAdd;
	LTVector*	m_pObjectColor;
};

struct LTPlane
{
	LTVector	m_vNormal;
	float		m_fDist;
};

struct LTMatrix
{
	float	m[4][4];
};

struct ModelInstanceHookData
{
	uint32		m_dwFlags;
	LTPlane		m_ClipPlane;
};

struct DefaultCache
{
	uint32	m_dwCacheSize;
	uint32	m_dwWantedCache;
};

struct NoCache
{
	uint32	m_dwPadding;
};

template <typename T, typename C>
struct CMoArray
{
	uint32*	m_pVTable;

	T*		m_pArray;
	uint32	m_dwElements;			
	C		m_Cache;
};

struct ILTStream
{
	uint32*	m_pVTable;
};

struct Leech;
struct Nexus
{
	Leech*	m_pLeechHead;
    void*	m_pData; 
};

struct LTRGB
{
	uint8	b;
	uint8	g;
	uint8	r;
	uint8	a;
};

struct PGColor
{
	float	r;
	float	g;
	float	b;
	float	a;
};

typedef LTBOOL(*ObjectFilterFn)(LTObject* pObj, void* pUserData);
typedef LTBOOL(*PolyFilterFn)(HPOLY hPoly, void* pUserData);

class IntersectQuery
{

public:

	IntersectQuery()
	{
		m_dwFlags = 0;
		m_FilterFn = nullptr;
		m_PolyFilterFn = nullptr;
		m_pUserData = nullptr;

		m_vFrom = { };
		m_vTo = { };
		m_vDirection = { };
	}

	LTVector	m_vFrom;
	LTVector	m_vTo;

	LTVector	m_vDirection;

	uint32	m_dwFlags;

	ObjectFilterFn	m_FilterFn;
	PolyFilterFn	m_PolyFilterFn;

	void*	m_pUserData;
};


struct IntersectInfo
{
	IntersectInfo()
	{
		m_vPoint = { };
		m_Plane.m_vNormal = { };
		m_Plane.m_fDist = 0.0f;
		m_pObject = nullptr;
		m_hPoly = INVALID_HPOLY;
		m_dwSurfaceFlags = 0;
	}

	LTVector	m_vPoint;
	LTPlane		m_Plane;
	LTObject*	m_pObject;

	HPOLY	m_hPoly;

	uint32	m_dwSurfaceFlags;
};

#endif