#ifndef __GLOBAL_MGR_H__
#define __GLOBAL_MGR_H__

#ifndef __DIRECT_X_MATH_H__
#include <DirectXMath.h>
#define __DIRECT_X_MATH_H__
#endif

#include "d3d_texture.h"
#include "draw_model.h"
#include "draw_sprite.h"
#include "draw_particles.h"
#include "draw_canvas.h"
#include "draw_polygrid.h"
#include "draw_linesystem.h"
#include "draw_worldmodel.h"
#include "d3d_device.h"
#include "d3d_lightmap.h"
#include "d3d_renderworld.h"
#include "d3d_optimizedsurface.h"
#include "rendererconsolevars.h"

//#define WORLD_TWEAKS_ENABLED

static const char* g_szFreeError_VB = "Failed to free D3D vertex buffer (count = %d)";
static const char* g_szFreeError_IB = "Failed to free D3D index buffer (count = %d)";

static const char* g_szFreeError_VB_LINE = "Failed to free D3D vertex buffer for lines (count = %d)";

static const char* g_szFreeError_IB_O2D = "Failed to free D3D index buffer for optimized 2D (count = %d)";
static const char* g_szFreeError_IB_SPR = "Failed to free D3D index buffer for sprites (count = %d)";

static const char* g_szExtrasResourceTag = "EXTRA";

typedef void(__cdecl* w_DoLightLookup_Type)(LightTable* pTable, LTVector* pPos, LTRGBColor* pRGB);
typedef bool(__cdecl* i_IntersectSegment_Type)(IntersectQuery* pQuery, IntersectInfo* pInfo, WorldTree* pWorldTree);

struct ILTStream_VTable
{
	void (_fastcall* ILTStream_Destructor)(ILTStream* pStream);
	void (_fastcall* ILTStream_Release)(ILTStream* pStream);
	uint32 (_fastcall* ILTStream_Read)(ILTStream* pStream, void* notUsed, void* pData, uint32 dwSize);
	uint32 (_fastcall* ILTStream_ReadString)(ILTStream* pStream, void* notUsed, char* pStr, uint32 dwMaxBytes);
	uint32 (_fastcall* ILTStream_ErrorStatus)(ILTStream* pStream);
	uint32 (_fastcall* ILTStream_SeekTo)(ILTStream* pStream, void* notUsed, uint32 dwOffset);
	uint32 (_fastcall* ILTStream_GetPos)(ILTStream* pStream, void* notUsed, uint32* pOffset);
	uint32 (_fastcall* ILTStream_GetLen)(ILTStream* pStream, void* notUsed, uint32* pLen);
};

#define WORLD_TWEAK_HIDE_SKY		(1<<0)
#define WORLD_TWEAK_HIDE_BSP		(1<<1)
#define WORLD_TWEAK_HIDE_TERRAIN	(1<<2)
#define WORLD_TWEAK_RB_DIST_TEST	(1<<3)
#define WORLD_TWEAK_OBJ_DIST_TEST	(1<<4)
#define WORLD_TWEAK_RB_CUT			(1<<5)

class CWorldTweakManager
{

public:

	struct RBCutData
	{
		uint32	m_dwIndex;

		char	m_nType;

		uint32	m_dwFrom;
		uint32	m_dwCount;
	};

	struct WorldTweak
	{
		WorldTweak(DirectX::XMFLOAT3* pBBoxMin, DirectX::XMFLOAT3* pBBoxMax)
		{
			m_vBBoxMin = *pBBoxMin;
			m_vBBoxMax = *pBBoxMax;

			m_dwFlags = 0;

			m_fRBDist = 0.0f;
			m_fObjDist = 0.0f;

			m_sRBCutData = { };
		}

		DirectX::XMFLOAT3	m_vBBoxMin;
		DirectX::XMFLOAT3	m_vBBoxMax;

		uint32	m_dwFlags;

		float	m_fRBDist;
		float	m_fObjDist;
		
		RBCutData	m_sRBCutData;
	};

	typedef std::vector<WorldTweak> Array_WorldTweak;

	CWorldTweakManager()
	{
		m_dwAllFlags = 0;
		m_vRB_BBoxMin = { };
		m_vRB_BBoxMax = { };
		m_vObj_BBoxMin = { };
		m_vObj_BBoxMax = { };

		m_pRBCutData = nullptr;
	}

	void	LoadWorldTweaks();

	void	InitFrameParams(DirectX::XMFLOAT3* pPos);
	void	CalcRBCutData(uint32 dwBaseIndexCount, uint32 dwBaseIndexPos, uint32& dwIndexCount1, uint32& dwIndexPos1,
		uint32& dwIndexCount2, uint32& dwIndexPos2);

	uint32	m_dwAllFlags;

	DirectX::XMFLOAT3	m_vRB_BBoxMin;
	DirectX::XMFLOAT3	m_vRB_BBoxMax;
	DirectX::XMFLOAT3	m_vObj_BBoxMin;
	DirectX::XMFLOAT3	m_vObj_BBoxMax;

	RBCutData* m_pRBCutData;

private:

	void	Clear() { m_aTweak.clear(); }

	void	LoadWorldTweaks(void* pConfigData, uint32 dwCRC32);

	Array_WorldTweak	m_aTweak;
};

struct BankedBuffer
{
	BankedBuffer(ID3D11Buffer* pBuffer, uint32 dwSize, bool bInUse)
	{
		Init(pBuffer, dwSize, bInUse);
	}

	void Init(ID3D11Buffer* pBuffer, uint32 dwSize, bool bInUse)
	{
		m_pBuffer = pBuffer;
		m_dwSize = dwSize;
		m_bInUse = bInUse;
	}

	ID3D11Buffer*	m_pBuffer;
	uint32			m_dwSize;
	bool			m_bInUse;
};

typedef std::vector<BankedBuffer> Array_BankedBuffer;

class CGlobalManager
{

public:

	struct EngineHacksStruct
	{
		void Init()
		{
			w_DoLightLookup = nullptr;
			i_IntersectSegment = nullptr;
		}

		w_DoLightLookup_Type	w_DoLightLookup;
		i_IntersectSegment_Type	i_IntersectSegment;
	};

	CGlobalManager();

	~CGlobalManager();

	bool	PostInit();

	void	FreeAll();
	void	FreeVertexBuffers();
	void	FreeIndexBuffers();

	void	FreeObjectManagersData();

	void	Update();

	CTextureManager*	GetTextureManager() { return m_pTextureMgr; }

	CModelManager*			GetModelMgr() { return m_pModelMgr; }
	CWorldModelManager*		GetWorldModelMgr() { return m_pWorldModelMgr; }
	CSpriteManager*			GetSpriteMgr() { return m_pSpriteMgr; }
	CParticleSystemManager* GetParticleSystemMgr() { return m_pParticleSystemMgr; }
	CCanvasManager*			GetCanvasMgr() { return m_pCanvasMgr; }
	CPolyGridManager*		GetPolyGridMgr() { return m_pPolyGridMgr; }
	CLineSystemManager*		GetLineSystemMgr() { return m_pLineSystemMgr; }

	COptimizedSurfaceBatchManager*	GetOptimizedSurfaceBatchMgr() { return m_pOptimizedSurfaceBatchMgr; }
	CCanvasBatchManager*			GetCanvasBatchMgr() { return m_pCanvasBatchMgr; }
	CSpriteBatchManager*			GetSpriteBatchMgr() { return m_pSpriteBatchMgr; }

	CLightMapManager*	GetLightMapMgr() { return m_pLightMapMgr; }

#ifdef WORLD_TWEAKS_ENABLED
	CWorldTweakManager* GetWorldTweakMgr() { return m_pWorldTweakMgr; }
#endif

	ID3D11Buffer*	GetVertexBuffer_Line() { return m_pVertexBuffer_Line; }

	ID3D11Buffer*	GetIndexBuffer_Opt2D() { return m_pIndexBuffer_Opt2D; }
	ID3D11Buffer*	GetIndexBuffer_Sprite() { return m_pIndexBuffer_Sprite; }
	ID3D11Buffer*	GetIndexBuffer_SpriteEx() { return m_pIndexBuffer_SpriteEx; }

	bool	UpdateGenericBuffer(ID3D11Buffer* pBuffer, void* pData, uint32 dwSize, D3D11_MAP eMapType);

	template<typename F>
	bool	UpdateGenericBufferEx(ID3D11Buffer* pBuffer, D3D11_MAP eMapType, F& func);

	bool	MapGenericBuffer(ID3D11Buffer* pBuffer, D3D11_MAP eMapType, D3D11_MAPPED_SUBRESOURCE* pSubResource);
	void	UnmapGenericBuffer(ID3D11Buffer* pBuffer);

	ID3D11Buffer*	CreateVertexBuffer(void* pData, uint32 dwSize, D3D11_USAGE eUsage, UINT dwCPUAccessFlags);
	ID3D11Buffer*	CreateVertexBuffer(uint32 dwSize, D3D11_USAGE eUsage, UINT dwCPUAccessFlags);

	template<typename F>
	uint32			AssignBankedVertexBuffer(void* pData, uint32 dwSize, F& updateFunc);
	uint32			AssignBankedVertexBuffer(void* pData, uint32 dwSize);

	ID3D11Buffer*	CreateIndexBuffer(void* pData, uint32 dwSize, D3D11_USAGE eUsage, UINT dwCPUAccessFlags);
	ID3D11Buffer*	CreateIndexBuffer(uint32 dwSize, D3D11_USAGE eUsage, UINT dwCPUAccessFlags);

	template<typename F>
	uint32			AssignBankedIndexBuffer(void* pData, uint32 dwSize, F& updateFunc);
	uint32			AssignBankedIndexBuffer(void* pData, uint32 dwSize);

	ID3D11Buffer*	CreateConstantBuffer(void* pData, uint32 dwSize, D3D11_USAGE eUsage, UINT dwCPUAccessFlags);
	ID3D11Buffer*	CreateConstantBuffer(uint32 dwSize, D3D11_USAGE eUsage, UINT dwCPUAccessFlags);

	BankedBuffer&	GetBankedVertexBuffer(uint32 dwIndex) { return m_aVertexBufferBank[dwIndex]; }
	BankedBuffer&	GetBankedIndexBuffer(uint32 dwIndex) { return m_aIndexBufferBank[dwIndex]; }

	EngineHacksStruct*	GetEngineHacks() { return &m_sEngineHacks; }

	SolidDrawingMode	GetSolidDrawingMode() { return m_eSolidDrawingMode; }
	QueueObjectsMode	GetQueueObjectsMode() { return m_eQueueObjectsMode; }

private:

	uint32	GetBufferBankItem(Array_BankedBuffer& bank, uint32 dwSize);

	CTextureManager*	m_pTextureMgr;

	CModelManager*			m_pModelMgr;
	CWorldModelManager*		m_pWorldModelMgr;
	CSpriteManager*			m_pSpriteMgr;
	CParticleSystemManager* m_pParticleSystemMgr;
	CCanvasManager*			m_pCanvasMgr;
	CPolyGridManager*		m_pPolyGridMgr;
	CLineSystemManager*		m_pLineSystemMgr;

	CLightMapManager*	m_pLightMapMgr;

	COptimizedSurfaceBatchManager*	m_pOptimizedSurfaceBatchMgr;
	CCanvasBatchManager*			m_pCanvasBatchMgr;
	CSpriteBatchManager*			m_pSpriteBatchMgr;

#ifdef WORLD_TWEAKS_ENABLED
	CWorldTweakManager* m_pWorldTweakMgr;
#endif

	ID3D11Buffer*	m_pVertexBuffer_Line;

	ID3D11Buffer*	m_pIndexBuffer_Opt2D;
	ID3D11Buffer*	m_pIndexBuffer_Sprite;
	ID3D11Buffer*	m_pIndexBuffer_SpriteEx;

	Array_BankedBuffer	m_aVertexBufferBank;
	Array_BankedBuffer	m_aIndexBufferBank;

	EngineHacksStruct	m_sEngineHacks;

	SolidDrawingMode	m_eSolidDrawingMode;
	QueueObjectsMode	m_eQueueObjectsMode;
};

extern CGlobalManager g_GlobalMgr;

extern DirectX::XMFLOAT4X4 g_mIdentity;

template<typename F>
inline bool CGlobalManager::UpdateGenericBufferEx(ID3D11Buffer* pBuffer, D3D11_MAP eMapType, F& updateFunc)
{
	D3D11_MAPPED_SUBRESOURCE subResource;
	HRESULT hResult = g_D3DDevice.GetDeviceContext()->Map(pBuffer, 0, eMapType, 0, &subResource);
	if (FAILED(hResult))
		return false;

	updateFunc(&subResource);

	g_D3DDevice.GetDeviceContext()->Unmap(pBuffer, 0);

	return true;
}

template<typename F>
uint32 CGlobalManager::AssignBankedVertexBuffer(void* pData, uint32 dwSize, F& updateFunc)
{
	uint32 dwIndex = GetBufferBankItem(m_aVertexBufferBank, dwSize);

	if (dwIndex != UINT32_MAX)
	{
		BankedBuffer& item = m_aVertexBufferBank[dwIndex];

		if (!UpdateGenericBufferEx(item.m_pBuffer, D3D11_MAP_WRITE_DISCARD, updateFunc))
			return UINT32_MAX;

		item.m_bInUse = true;

		return dwIndex;
	}
	else
	{
		ID3D11Buffer* pBuffer = CreateVertexBuffer(pData, dwSize, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
		if (pBuffer == nullptr)
			return UINT32_MAX;

		m_aVertexBufferBank.emplace_back(pBuffer, dwSize, true);

		return m_aVertexBufferBank.size() - 1;
	}
}

template<typename F>
uint32 CGlobalManager::AssignBankedIndexBuffer(void* pData, uint32 dwSize, F& updateFunc)
{
	uint32 dwIndex = GetBufferBankItem(m_aIndexBufferBank, dwSize);

	if (dwIndex != UINT32_MAX)
	{
		BankedBuffer& item = m_aIndexBufferBank[dwIndex];

		if (!UpdateGenericBufferEx(item.m_pBuffer, D3D11_MAP_WRITE_DISCARD, updateFunc))
			return UINT32_MAX;

		item.m_bInUse = true;

		return dwIndex;
	}
	else
	{
		ID3D11Buffer* pBuffer = CreateIndexBuffer(pData, dwSize, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
		if (pBuffer == nullptr)
			return UINT32_MAX;

		m_aIndexBufferBank.emplace_back(pBuffer, dwSize, true);

		return m_aIndexBufferBank.size() - 1;
	}
}

#endif
