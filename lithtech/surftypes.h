#ifndef __LT_SURF_TYPES__
#define __LT_SURF_TYPES__

#define SURF_SOLID				(1<<0)
#define SURF_NONEXISTANT		(1<<1)
#define SURF_INVISIBLE			(1<<2)
#define SURF_TRANSPARENT		(1<<3)
#define SURF_SKY				(1<<4)
#define SURF_BRIGHT				(1<<5)
#define SURF_FLATSHADE			(1<<6)
#define SURF_LIGHTMAP			(1<<7)
#define SURF_NOSUBDIV			(1<<8)
#define SURF_HULLMAKER			(1<<9)
#define SURF_ALWAYSLIGHTMAP		(1<<10)
#define SURF_DIRECTIONALLIGHT	(1<<11)
#define SURF_GOURAUDSHADE		(1<<12)
#define SURF_PORTAL				(1<<13)
#define SURF_SPRITEANIMATE		(1<<14)
#define SURF_PANNINGSKY			(1<<15)
#define SURF_XZONLY				(1<<16)
#define SURF_PHYSICSFIX			(1<<17)

#define SURF_TERRAINOCCLUDER	(1<<18)
#define SURF_ADDITIVE			(1<<19)
#define SURF_TIMEOFDAY			(1<<20)
#define SURF_VISBLOCKER			(1<<21)
#define SURF_NOTASTEP			(1<<22)
#define SURF_NOWALLWALK			(1<<23)
#define SURF_NOBLOCKLIGHT		(1<<24)

#define LMPLANE_MASK	0xE0000000
#define LMPLANE_SHIFT	29

enum LTSurfaceBlend
{
	LTSURFACEBLEND_ALPHA = 0,
	LTSURFACEBLEND_SOLID = 1,
	LTSURFACEBLEND_ADD,
	LTSURFACEBLEND_MULTIPLY,
	LTSURFACEBLEND_MULTIPLY2,
	LTSURFACEBLEND_MASK,
	LTSURFACEBLEND_MASKADD
};

struct BlitRequest
{	
	HLTBUFFER	m_hBuffer;

	uint32			m_dwBlitOptions;
	GenericColor	m_dwTransparentColor;

	LTRect*	m_pSrcRect;
	LTRect*	m_pDestRect;

	float	m_fAlpha;
	
	LTWarpPt*	m_pWarpPts;
	int			m_nWarpPts;
};

#endif
