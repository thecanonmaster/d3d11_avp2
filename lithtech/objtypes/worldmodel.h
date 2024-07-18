struct MainWorld;
struct WorldBsp;
struct TerrainSection;
struct Surface;

#pragma pack(push, 1)
struct PBlockData
{
	uint16	m_wData_0; // 0
	uint16	m_wData_2; // 2
	uint16	m_wData_4; // 4

	// 6 max
};
#pragma pack(pop)

struct PBlock
{
	PBlockData*	m_pData; // 0

	uint16	m_wSize; // 4
	uint16	m_wData_6; // 6 ?

	// 8 max?
};

struct PBlockTable
{
	LTVector	m_vData_0; // 0 ?
	LTVector	m_vData_12; // 12 ?

	uint32	m_dwSizeX; // 24
	uint32	m_dwSizeY; // 28
	uint32	m_dwSizeZ; // 32

	uint32	m_dwSizeXY; // 36

	uint32	m_dwBlocks; // 40
	PBlock*	m_pBlocks; // 44
};

struct SurfaceData
{
	LTVector	m_vO;
	LTVector	m_vP;
	LTVector	m_vQ;
	
	MainWorld*	m_pInternalWorld;
	WorldBsp*	m_pInternalWorldBsp;
	
	Surface*	m_pInternalSurface;
};

struct SurfaceFXDescInternal
{
	void*	(*InitEffect)(SurfaceData* pSurfaceData, int argc, char **argv);
	void	(*UpdateEffect)(SurfaceData* pSurfaceData, void* pData);
	void	(*TermEffect)(void* pData);
	
	SurfaceFXDescInternal*	m_pPrev;
	
	char	m_szName[1];
};

struct CommonEffect
{
	LTVector	m_vO; // 0
	LTVector	m_vP; // 12
	LTVector	m_vQ; // 24

	LTVector	m_vData_36; // 36 ?
	
	float	m_fParam1; // 48 ?
	float	m_fParam2; // 52 ?
	float	m_fParam3; // 56 ?
	float	m_fParam4; // 60 ?

	// 64 max
};

struct SurfaceEffect
{
	WorldBsp*	m_pWorldBsp; // 0
	
	Surface*	m_pSurface; // 4

	CommonEffect*	m_pCommonEffect; // 8

	SurfaceFXDescInternal*	m_pDesc; // 12
	
	SurfaceEffect*	m_pPrev; // 16
};

struct Surface
{
	LTVector	m_vO; // 0
	LTVector	m_vP; // 12
	LTVector	m_vQ; // 24

	Nexus	m_Nexus; // 36

	SharedTexture*	m_pTexture; // 44

	uint32	m_dwFlags; // 48

	uint16	m_wTextureFlags; // 52
	uint16	m_wTexture; // 54

	uint16	m_wPoly; // 56 ? 0xFFFF index
	uint16	m_wPortalIDAndFlags; // 58 ? 0x8000 - for mirror overlay

	uint8	m_nData_60; // 60 ?
	uint8	m_nData_61; // 61 ? padding
	uint16	m_wData_62; // 62 ? padding
			
	// 64 max
};

struct Vertex
{
	LTVector	m_vVec;

	// 12 max?
};

struct SPolyVertex
{
	Vertex*	m_pPoints; // 88
	
	float	m_fScaledU; // 92
	float	m_fScaledV; // 96

	float	m_fLightmapU; // 100
	float	m_fLightmapV; // 104
	
	uint8	m_nColorR; // 108 ?
	uint8	m_nColorG; // 109 ?
	uint8	m_nColorB; // 110 ?
	uint8	m_nColorA; // 111 ? 0xFF
	
	// 24 max?
};

struct PolyAnimRef
{
	uint16	m_wRefs[2];
};

struct WorldPoly
{
	uint32	m_adwData_0[3]; // 0 ?

	PolyAnimRef*	m_pPolyAnimRefs; // 12
	uint32			m_dwPolyAnimRefs; // 16

	uint16	m_wLMPlaneAndFlags; // 20 ?
	
	uint16	m_wIndex; // 22

	LTVector	m_vCenter; // 24 ?
	float		m_fRadius; // 36 ?

	LTPlane*	m_pPlane; // 40

	Surface*	m_pSurface; // 44

	uint32	m_adwData_48[2]; // 48 ?

	LTVector	m_vLMCenter; // 56

	uint16	m_wPrevPoly; // 68 ? 0xFFFF
	uint16	m_wFrameCode; // 70 ?	

	void*	m_pLMData; // 72 ?

	uint8	m_nLightmapWidth; // 76
	uint8	m_nLightmapHeight; // 77
	uint8	m_nLMPageX; // 78 ?
	uint8	m_nLMPageY; // 79 ?

	SPolyVertex*	m_pPolyVertexStart; // 80 ?

	uint16	m_wNumVerts; // 84
	uint16	m_wNumFixVerts; // 86

	// 88 max?

	SPolyVertex	m_Vertices[1];

	// 112 max?
};

struct Node
{
    WorldPoly*	m_pPoly; // 0

    Node*	m_pSides[2]; // 4 

	CheapLTLink	m_Link; // 12 ?

	uint16	m_wLeaf; // 20
    uint8	m_nFlags; // 22 ?
    uint8	m_nPlaneType; // 23
};

struct UnkStruct144
{
	uint16	m_awData[2];
};

struct LeafList
{
	uint16	m_wPortalID; // 0
	
	uint16	m_wSize; // 2

	uint8*	m_pVisData; // 4

	// 8 max?
};

struct LeafLinkData
{
	uint32		m_adwData0[6];
	LTObject*	m_pObject; // 24
};

struct Leaf
{
	uint32	m_adwData_0[4]; // 0
	
	LeafList*	m_pLists; // 16

	LTLink	m_Link; // 20 ?

	WorldPoly**	m_pPolies; // 32
	uint32		m_dwPolies; // 36

	float	m_fData_40; // 40 ?

	uint16	m_wFrameCode; // 44 frame code?

	uint16	m_nLists; // 46 ? not index

	// 48 max?
};

struct UserPortal
{
	char*	m_szName; // 0

	uint16	m_wData_4; // 4 ?
	uint16	m_wData_6; // 6 ? padding

	uint16	m_wPortalID; // 8
	uint16	m_wData_10; // 10 ? padding

	LTVector	m_vCenter; // 12 ?
	LTVector	m_vDims; // 24 ?
	
	// 56 or 36 max?
};

struct WorldBsp
{
	uint32*	m_pVTable;

	char	m_szWorldName[MAX_WORLDNAME_LEN + 1]; // 4
	uint8	m_nWorldNamePadding0; // 69
	uint16	m_wWorldNamePadding1; // 70

	uint32	m_dwWorldInfoFlags; // 72

	uint32	m_dwData_76; // 76 ? inherit flags

	uint32	m_dwMemoryUse; // 80 ?

	CMoArray<PolyAnimRef, DefaultCache>	m_PolyAnimRefs; // 84

	LTPlane*	m_pPlanes; // 104
    uint32		m_dwPlanes; // 108

	Node*	m_pNodes; // 112
	uint32	m_dwNodes; // 116

	Surface*	m_pSurfaces; // 120
    uint32		m_dwSurfaces; // 124

	LeafList*	m_pLeafLists; // 128
	uint32		m_dwLeafLists; // 132

	Leaf*		m_pLeafs; // 136
	uint32		m_dwLeafs; // 140

	UnkStruct144*	m_pUnkStructs144; // 144
	uint32			m_dwUnkStructs144; // 148

	uint32	m_dwTotalVisListSize; // 152 ?

	Node*	m_pRootNode; // 156

	WorldPoly**	m_pPolies; // 160
    uint32		m_dwPolies; // 164    

	Vertex*	m_pPoints; // 168
	uint32	m_dwPoints; // 172

	UserPortal*	m_pUserPortals; // 176
	uint32	m_dwUserPortals; // 180

	char*	m_szTextureNameData; // 184
    char**	m_szTextureNames; // 188
	uint32	m_dwTextures; // 192

	LTVector	m_vMinBox; // 196
	LTVector	m_vMaxBox; // 208

	uint32	m_dwData_220; // 220 ? from .DAT after WIF

	uint32	m_adwData_224[2]; // 224

	LTVector	m_vWorldTranslation; // 232 ?

	uint32	m_dwData_244; // 244 ? pointer to struct
	uint16	m_wIndex; // 248 ?
	uint16	m_wData_250; // 250 ? padding

	uint8*	m_pPolyData; // 252
	uint32	m_dwPolyDataSize; // 256

	uint8*	m_pVisListData; // 260
	
	PBlockTable	m_PBlockTable; // 264

	CMoArray<TerrainSection, DefaultCache>	m_TerrainSections; // 312

	float	m_fPolyClipRadius; // 332 ? max radius

	uint32	m_dwFrameCode; // 336 ?

	// 340 max

	HPOLY	MakeHPoly(Node* pNode)
	{
		return (pNode->m_pPoly->m_wIndex | m_wIndex << 16);
	}
};

struct WorldData
{
    uint32	m_dwFlags; // 0

	WorldBsp*	m_pOriginalBsp; // 4 ?
	WorldBsp*	m_pWorldBsp; // 8 ?
    WorldBsp*	m_pValidBsp; // 12 ?
    
    /*WorldBsp*	m_pWorldBsp;	
    WorldBsp*	m_pValidBsp;
    WorldBsp*	m_pOriginalBsp;*/
};

struct WorldModelInstance
{
	LTObject	m_Base;

	WorldBsp*	m_pOriginalBsp; // 432
    WorldBsp*	m_pWorldBsp; // 436
    WorldBsp*	m_pValidBsp; // 440

	LTLink		m_Links[5]; // 444 ? MAX_OBJ_NODE_LINKS

	LTMatrix	m_mTransform; // 504
    LTMatrix	m_mBackTransform; // 568

	inline bool IsMainWorldModel()
	{
		return (m_pOriginalBsp->m_dwWorldInfoFlags & WIF_MAINWORLD);
	}
};