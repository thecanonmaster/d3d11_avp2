#ifndef __D3D_OPTIMIZED_SURFACE_H__
#define __D3D_OPTIMIZED_SURFACE_H__

#ifndef __D3D11_H__
#include <d3d11_1.h>
#define __D3D11_H__
#endif

#ifndef __DIRECT_X_MATH_H__
#include <DirectXMath.h>
#define __DIRECT_X_MATH_H__
#endif

#include "d3d_surface.h"
#include "d3d_vertextypes.h"
#include "common_datamgr.h"
#include "d3d_shader_optimizedsurface.h"
#include "renderstatemgr.h"

#define OPTIMIZED_SURFACE_VERTICES		4
#define OPTIMIZED_SURFACE_INDICES		6
#define OPTIMIZED_SURFACE_WARP_PTS		4

#define OPTIMIZED_SURFACE_TEXTURES_PER_DRAW		12

typedef VertexTypes::PositionTexture InternalSurfaceVertex;
typedef VertexTypes::PositionTextureIndex InternalSurfaceBatchVertex;

class SurfaceTile
{

public:

	SurfaceTile() : m_aLastWarpPts()
	{
		m_dwBankedVBIndex = UINT32_MAX;
		m_pResourceView = nullptr;
		m_pTexture = nullptr;
		m_rcLastSrcRect = { };
		m_rcLastDestRect = { };
	}

	uint32	m_dwBankedVBIndex;

	ID3D11ShaderResourceView*	m_pResourceView;
	ID3D11Texture2D*			m_pTexture;

	LTRect	m_rcLastSrcRect;
	LTRect	m_rcLastDestRect;

	LTWarpPt m_aLastWarpPts[OPTIMIZED_SURFACE_WARP_PTS];
};

class COptimizedSurfaceBatchManager : public CBaseDataManager
{

public:

	struct Command
	{
		Command(BlendState eBlendState, uint32 dwIndexOffset)
		{
			m_eBlendState = eBlendState;
			m_dwIndexOffset = dwIndexOffset;
			m_dwIndexCount = OPTIMIZED_SURFACE_INDICES;
		};

		BlendState	m_eBlendState;
		uint32		m_dwIndexOffset;
		uint32		m_dwIndexCount;
	};

	typedef std::vector<Command> Array_Command;

	COptimizedSurfaceBatchManager() : CBaseDataManager(), m_aParams(), m_apSRV()
	{
		m_dwSurfaceCount = 0;
		m_dwTextureCount = 0;
		m_pMappedVertexData = nullptr;
		m_pVertexBuffer = nullptr;
		m_pIndexBuffer = nullptr;
	}

	virtual void	Init();
	virtual void	FreeAllData();

	void	BatchBlit(BlitRequest* pRequest, LTSurfaceBlend eBlend, RSurface* pSurface, bool bWarp);
	void	RenderBatch();

private:

	bool	IsFull() 
	{ 
		return (m_dwTextureCount == OPTIMIZED_SURFACE_TEXTURES_PER_DRAW ||
			m_dwSurfaceCount == OPTIMIZED_SURFACE_BATCH_SIZE);
	}

	uint32	BatchTexture(ID3D11ShaderResourceView* pSRV);

	void	UpdateVertexBuffer_Blit(LTRect* pCurSrcRect, LTRect* pCurDestRect, RSurface* pSurface, 
		uint32 dwIndex, uint32 dwDataIndex);
	void	UpdateVertexBuffer_Warp(LTRect* pCurSrcRect, LTRect* pCurDestRect, LTWarpPt* pWarpPts, 
		RSurface* pSurface, uint32 dwIndex, uint32 dwDataIndex);

	bool	MapVertexBuffer();
	void	SetBuffersAndTopology();

	void	AppendCommand(BlendState eBlendState, uint32 dwIndexOffset);

	CRenderShader_OptimizedSurfaceBatch::PSPerObjectParams	m_aParams[OPTIMIZED_SURFACE_BATCH_SIZE];
	uint32													m_dwSurfaceCount;

	ID3D11ShaderResourceView*	m_apSRV[OPTIMIZED_SURFACE_TEXTURES_PER_DRAW];
	uint32						m_dwTextureCount;

	InternalSurfaceBatchVertex*	m_pMappedVertexData;

	ID3D11Buffer*	m_pVertexBuffer;
	ID3D11Buffer*	m_pIndexBuffer;

	Array_Command	m_aCommand;
};

void d3d_DestroyTiles(RSurface* pSurface);
void d3d_UnoptimizeSurface(HLTBUFFER hBuffer);
bool d3d_OptimizeSurface(HLTBUFFER hSurface, GenericColor dwTransparentColor);

void d3d_Unoptimized2D_SetStates();

bool d3d_StartUnoptimized2D();
bool d3d_StartOptimized2D();
void d3d_EndOptimized2D();
LTBOOL d3d_IsInOptimized2D();

LTBOOL d3d_SetOptimized2DBlend(LTSurfaceBlend eBlend);
LTBOOL d3d_GetOptimized2DBlend(LTSurfaceBlend& eBlend);
LTBOOL d3d_SetOptimized2DColor(HLTCOLOR dwColor);
LTBOOL d3d_GetOptimized2DColor(HLTCOLOR& dwColor);

void d3d_BlitToScreen3D(BlitRequest* pRequest);
void d3d_WarpToScreen3D(BlitRequest* pRequest);
void d3d_BlitToScreen3D(InternalBlitRequest* pRequest);

#endif