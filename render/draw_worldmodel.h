#ifndef __DRAW_WORLD_MODEL_H__
#define __DRAW_WORLD_MODEL_H__

#ifndef __D3D11_H__
#include <d3d11_1.h>
#define __D3D11_H__
#endif

#ifndef __DIRECT_X_MATH_H__
#include <DirectXMath.h>
#define __DIRECT_X_MATH_H__
#endif

#include "common_datamgr.h"
#include "d3d_vertextypes.h"
#include "common_stuff.h"

enum BlendState;
class ViewParams;
class ObjectDrawList;
struct LMVertexData;

#define WORLD_MODEL_TEXTURES_PER_DRAW		12
#define SKY_PORTAL_BBOX_VERTICES			8
#define SKY_PORTAL_BBOX_INDICES				36

typedef VertexTypes::PositionNormalColorTexture3Index4 InternalWorldModelVertex;
typedef VertexTypes::Texture InternalWorldModelVertexFX;
typedef std::vector<InternalWorldModelVertex> Array_InternalWorldModelVertex;
typedef VertexTypes::Position InternalSkyPortalVertex;
typedef std::vector<InternalSkyPortalVertex> Array_InternalSkyPortalVertex;

enum WorldModelTextureIndex
{
	WMTI_Main = 0,
	WMTI_Extra = 1,
	WMTI_LMDataStart,
	WMTI_VertexFXStart
};

class CWorldModelData : public CExpirableData
{

public:

	struct BBox
	{
		DirectX::XMFLOAT3	m_vMin;
		DirectX::XMFLOAT3	m_vMax;
	};

	typedef std::vector<BBox> Array_BBox;

	struct PolyTextures
	{
		PolyTextures(Surface* pSurface)
		{
			Init(pSurface);
		}
		
		void Init(Surface* pSurface)
		{
			m_pSurface = pSurface;
		}

		Surface*	m_pSurface;;
	};

	struct PolyTextureIndices
	{
		PolyTextureIndices()
		{
			Clear();
		};

		void Clear()
		{
			m_dwMainIndex = 0;
			m_dwExtraIndex = 0;
			m_dwLMDataStart = UINT32_MAX;
		}

		uint32	m_dwMainIndex;
		uint32	m_dwExtraIndex;
		uint32	m_dwLMDataStart;
	};

	enum TextureType
	{
		TT_DETAIL,
		TT_ENVMAP,
		TT_ENVMAP_ALPHA,
		TT_MAIN,
		TT_LIGHTMAP
	};

	struct TextureInfo
	{
		TextureInfo(Surface* pSurface, ESharedTexType eType)
		{
			Init(pSurface, eType);
		}

		TextureInfo(Surface* pSurface, TextureType eType, uint32 dwExtraModeFlags)
		{
			Init(pSurface, eType, dwExtraModeFlags);
		}

		TextureInfo(uint32 dwLMTextureIndex)
		{
			Init(dwLMTextureIndex);
		}

		void Init(Surface* pSurface, ESharedTexType eType)
		{
			m_pSurface = pSurface;
			m_eType = (TextureType)eType;
			m_dwExtraModeFlags = 0;
		}

		void Init(Surface* pSurface, TextureType eType, uint32 dwExtraModeFlags)
		{
			m_pSurface = pSurface;
			m_eType = eType;
			m_dwExtraModeFlags = dwExtraModeFlags;
		}

		void Init(uint32 dwLMTextureIndex)
		{
			m_dwLMTextureIndex = dwLMTextureIndex;
			m_eType = TT_LIGHTMAP;
			m_dwExtraModeFlags = 0;
		}
		
		union
		{
			Surface*	m_pSurface;
			uint32		m_dwLMTextureIndex;
		};
		
		TextureType		m_eType;

		uint32	m_dwExtraModeFlags;
	};

	typedef std::vector<TextureInfo> Array_TextureInfo;

	struct VertexFX
	{
		VertexFX(SharedTexture* pTexture, SPolyVertex* pVertex)
		{
			Init(pTexture, pVertex);
		}

		void Init(SharedTexture* pTexture, SPolyVertex* pVertex)
		{
			m_pTexture = pTexture;
			m_pVertex = pVertex;
		}

		SharedTexture*	m_pTexture;
		SPolyVertex*	m_pVertex;
	};

	typedef std::vector<VertexFX> Array_VertexFX;

	struct RenderBlock
	{
		RenderBlock()
		{
			Init();
		}

		void Init()
		{
			m_dwIndexPos = UINT32_MAX;
			m_dwIndexCount = UINT32_MAX;

			m_BBox = { };

			m_pVertexFXBuffer = nullptr;
			m_pVertexFXView = nullptr;
		}

		void XM_CALLCONV SetBBox(DirectX::FXMVECTOR vMin, DirectX::FXMVECTOR vMax)
		{
			DirectX::XMStoreFloat3(&m_BBox.m_vMin, vMin);
			DirectX::XMStoreFloat3(&m_BBox.m_vMax, vMax);
		}

		void XM_CALLCONV AddExtraBBox(DirectX::FXMVECTOR vMin, DirectX::FXMVECTOR vMax)
		{
			m_aExtraBBox.emplace_back();

			DirectX::XMStoreFloat3(&m_aExtraBBox.back().m_vMin, vMin);
			DirectX::XMStoreFloat3(&m_aExtraBBox.back().m_vMax, vMax);
		}

		void XM_CALLCONV AppendExtraBBox(DirectX::FXMVECTOR vMin, DirectX::FXMVECTOR vMax);

		void OptimizeExtraBBoxes();

		bool IsVisibleInFrustum(ViewParams* pParams);

		void SetVertexData(uint32 dwIndexPos, uint32 dwIndexCount, uint32 dwVertexCount)
		{
			m_dwIndexPos = dwIndexPos;
			m_dwIndexCount = dwIndexCount;
			m_dwVertexCount = dwVertexCount;

			if (m_aVertexFXData.size())
				CreateVertexFXBuffer();
		}

		bool CreateVertexFXBuffer();
		bool UpdateVertexFXBuffer();

		BBox		m_BBox;
		Array_BBox	m_aExtraBBox;

		uint32	m_dwIndexPos;
		uint32	m_dwIndexCount;

		uint32	m_dwVertexCount;

		Array_TextureInfo	m_aTextureInfo;

		Array_VertexFX				m_aVertexFXData;
		ID3D11Buffer*				m_pVertexFXBuffer;
		ID3D11ShaderResourceView*	m_pVertexFXView;
	};

	typedef std::vector<RenderBlock> Array_RenderBlock;

	CWorldModelData() : CExpirableData()
	{
#if defined(DEBUG) || defined(_DEBUG)
		m_szName[0] = 0;
#endif
		m_pVertexBuffer = nullptr;
		m_pIndexBuffer = nullptr;
		m_pLMVertexData = nullptr;
		m_dwVerts = 0;
		m_dwIndices = 0;
		m_fAlphaMod = 1.0f;
	}

	~CWorldModelData();

	bool	Init(WorldBsp* pBsp);

	uint32	GetVertexCount() { return m_dwVerts; }
	uint32	GetIndexCount() { return m_dwIndices; }
	float	GetAlphaMod() { return m_fAlphaMod; }

#if defined(DEBUG) || defined(_DEBUG)
	char	m_szName[MAX_WORLDNAME_LEN + 1];
#endif

	ID3D11Buffer*	m_pVertexBuffer;
	ID3D11Buffer*	m_pIndexBuffer;

	ID3D11ShaderResourceView*	m_pLMVertexData;

	Array_RenderBlock	m_aRenderBlock;

private:

	bool	CreateVertexBuffer(InternalWorldModelVertex* pData, uint32 dwSize);
	bool	CreateIndexBuffer(uint32* pData, uint32 dwSize);
	bool	CreateLightmapDataBuffer(LMVertexData* pData, uint32 dwSize);
	
	uint32	m_dwVerts;
	uint32	m_dwIndices;

	float	m_fAlphaMod;
};

class CWorldModelManager : public CExpirableDataManager2<CWorldModelData*>
{

public:

	CWorldModelManager() : CExpirableDataManager2()
	{
		m_pSkyPortal_VB = nullptr;
		m_pSkyPortal_IB = nullptr;
		m_pSkyPortalBBox_VB = nullptr;
		m_pSkyPortalBBox_IB = nullptr;
		m_dwSkyPortalVerts = 0;
		m_dwSkyPortalIndices = 0;
		m_SkyPortalBBox = { };
	}

	virtual	void FreeAllData();

	CWorldModelData* GetWorldModelData(WorldBsp* pBsp);

	void	ReserveData();

	uint32	GetSkyPortalVertexCount() { return m_dwSkyPortalVerts; }
	uint32	GetSkyPortalIndexCount() { return m_dwSkyPortalIndices; }

	void	SetSkyPortalBuffersAndTopology();

	CWorldModelData::BBox*	GetSkyPortalBBox() { return &m_SkyPortalBBox; }

	void XM_CALLCONV SetSkyPortalBBox(DirectX::FXMVECTOR vMin, DirectX::FXMVECTOR vMax)
	{
		DirectX::XMStoreFloat3(&m_SkyPortalBBox.m_vMin, vMin);
		DirectX::XMStoreFloat3(&m_SkyPortalBBox.m_vMax, vMax);
	}

	ID3D11Buffer*	m_pSkyPortalBBox_VB;
	ID3D11Buffer*	m_pSkyPortalBBox_IB;

private:

	bool	InitSkyPortalBuffers();

	bool	CreateSkyPortalVertexBuffer(InternalSkyPortalVertex* pData, uint32 dwSize);
	bool	CreateSkyPortalIndexBuffer(uint16* pData, uint32 dwSize);
	bool	CreateSkyPortalBBoxVertexBuffer(InternalSkyPortalVertex* pData, uint32 dwFullSize);
	bool	CreateSkyPortalBBoxIndexBuffer(uint16* pData, uint32 dwFullSize);

	ID3D11Buffer*	m_pSkyPortal_VB;
	ID3D11Buffer*	m_pSkyPortal_IB;

	uint32		m_dwSkyPortalVerts;
	uint32		m_dwSkyPortalIndices;

	CWorldModelData::BBox	m_SkyPortalBBox;
};

bool IsCameraInsideBsp(ViewParams* pParams, WorldBsp* pBsp);
float DistanceToBspEst(ViewParams* pParams, WorldBsp* pBsp);
void d3d_ProcessWorldModel(LTObject* pObject);
bool d3d_IsTranslucentWorldModel(LTObject* pObject);
void d3d_DrawSkyPortals(ViewParams* pParams, DirectX::XMFLOAT4X4* pTransform);
void d3d_DrawWorldModel(ViewParams* pParams, LTObject* pObject, BlendState eDefaultBlendState, uint32 dwExtraFlags);
void d3d_DrawWorldModel(ViewParams* pParams, WorldBsp* pBsp, DirectX::XMFLOAT4X4* pTransform, uint32 dwFlags,
	uint32 dwFlags2, DirectX::XMFLOAT3* pPos, DirectX::XMFLOAT4* pDiffuseColor, BlendState eDefaultBlendState, 
	uint32 dwExtraModeFlags);
void d3d_DrawSolidWorldModels(ViewParams* pParams);
void d3d_DrawChromakeyWorldModels(ViewParams* pParams);
void d3d_QueueTranslucentWorldModels(ViewParams* pParams, ObjectDrawList* pDrawList);
bool d3d_CacheWorldModel(WorldBsp* pBsp);
bool d3d_CacheWorldModel(LTObject* pObject);

#endif
