#ifndef __LT_MAIN_WORLD__
#define __LT_MAIN_WORLD__

struct WorldPoly;
struct Surface;
struct MainWorld;

#define MWF_UNKNOWN				(1<<0)
#define MWF_VIS_BSP_PRESENT		(1<<1)
#define MWF_BASE_LA_PRESENT		(1<<2)

struct LMPolyRef
{
	uint16	m_wModel;
	uint16	m_wPoly;
};

struct LMFramePolyData
{
	uint8*	m_pData; // 0
	uint16	m_wSize; // 4
	uint16	m_wData_6; // 6 ? padding

	uint8*	m_pReds; // 8
	uint8*	m_pGreens; // 12
	uint8*	m_pBlues; // 16
	uint8	m_nVertices; // 20

	uint8	m_nData_21; // 21 ? padding
	uint16	m_wData_22; // 22 ? padding
};

struct LMFrame
{
	LMFramePolyData*	m_pPolyDataList;
};

struct LMAnim
{
	char	m_szName[MAX_LIGHTANIMNAME_LEN];

	LTBOOL	m_bShadowMap; // 32

	LMFrame*	m_pFrames; // 36
	uint8		m_nFrames; // 40
	uint8		m_nData_41; // 41 ? padding
	uint16		m_wData_42; // 42 ? padding

	LMPolyRef*	m_pPolyRefs; // 44
	uint16		m_wPolyRefs; // 48
	uint16		m_wData_50; // 50 ? padding

	uint32	m_dwBetweenFrames[2]; // 52
	float	m_fPercentBetween; // 60
	float	m_fBlendPercent; // 64

	LTVector	m_vLightPos; // 68
	LTVector	m_vLightColor; // 80

	float	m_fLightRadius; // 92
};

struct LTRGBColor
{
	union
	{
		LTRGB	rgb;
		uint32	dwVal;
	};
};

struct LightTable
{
	LTRGBColor*	m_pLookup;
	uint32		m_dwFullLookupSize; // 4
	uint32		m_adwLookupSize[3]; // 8
	int32		m_anLookupSizeMinusOne[3]; // 20
	uint32		m_dwXSizeTimesYSize; // 32
	
	LTVector	m_vBlockSize; // 36
	LTVector	m_vReciBlockSize; // 48
	LTVector	m_vLookupStart; // 60
};

struct MainWorld
{
	uint32*	m_pVTable;

	CMoArray<LMAnim, DefaultCache>			m_LMAnims; // 4
	CMoArray<LMPolyRef, DefaultCache>		m_LMPolyRefs; // 24
	CMoArray<LMFrame, DefaultCache>			m_LMFrames; // 44
	CMoArray<uint8, DefaultCache>			m_LMData; // 64
	CMoArray<LMFramePolyData, DefaultCache>	m_LMFramePolyDataList; // 84

	uint32	m_dwRenderDataPos; // 104

	WorldTree	m_WorldTree; // 108

	StaticLightListHead	m_StaticLightLightList; // 232

	uint32	m_dwData_240; // 240 ?

	SurfaceEffect*	m_pSurfaceEffects; // 244

	float	m_fLMGridSize; // 248 ?

	LightTable	m_LightTable; // 252

	LTVector	m_vExtentsMin; // 324 ? sunlight color
	LTVector	m_vExtentsMax; // 336 ? sunlight dir
	LTVector	m_vExtentsDiffScale; // 348
	LTVector	m_vExtentsMinExtra; // 360
	LTVector	m_vExtentsMaxExtra; // 372
	
	uint32	m_dwFlags; // 384
	ILTStream*	m_pFileStream; // 388

	CMoArray<WorldData*, DefaultCache>	m_WorldModels; // 392
	CMoArray<WorldPoly*, DefaultCache>	m_Polies; // 412
	
	UserPortal*	m_pUserPortals; // 432 ?
	uint32		m_nUserPortals; // 436 ?

	char*	m_szWorldProperties; // 440

	LTLink	m_Link_444; // 444 - WorldData inside?

	uint32	m_dwData_456; // 456 ?

	LTBOOL	m_bLoaded; // 460 ?

	LTLink	m_Link_464; // 464
	LTLink	m_Link_476; // 476

	LTBOOL	m_bInherited; // 488 ?

	uint32	m_dwWorldVersion; // 492

	// 496

	WorldBsp*	GetSpecificBsp(uint32 dwTestFlags);

	uint32	GetLMAnimCount() { return m_LMAnims.m_dwElements; }
	LMAnim*	GetLMAnim(uint32 dwIndex) { return &m_LMAnims.m_pArray[dwIndex]; }

	uint32		GetWorldModelCount() { return m_WorldModels.m_dwElements; }
	WorldData*	GetWorldModelData(uint32 dwIndex) { return m_WorldModels.m_pArray[dwIndex]; }

	WorldPoly* GetPolyFromHPoly(HPOLY hPoly)
	{
		WorldBsp* pBsp = GetWorldModelData((hPoly >> 16) & 0xFFFF)->m_pOriginalBsp;

		return pBsp->m_pPolies[hPoly & 0xFFFF];
	}
};

#endif
