#ifndef __D3D_DRAW_SPRITE_H__
#define __D3D_DRAW_SPRITE_H__

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
#include "d3d_convar.h"
#include "d3d_shader_sprite.h"
#include "renderstatemgr.h"

//#define SPRITE_DATA_MEM_CMP

#define SPRITE_DEFAULT_VERTICES		4
#define SPRITE_DEFAULT_INDICES		6
#define SPRITE_MAX_VERTICES			8
#define SPRITE_MAX_INDICES			18
#define SPRITE_CLIPPED_VERTICES		40 + 5
#define MAX_SPRITE_LIGHTS			8
#define SPRITE_TEXTURES_PER_DRAW	12

typedef VertexTypes::PositionTexture InternalSpriteVertex;
typedef VertexTypes::PositionTextureIndex InternalSpriteBatchVertex;

class ViewParams;
class ObjectDrawList;

class CSpriteData : public CExpirableData
{

public:

	CSpriteData() : CExpirableData(), m_aData()
	{
		m_dwBankedVBIndex = UINT32_MAX;
	}

	~CSpriteData();

	bool CompareTo(InternalSpriteVertex* pOthers)
	{
#ifndef SPRITE_DATA_MEM_CMP
		return m_aData[0].CompareTo(pOthers) && m_aData[1].CompareTo(pOthers + 1) &&
			m_aData[2].CompareTo(pOthers + 2) && m_aData[3].CompareTo(pOthers + 3);
#else
		return !memcmp(m_aData, pOthers, SPRITE_VERTICES * sizeof(InternalSpriteVertex));
#endif
	}

	bool	CreateVertexBuffer();
	bool	UpdateVertexBuffer(InternalSpriteVertex* pNewVerts);

	InternalSpriteVertex*	GetData() { return m_aData; }

	uint32	m_dwBankedVBIndex;

private:

	InternalSpriteVertex	m_aData[SPRITE_MAX_VERTICES];
};

class CSpriteBatchManager : public CBaseDataManager
{

public:

	struct Command
	{
		Command(BlendState eBlendState, StencilState eStencilState, bool bReallyClose,
			uint32 dwVertexOffset, uint32 dwVertexCount, uint32 dwIndexOffset, uint32 dwIndexCount)
		{
			m_eBlendState = eBlendState;
			m_eStencilState = eStencilState;
			m_bReallyClose = bReallyClose;
			m_dwVertexOffset = dwVertexOffset;
			m_dwVertexCount = dwVertexCount;
			m_dwIndexOffset = dwIndexOffset;
			m_dwIndexCount = dwIndexCount;
		};

		BlendState		m_eBlendState;
		StencilState	m_eStencilState;
		bool			m_bReallyClose;
		uint32			m_dwVertexOffset;
		uint32			m_dwVertexCount;
		uint32			m_dwIndexOffset;
		uint32			m_dwIndexCount;
	};

	CSpriteBatchManager() : CBaseDataManager(), m_aParams(), m_apSRV()
	{
		m_pViewParams = nullptr;
		m_dwSpriteCount = 0;
		m_dwTextureCount = 0;
		m_pMappedVertexData = nullptr;
		m_pMappedIndexData = nullptr;
		m_pVertexBuffer = nullptr;
		m_pIndexBuffer = nullptr;
		m_dwVertexCount = 0;
		m_dwIndexCount = 0;
		m_bHasReallyCloseItems = false;
	}

	typedef std::vector<Command> Array_Command;

	virtual void	Init();
	virtual void	FreeAllData();

	void	BatchSprite(SpriteInstance* pInstance, SpriteEntry* pEntry);
	void	RenderBatch() { if (m_dwSpriteCount) InternalRenderBatch(); }

	void	SetViewParams(ViewParams* pSet) { m_pViewParams = pSet; }

private:

	bool	IsFull()
	{
		return (m_dwTextureCount == SPRITE_TEXTURES_PER_DRAW || m_dwSpriteCount == SPRITE_BATCH_SIZE);
	}

	uint32	BatchTexture(ID3D11ShaderResourceView* pSRV);

	uint32	BatchSprite_Normal(ViewParams* pParams, SpriteInstance* pInstance, InternalSpriteBatchVertex* pSpriteVerts,
		SharedTexture* pSharedTexture, bool bReallyClose, uint32 dwDataIndex);
	uint32	BatchSprite_Rotatable(ViewParams* pParams, SpriteInstance* pInstance, InternalSpriteBatchVertex** ppSpriteVerts, 
		InternalSpriteBatchVertex* pClippedSpriteVerts, SharedTexture* pSharedTexture, bool bReallyClose, uint32 dwDataIndex);

	void	UpdateBuffers(InternalSpriteBatchVertex* pSpriteVerts, uint32 dwVertexStart, uint32 dwVertexCount, 
		uint32 dwIndexStart, uint32 dwIndexCount);

	bool	MapBuffers();
	void	SetBuffersAndTopology();

	void	AppendCommand(BlendState eBlendState, StencilState eStencilState, bool bReallyClose,
		uint32 dwVertexOffset, uint32 dwVertexCount, uint32 dwIndexOffset, uint32 dwIndexCount);

	void	InternalRenderBatch();

	ViewParams*	m_pViewParams;

	uint32	m_dwVertexCount;
	uint32	m_dwIndexCount;
	bool	m_bHasReallyCloseItems;

	CRenderShader_SpriteBatch::VPSPerObjectParams	m_aParams[SPRITE_BATCH_SIZE];
	uint32											m_dwSpriteCount;

	ID3D11ShaderResourceView*	m_apSRV[SPRITE_TEXTURES_PER_DRAW];
	uint32						m_dwTextureCount;

	InternalSpriteBatchVertex*	m_pMappedVertexData;
	uint16*						m_pMappedIndexData;

	ID3D11Buffer*	m_pVertexBuffer;
	ID3D11Buffer*	m_pIndexBuffer;

	Array_Command	m_aCommand;
};

class CSpriteManager : public CExpirableDataManager<Sprite*, CSpriteData*>
{

public:

	CSpriteData*	GetSpriteData(SpriteInstance* pInstance, InternalSpriteVertex* pVerts);
};

void d3d_ProcessSprite(LTObject* pObject);
void d3d_QueueTranslucentSprites(ViewParams* pParams, ObjectDrawList* pDrawList);
void d3d_DrawSprite(ViewParams* pParams, LTObject* pObj);
void d3d_DrawNoZSprites(ViewParams* pParams);

#endif