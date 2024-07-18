#ifndef __D3D_LIGHTMAP_H__
#define __D3D_LIGHTMAP_H__

#ifndef __D3D11_H__
#include <d3d11_1.h>
#define __D3D11_H__
#endif

#ifndef __DIRECT_X_MATH_H__
#include <DirectXMath.h>
#define __DIRECT_X_MATH_H__
#endif

#define LIGHTMAP_MAX_DATA_SIZE	1024
#define LIGHTMAP_PAGE_DIMS		1024
#define LIGHTMAP_PAGE_DIMS_F	1024.0f

#define LMVD_FLAG_USE_COLORS	(1<<0)
#define LMVD_FLAG_USE_TEXTURE	(1<<1)

static const char* g_szLMDecompessionError = "Failed to decompress lightmap (AN = %d, FR = %d, DA = %d)";
static const char* g_szLMCreatePagesError = "Failed to create shader view for lightmap pages";

struct LMVertexData
{
	LMVertexData()
	{	
		m_vColor = { };
		m_vTexCoordOffsets = { };
		m_dwPage = UINT32_MAX;
		m_dwFlags = 0;
	}

	LMVertexData(float fRed, float fGreen, float fBlue, uint32 dwPage, uint32 dwFlags,
		float fOffsetU, float fOffsetV)
	{
		Init(fRed, fGreen, fBlue, dwPage, dwFlags, fOffsetU, fOffsetV);
	}

	void Init(float fRed, float fGreen, float fBlue, uint32 dwPage, uint32 dwFlags, 
		float fOffsetU, float fOffsetV)
	{
		m_vColor = { fRed, fGreen, fBlue };
		m_vTexCoordOffsets = { fOffsetU, fOffsetV };
		m_dwPage = dwPage;
		m_dwFlags = dwFlags;
	}

	DirectX::XMFLOAT3	m_vColor;
	DirectX::XMFLOAT2	m_vTexCoordOffsets;

	uint32	m_dwPage;
	uint32	m_dwFlags;
};

typedef std::vector<LMVertexData> Array_LMVertexData;

struct LMPagedPoly
{
	struct FrameInfo
	{
		FrameInfo()
		{
			m_dwPage = UINT32_MAX;
			m_fOffsetU = 0.0f;
			m_fOffsetV = 0.0f;
		}

		FrameInfo(uint16 dwPage, float fOffsetU, float fOffsetV)
		{
			Init(dwPage, fOffsetU, fOffsetV);
		}

		void Init(uint32 dwPage, float fOffsetU, float fOffsetV)
		{
			m_dwPage = dwPage;

			m_fOffsetU = fOffsetU;
			m_fOffsetV = fOffsetV;
		}
		
		uint32	m_dwPage;

		float	m_fOffsetU;
		float	m_fOffsetV;
	};

	typedef std::vector<FrameInfo> Array_FrameInfo;
	
	LMPagedPoly(uint16 wModelIndex, uint16 wPolyIndex, uint32 dwFrameCount)
	{
		Init(wModelIndex, wPolyIndex, dwFrameCount);
	}

	void Init(uint16 wModelIndex, uint16 wPolyIndex, uint32 dwFrameCount)
	{
		m_wModelIndex = wModelIndex;
		m_wPolyIndex = wPolyIndex;

		m_aFrameInfo.resize(dwFrameCount);
	}

	void SetFrameInfo(uint32 dwFrame, uint32 dwPage, float fOffsetU, float fOffsetV)
	{
		m_aFrameInfo[dwFrame].Init(dwPage, fOffsetU, fOffsetV);
	}

	uint16	m_wModelIndex;
	uint16	m_wPolyIndex;

	Array_FrameInfo	m_aFrameInfo;
};

typedef std::unordered_map<uint32, LMPagedPoly*> Map_PLMPagedPoly;

class CLightMapManager
{

public:

	struct LMTempPage
	{
		void Init(uint32 dwPosX, uint32 dwPosY, uint32 dwRowHeight)
		{
			m_dwNextPosX = dwPosX;
			m_dwNextPosY = dwPosY;
			m_dwRowHeight = dwRowHeight;
		}

		void InitPixels()
		{
			m_pPixels = (uint32*)calloc(LIGHTMAP_PAGE_DIMS * LIGHTMAP_PAGE_DIMS, sizeof(uint32));
		}

		void TermPixels()
		{
			free(m_pPixels);
		}

		uint32* m_pPixels;

		uint32	m_dwNextPosX;
		uint32	m_dwNextPosY;

		uint32	m_dwRowHeight;
	};

	typedef std::vector<LMTempPage> Array_LMTempPage;

	CLightMapManager()
	{
		m_bInitialized = false;
		m_dwAllFrameCount = 0;
		m_pPages = nullptr;
	}

	~CLightMapManager()
	{
		if (m_bInitialized)
			Term();
	}

	void	FreeAllData();

	void	CreateLightmapPages(LMAnim* pAnims, uint32 dwAnimCount);

	void	Init();
	void	Term();

	ID3D11ShaderResourceView*	CreateLMVertexDataBuffer(void* pData, uint32 dwSize);

	uint32	GetAllFrameCount() { return m_dwAllFrameCount; }

	DirectX::XMFLOAT2	SetupBaseLMVertexTexCoords(LMPagedPoly* pPagedPoly, WorldPoly* pPoly, uint32 dwVert,
		DirectX::XMFLOAT3* pLMVectorU, DirectX::XMFLOAT3* pLMVectorV);

	Map_PLMPagedPoly	m_PagedPolies;

	ID3D11ShaderResourceView*	m_pPages;

private:

	void	CalcAllFrameCount(LMAnim* pAnims, uint32 dwAnimCount);

	LMPagedPoly*	GetPagedPoly(uint16 wBspIndex, uint16 wPolyIndex);

	void	AddToPage(uint32* pPixels, uint32 dwWidth, uint32 dwHeight, LMPagedPoly* pPagedPoly, 
		uint32 dwFrame);

	void	RemoveTempData();

	ID3D11ShaderResourceView*	CreatePages();

	bool	m_bInitialized;

	uint32	m_dwAllFrameCount;

	Array_LMTempPage	m_aTempPage;
};

void SetupLMPlaneVectors(uint32 dwIndex, DirectX::XMFLOAT3* pNormal, DirectX::XMFLOAT3* pOut1,
	DirectX::XMFLOAT3* pOut2);

inline bool IsValidLMPlane(WorldPoly* pPoly)
{
	// TODO - no idea
	return !(pPoly->m_wLMPlaneAndFlags & 0x3F);
}

inline uint16 GetLMPlaneIndex(WorldPoly* pPoly)
{
	// TODO - no idea
	return (pPoly->m_wLMPlaneAndFlags >> 11) & 7;
}

#endif