#ifndef __LT_TEX_TYPES__
#define __LT_TEX_TYPES__

struct DtxSection;

enum BPPIdent 
{
    BPP_8P = 0, 
	BPP_8,
	BPP_16,
	BPP_32,
	BPP_S3TC_DXT1,
	BPP_S3TC_DXT3,
	BPP_S3TC_DXT5,
	BPP_32P,
	BPP_24,
	NUM_BIT_TYPES
};

enum ColorPlanes
{
	CP_ALPHA = 0,
	CP_RED = 1,
	CP_GREEN = 2,
	CP_BLUE = 3,
	NUM_COLORPLANES = 4,
};

struct PFormat
{
	uint32	m_dwVTable;

	void	Init(BPPIdent eType, uint32 dwAMask, uint32 dwRMask, uint32 dwGMask, uint32 dwBMask);
	
	BPPIdent	m_eType;
	
    uint32	m_adwMasks[NUM_COLORPLANES];
    uint32	m_adwBits[NUM_COLORPLANES];
    uint32	m_adwFirstBits[NUM_COLORPLANES];
};

enum ESharedTexType
{
	eSharedTexType_Detail = 0,
	eSharedTexType_EnvMap,
	eSharedTexType_EnvMapAlpha
};

struct SharedTexture
{
	Nexus	m_Nexus; // 0 ?
	
    void*	m_pEngineData; // 4
    void*	m_pRenderData; // 8
	
	LTLink	m_Link; // 16
	
	uint32	m_adwData_28[2]; // 28 ? 8 bytes of something
	
	FileIdentifier*	m_pFile; // 36
	
	SharedTexture*	m_pLinkedTexture; // 40
	ESharedTexType	m_eTexType; // 44
	
	uint16	m_wFrameCode; // 48 ? used in WasTextureDrawnLastFrame
	uint16	m_wFlags; // 50
	
	StateChange*	m_pStateChange; // 52 ?

	char*	m_szCommandString; // 56
	
	uint32	m_dwMipMapOffsetED; // 60 ?

	// 64
};

struct BaseResHeader
{
    uint32	m_dwType;
};

#pragma pack(push, 1)
struct DtxExtra
{
    uint8	m_nTexGroup;
	uint8	m_nMipMapsInUse;
	uint8	m_nBPPIndent;
	uint8	m_nMipMapOffsetNC;
	uint8	m_nMipMapOffsetED;
	uint8	m_nTexPriority;
	float	m_fDetTexScale;
	int16	m_nDetTexAngle;
};
#pragma pack(pop)

struct DtxHeader
{
	inline uint32 GetBPPIdent() { return m_Extra.m_nBPPIndent == 0 ? BPP_32 : m_Extra.m_nBPPIndent; }

    uint32	m_dwResType;
    int32	m_nVersion;
	
    uint16  m_wBaseWidth;
	uint16	m_wBaseHeight;
	
    uint16  m_wMipmaps;
    uint16  m_wSections;
	
    int32	m_nIFlags;
    int32   m_nUserFlags;
	
	DtxExtra	m_Extra;
	
    char    m_szCommandString[DTX_COMMANDSTRING_LEN];
};

struct TextureMipData
{
    uint32	m_dwWidth;
	uint32	m_dwHeight;
	
    uint8*	m_pData;
	int32	m_nPitch;
	
    uint32	m_dwData_16;
    uint32	m_dwData_20;
};

struct TextureData
{
	uint32	m_dwVTable; // 0
    //virtual void SetupPFormat(PFormat* pFormat);
	
	BaseResHeader	m_ResHeader;
    DtxHeader		m_DtxHeader;
	
    LTLink	m_Link; // 172
    uint32	m_dwAllocSize; // 184
	
	DtxSection*	m_pDtxSections; // 188
	
    SharedTexture*	m_pSharedTexture;
    uint32			m_dwFlags;	
    uint8*			m_pDataBuffer;
	
    TextureMipData	m_Mips[MAX_DTX_MIPMAPS];
};

struct GlobalPanInfo
{
	SharedTexture*	m_pTexture;
	
	float	m_fCurSkyXOffset;
	float	m_fCurSkyZOffset;
	float	m_fPanSkyScaleX;
	float	m_fPanSkyScaleZ;
};

#endif
