#ifndef __DRAW_CANVAS_H__
#define __DRAW_CANVAS_H__

#ifndef __TAGNODES_H__
#include "tagnodes.h"
#endif

#ifndef __ILTCUSTOMDRAW_H__
#include "../lithtech/iltcustomdraw.h"
#endif

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
#include "renderstatemgr.h"
#include "d3d_shader_canvas.h"

//#define CANVAS_DATA_MEM_CMP
#define CANVAS_TEMP_BUFFER_LEN		16
#define CANVAS_TEXTURES_PER_DRAW	12
#define CANVAS_COMMON_VERTICES		6

typedef VertexTypes::PositionColorTexture InternalCanvasVertex;
typedef VertexTypes::PositionColorTextureIndex InternalCanvasBatchVertex;

class InternalCustomDraw;

class CCanvasData : public CExpirableData
{

public:

	CCanvasData(uint32 dwVerts) : CExpirableData()
	{
		m_pData = new LTVertex[dwVerts];
		m_dwVerts = dwVerts;
		m_dwBankedVBIndex = UINT32_MAX;
	}

	~CCanvasData() { Term(); }

	void Term();

	CompareResult CompareTo(LTVertex* pOthers, uint32 dwVerts);

	bool	CreateVertexBuffer();
	bool	UpdateVertexBuffer(LTVertex* pNewVerts, uint32 dwVerts);

	LTVertex*	GetData() { return m_pData; }
	
	uint32	m_dwBankedVBIndex;

private:

	LTVertex*	m_pData;
	uint32		m_dwVerts;
};

class CCanvasBatchManager : public CBaseDataManager
{

public:

	struct Command
	{
		Command(BlendState eBlendState, StencilState eStencilState, bool bReallyClose, 
			uint32 dwVertexOffset, uint32 dwVertexCount)
		{
			m_eBlendState = eBlendState;
			m_eStencilState = eStencilState;
			m_bReallyClose = bReallyClose;
			m_dwVertexOffset = dwVertexOffset;
			m_dwVertexCount = dwVertexCount;
		};

		BlendState		m_eBlendState;
		StencilState	m_eStencilState;
		bool			m_bReallyClose;
		uint32			m_dwVertexOffset;
		uint32			m_dwVertexCount;
	};

	CCanvasBatchManager() : CBaseDataManager(), m_aParams(), m_apSRV()
	{
		m_pViewParams = nullptr;
		m_dwCanvasCount = 0;
		m_dwTextureCount = 0;
		m_pMappedVertexData = nullptr;
		m_pVertexBuffer = nullptr;
		m_dwVertexCount = 0;
		m_bHasReallyCloseItems = false;
	}

	typedef std::vector<Command> Array_Command;

	virtual void	Init();
	virtual void	FreeAllData();

	void	BatchPrimitive(LTVertex* pVerts, uint32 dwVerts, uint32 dwFlags, InternalCustomDraw* pDraw);
	void	RenderBatch() { if (m_dwCanvasCount) InternalRenderBatch(); }

	void	SetViewParams(ViewParams* pSet) { m_pViewParams = pSet; }

private:

	bool	IsFull()
	{
		return (m_dwTextureCount == CANVAS_TEXTURES_PER_DRAW || m_dwCanvasCount == CANVAS_BATCH_SIZE);
	}

	uint32	BatchTexture(ID3D11ShaderResourceView* pSRV);

	void	UpdateVertexBuffer(LTVertex* pVerts, uint32 dwVerts, uint32 dwStart, uint32 dwDataIndex);

	bool	MapVertexBuffer();
	void	SetBuffersAndTopology();

	void	AppendCommand(BlendState eBlendState, StencilState eStencilState, bool bReallyClose, 
		uint32 dwVertexOffset, uint32 dwVertexCount);

	void	InternalRenderBatch();

	ViewParams* m_pViewParams;

	uint32	m_dwVertexCount;
	bool	m_bHasReallyCloseItems;
	 
	CRenderShader_CanvasBatch::VPSPerObjectParams	m_aParams[CANVAS_BATCH_SIZE];
	uint32											m_dwCanvasCount;

	ID3D11ShaderResourceView*	m_apSRV[CANVAS_TEXTURES_PER_DRAW];
	uint32						m_dwTextureCount;

	InternalCanvasBatchVertex*	m_pMappedVertexData;

	ID3D11Buffer* m_pVertexBuffer;

	Array_Command	m_aCommand;
};

class CCanvasManager : public CExpirableDataManager<SharedTexture*, CCanvasData*>
{

public:

	CCanvasManager() : CExpirableDataManager()
	{
		m_pTempBuffer = nullptr;
	}

	virtual void Init()
	{
		CExpirableDataManager::Init();
		m_pTempBuffer = new InternalCanvasVertex[CANVAS_TEMP_BUFFER_LEN];
	}

	virtual void Term()
	{
		CExpirableDataManager::Term();
		delete[] m_pTempBuffer;
	}

	CCanvasData*	GetCanvasData(SharedTexture* pTexture, LTVertex* pVerts, uint32 dwVerts);

	InternalCanvasVertex*	GetTempBuffer() { return m_pTempBuffer; }

private:

	InternalCanvasVertex*	m_pTempBuffer;
};

class InternalCustomDraw : ILTCustomDraw
{

public:

	InternalCustomDraw();
	~InternalCustomDraw() { };

	virtual LTRESULT	DrawPrimitive(LTVertex* pVerts, uint32 dwVerts, uint32 dwFlags);

	virtual LTRESULT	SetState(LTRState eState, uint32 dwVal);
	virtual LTRESULT	GetState(LTRState eState, uint32& dwVal);

	virtual LTRESULT	SetTexture(const char* szTexture);

	virtual LTRESULT	GetTexelSize(float& fSizeU, float& fSizeV);

	void	DrawCanvases(Canvas** ppList, uint32& dwListSize);
	void	DrawCanvas(Canvas* pCanvas);

	Canvas*			GetCanvas() { return m_pCanvas; }
	SharedTexture*	GetTexture() { return m_pTexture; }

	void	TranslateStates(uint32& dwRenderMode, BlendState& eBlendState, StencilState& eStencilState);

	void	SetExtendedDraw(bool bSet) { m_bExtendedDraw = bSet; }

private:

	void	DrawPrimitiveActual(LTVertex* pVerts, uint32 dwVerts, DirectX::XMFLOAT4X4* pReallyCloseProj, 
		uint32 dwRenderMode);

	uint32	m_adwLTRStates[NUM_LTRSTATES];

	Canvas*			m_pCanvas;
	SharedTexture*	m_pTexture;

	bool	m_bExtendedDraw;
};

void d3d_ProcessCanvas(LTObject* pObject);
void d3d_DrawSolidCanvases(ViewParams* pParams);
void d3d_QueueTranslucentCanvases(ViewParams* pParams, ObjectDrawList* pDrawList);

#endif