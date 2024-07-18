#include "pch.h"

#include "d3d_optimizedsurface.h"
#include "d3d_device.h"
#include "d3d_init.h"
#include "renderstatemgr.h"
#include "rendererconsolevars.h"
#include "d3d_shader_optimizedsurface.h"
#include "d3d_shader_soliddrawing.h"
#include "d3d_shader_fullscreenvideo.h"
#include "rendershadermgr.h"
#include "common_stuff.h"
#include "d3d_texture.h"
#include "d3d_postprocess.h"
#include "d3d_mathhelpers.h"
#include "globalmgr.h"

using namespace DirectX;

static const char* g_szFreeError_TXR = "Failed to free D3D texture (count = %d)";
static const char* g_szFreeError_RVW = "Failed to free D3D resource view (count = %d)";

static uint32 g_dwOptimized2DBlend = LTSURFACEBLEND_ALPHA;
static HLTCOLOR g_dwOptimized2DColor = 0xFFFFFFFF;
static SamplerState g_dwOptimized2DOldSamplerState = SAMPLER_STATE_Point;

static const BlendState m_aeLTSurfaceBlendToState[LTSURFACEBLEND_MASKADD + 1] =
{
	BLEND_STATE_Alpha,
	BLEND_STATE_Solid,
	BLEND_STATE_Add,
	BLEND_STATE_Multiply,
	BLEND_STATE_Multiply2,
	BLEND_STATE_Mask,
	BLEND_STATE_MaskAdd
};

static inline BlendState d3d_GetOptimized2DBlendState(LTSurfaceBlend eBlend)
{
	return m_aeLTSurfaceBlendToState[eBlend];
}

static inline void d3d_SetOptimized2DBlendState(LTSurfaceBlend eBlend)
{
	g_RenderStateMgr.SetBlendState(d3d_GetOptimized2DBlendState(eBlend));
}

void COptimizedSurfaceBatchManager::FreeAllData()
{
	RELEASE_INTERFACE(m_pVertexBuffer, g_szFreeError_VB);
	RELEASE_INTERFACE(m_pIndexBuffer, g_szFreeError_IB);
}

void COptimizedSurfaceBatchManager::Init()
{
	CBaseDataManager::Init();
	
	m_dwSurfaceCount = 0;
	m_dwTextureCount = 0;
	m_pMappedVertexData = nullptr;

	memset(m_apSRV, 0, sizeof(m_apSRV));
	m_aCommand.reserve(OPTIMIZED_SURFACE_BATCH_SIZE);

	m_pVertexBuffer = g_GlobalMgr.CreateVertexBuffer(
		sizeof(InternalSurfaceBatchVertex) * OPTIMIZED_SURFACE_VERTICES * OPTIMIZED_SURFACE_BATCH_SIZE,
		D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

	uint16 awIndex[OPTIMIZED_SURFACE_BATCH_SIZE * OPTIMIZED_SURFACE_INDICES] = { 0, 1, 2, 0, 3, 1 };

	for (uint32 i = 1; i < OPTIMIZED_SURFACE_BATCH_SIZE; i++)
	{
		uint32 dwStart = i * OPTIMIZED_SURFACE_INDICES;
		uint16 wShift = i * OPTIMIZED_SURFACE_VERTICES;

		awIndex[dwStart + 0] = awIndex[0] + wShift;
		awIndex[dwStart + 1] = awIndex[1] + wShift;
		awIndex[dwStart + 2] = awIndex[2] + wShift;
		awIndex[dwStart + 3] = awIndex[3] + wShift;
		awIndex[dwStart + 4] = awIndex[4] + wShift;
		awIndex[dwStart + 5] = awIndex[5] + wShift;
	}

	m_pIndexBuffer = g_GlobalMgr.CreateIndexBuffer(awIndex, sizeof(uint16) * OPTIMIZED_SURFACE_BATCH_SIZE * OPTIMIZED_SURFACE_INDICES, 
		D3D11_USAGE_IMMUTABLE, 0);
}

void COptimizedSurfaceBatchManager::BatchBlit(BlitRequest* pRequest, LTSurfaceBlend eBlend, RSurface* pSurface, bool bWarp)
{
	if (IsFull())
		RenderBatch();
	
	if (!MapVertexBuffer())
		return;

	CRenderShader_OptimizedSurfaceBatch::PSPerObjectParams& params = m_aParams[m_dwSurfaceCount];

	if (!bWarp)
	{
		UpdateVertexBuffer_Blit(pRequest->m_pSrcRect, pRequest->m_pDestRect, pSurface, m_dwSurfaceCount,
			m_dwSurfaceCount);
	}
	else
	{
		UpdateVertexBuffer_Warp(pRequest->m_pSrcRect, pRequest->m_pDestRect, pRequest->m_pWarpPts, pSurface, 
			m_dwSurfaceCount, m_dwSurfaceCount);
	}

	params.m_fAlpha = pRequest->m_fAlpha;
	params.m_dwTextureIndex = BatchTexture(((RSurface*)pRequest->m_hBuffer)->m_pTile->m_pResourceView);

	uint32 dwTransparentColor = pRequest->m_dwTransparentColor.dwVal;
	params.m_vTransparentColor =
	{
		RGBA_GETFR(dwTransparentColor) * MATH_ONE_OVER_255,
		RGBA_GETFG(dwTransparentColor) * MATH_ONE_OVER_255,
		RGBA_GETFB(dwTransparentColor) * MATH_ONE_OVER_255,
	};

	params.m_vBaseColor =
	{
		RGBA_GETFR(g_dwOptimized2DColor) * MATH_ONE_OVER_255,
		RGBA_GETFG(g_dwOptimized2DColor) * MATH_ONE_OVER_255,
		RGBA_GETFB(g_dwOptimized2DColor) * MATH_ONE_OVER_255
	};

	BlendState eBlendState = (pSurface->m_bTileTransparent || pRequest->m_fAlpha != 1.0f) ? 
		d3d_GetOptimized2DBlendState((LTSurfaceBlend)g_dwOptimized2DBlend) : BLEND_STATE_Default;

	AppendCommand(eBlendState, m_dwSurfaceCount * OPTIMIZED_SURFACE_INDICES);

	m_dwSurfaceCount++;
}

void COptimizedSurfaceBatchManager::RenderBatch()
{
	if (!m_dwSurfaceCount)
		return;
	
	if (m_pMappedVertexData != nullptr)
	{
		g_GlobalMgr.UnmapGenericBuffer(m_pVertexBuffer);
		m_pMappedVertexData = nullptr;
	}

	SetBuffersAndTopology();
	
	CRenderShader_OptimizedSurfaceBatch* pRenderShader = 
		g_RenderShaderMgr.GetRenderShader<CRenderShader_OptimizedSurfaceBatch>();

	if (!pRenderShader->SetPerObjectParams(m_apSRV, m_dwTextureCount, m_aParams, m_dwSurfaceCount))
		return;

	for (COptimizedSurfaceBatchManager::Command& command : m_aCommand)
	{
		g_RenderStateMgr.SetBlendState(command.m_eBlendState);
		pRenderShader->Render(command.m_dwIndexCount, command.m_dwIndexOffset);
	}

	for (uint32 i = 0; i < m_dwTextureCount; i++)
		m_apSRV[i]->Release();

	m_dwSurfaceCount = 0;
	m_dwTextureCount = 0;
	m_aCommand.clear();
}

uint32 COptimizedSurfaceBatchManager::BatchTexture(ID3D11ShaderResourceView* pSRV)
{
	for (uint32 i = 0; i < m_dwTextureCount; i++)
	{
		if (m_apSRV[i] == pSRV)
			return i;
	}

	m_apSRV[m_dwTextureCount] = pSRV;
	m_apSRV[m_dwTextureCount]->AddRef();
	m_dwTextureCount++;
	
	return m_dwTextureCount - 1;
}

void COptimizedSurfaceBatchManager::UpdateVertexBuffer_Blit(LTRect* pCurSrcRect, LTRect* pCurDestRect, RSurface* pSurface,
	uint32 dwIndex, uint32 dwDataIndex)
{
	InternalSurfaceBatchVertex* pVertices = &m_pMappedVertexData[dwIndex * OPTIMIZED_SURFACE_VERTICES];

	float fLeft = (float)pCurDestRect->m_nLeft;
	float fRight = (float)pCurDestRect->m_nRight;
	float fTop = (float)pCurDestRect->m_nTop;
	float fBottom = (float)pCurDestRect->m_nBottom;

	pVertices[0].vPosition = { fLeft, fTop, 0.0f };
	pVertices[1].vPosition = { fRight, fBottom, 0.0f };
	pVertices[2].vPosition = { fLeft, fBottom, 0.0f };
	pVertices[3].vPosition = { fRight, fTop, 0.0f };

	float fTexLeft = (float)pCurSrcRect->m_nLeft / (float)pSurface->m_dwWidth;
	float fTexRight = (float)pCurSrcRect->m_nRight / (float)pSurface->m_dwWidth;
	float fTexTop = (float)pCurSrcRect->m_nTop / (float)pSurface->m_dwHeight;
	float fTexBottom = (float)pCurSrcRect->m_nBottom / (float)pSurface->m_dwHeight;

	pVertices[0].vTexCoords = { fTexLeft, fTexTop };
	pVertices[1].vTexCoords = { fTexRight, fTexBottom };
	pVertices[2].vTexCoords = { fTexLeft, fTexBottom };
	pVertices[3].vTexCoords = { fTexRight, fTexTop };

	pVertices[0].dwDataIndex = dwDataIndex;
	pVertices[1].dwDataIndex = dwDataIndex;
	pVertices[2].dwDataIndex = dwDataIndex;
	pVertices[3].dwDataIndex = dwDataIndex;
}

void COptimizedSurfaceBatchManager::UpdateVertexBuffer_Warp(LTRect* pCurSrcRect, LTRect* pCurDestRect, LTWarpPt* pWarpPts, 
	RSurface* pSurface, uint32 dwIndex, uint32 dwDataIndex)
{
	DirectX::XMVECTOR vWarpStart = DirectX::XMVectorSet(pWarpPts[0].m_fDestX, pWarpPts[0].m_fDestY, 0.0f, 1.0f);
	DirectX::XMVECTOR vWarpDirX = DirectX::XMVectorSet(pWarpPts[1].m_fDestX, pWarpPts[1].m_fDestY, 0.0f, 1.0f) - vWarpStart;
	DirectX::XMVECTOR vWarpDirY = DirectX::XMVectorSet(pWarpPts[3].m_fDestX, pWarpPts[3].m_fDestY, 0.0f, 1.0f) - vWarpStart;

	float fSrcRectWidth = (float)(pCurSrcRect->m_nRight - pCurSrcRect->m_nLeft);
	float fSrcRectHeight = (float)(pCurSrcRect->m_nBottom - pCurSrcRect->m_nTop);

	DirectX::XMFLOAT3 vWarpDirXTemp, vWarpDirYTemp;
	DirectX::XMStoreFloat3(&vWarpDirXTemp, vWarpDirX);
	DirectX::XMStoreFloat3(&vWarpDirYTemp, vWarpDirY);

	float fScaleX = Vector_Mag(&vWarpDirXTemp) / fSrcRectWidth;
	float fScaleY = Vector_Mag(&vWarpDirYTemp) / fSrcRectHeight;

	Vector_Norm(&vWarpDirXTemp, fScaleX);
	Vector_Norm(&vWarpDirYTemp, fScaleY);

	vWarpDirX = DirectX::XMLoadFloat3(&vWarpDirXTemp);
	vWarpDirY = DirectX::XMLoadFloat3(&vWarpDirYTemp);

	InternalSurfaceBatchVertex* pVertices = &m_pMappedVertexData[dwIndex * OPTIMIZED_SURFACE_VERTICES];

	DirectX::XMVECTOR vPosTemp = vWarpStart + vWarpDirX + vWarpDirY;
	DirectX::XMStoreFloat3(&pVertices[0].vPosition, vPosTemp);
	DirectX::XMStoreFloat3(&pVertices[1].vPosition, vPosTemp + (vWarpDirX * fSrcRectWidth) + (vWarpDirY * fSrcRectHeight));
	DirectX::XMStoreFloat3(&pVertices[2].vPosition, vPosTemp + (vWarpDirY * fSrcRectHeight));
	DirectX::XMStoreFloat3(&pVertices[3].vPosition, vPosTemp + (vWarpDirX * fSrcRectWidth));

	pVertices[0].vTexCoords = { 0.0f, 0.0f };
	pVertices[1].vTexCoords = { 1.0f, 1.0f };
	pVertices[2].vTexCoords = { 0.0f, 1.0f };
	pVertices[3].vTexCoords = { 1.0f, 0.0f };

	pVertices[0].dwDataIndex = dwDataIndex;
	pVertices[1].dwDataIndex = dwDataIndex;
	pVertices[2].dwDataIndex = dwDataIndex;
	pVertices[3].dwDataIndex = dwDataIndex;
}

bool COptimizedSurfaceBatchManager::MapVertexBuffer()
{
	if (m_pMappedVertexData == nullptr)
	{
		D3D11_MAPPED_SUBRESOURCE subResource;

		if (!g_GlobalMgr.MapGenericBuffer(m_pVertexBuffer, D3D11_MAP_WRITE_DISCARD, &subResource))
			return false;

		m_pMappedVertexData = (InternalSurfaceBatchVertex*)subResource.pData;
	}

	return true;
}

void COptimizedSurfaceBatchManager::SetBuffersAndTopology()
{
	g_RenderShaderMgr.SetVertexResource(VRS_Main, m_pVertexBuffer, sizeof(InternalSurfaceBatchVertex), 0);
	g_RenderShaderMgr.SetIndexBuffer16(m_pIndexBuffer, 0);
	g_RenderShaderMgr.SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void COptimizedSurfaceBatchManager::AppendCommand(BlendState eBlendState, uint32 dwIndexOffset)
{
	if (m_aCommand.empty())
	{
		m_aCommand.emplace_back(eBlendState, dwIndexOffset);
		return;
	}

	Command& command = m_aCommand.back();

	if (command.m_eBlendState == eBlendState)
		command.m_dwIndexCount += OPTIMIZED_SURFACE_INDICES;
	else
		m_aCommand.emplace_back(eBlendState, dwIndexOffset);
}

static void d3d_FreeTileTextures(RSurface* pSurface)
{
	SurfaceTile* pTile = pSurface->m_pTile;
	if (pTile == nullptr)
		return;

	RELEASE_INTERFACE(pTile->m_pTexture, g_szFreeError_TXR);
	RELEASE_INTERFACE(pTile->m_pResourceView, g_szFreeError_RVW);
}

void d3d_DestroyTiles(RSurface* pSurface)
{
	SurfaceTile* pTile = pSurface->m_pTile;
	if (pTile == nullptr)
		return;

	if (pTile->m_dwBankedVBIndex != UINT32_MAX)
		g_GlobalMgr.GetBankedVertexBuffer(pTile->m_dwBankedVBIndex).m_bInUse = false;

	if (pTile->m_pTexture != nullptr)
	{
		uint32 dwRefCount = pTile->m_pTexture->Release();
		if (dwRefCount)
			AddDebugMessage(0, g_szFreeError_TXR, dwRefCount);

		pTile->m_pTexture = nullptr;
	}

	RELEASE_INTERFACE(pTile->m_pTexture, g_szFreeError_TXR);

	if (pTile->m_pResourceView != nullptr) 
	{
		pTile->m_pResourceView->Release(); 
		pTile->m_pResourceView = nullptr;
	};

	delete pSurface->m_pTile;
	pSurface->m_pTile = nullptr;
}

void d3d_UnoptimizeSurface(HLTBUFFER hSurface)
{
	if (hSurface == nullptr)
		return;

	d3d_DestroyTiles((RSurface*)hSurface);
}

static bool d3d_CreateTileVertexBuffer(SurfaceTile* pTile)
{	
	// TODO - initialize buffer?
	pTile->m_dwBankedVBIndex = 
		g_GlobalMgr.AssignBankedVertexBuffer(nullptr, sizeof(InternalSurfaceVertex) * OPTIMIZED_SURFACE_VERTICES);

	return (pTile->m_dwBankedVBIndex != UINT32_MAX);
}

static void d3d_ReuploadTileSurface(RSurface* pSurface)
{
	if (pSurface->m_pTile != nullptr && pSurface->m_pTile->m_pTexture)
		g_D3DDevice.GetDeviceContext()->CopyResource(pSurface->m_pTile->m_pTexture, pSurface->m_pSurface);
}

static bool d3d_CreateTileResourceView(RSurface* pSurface, bool bUploadTexture)
{
	D3D11_TEXTURE2D_DESC texDesc = { };

	texDesc.Width = pSurface->m_dwWidth;
	texDesc.Height = pSurface->m_dwHeight;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = (DXGI_FORMAT)g_D3DDevice.GetModeInfo()->m_dwFormat;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;

	ID3D11Texture2D* pTexture;
	HRESULT hResult = g_D3DDevice.GetDevice()->CreateTexture2D(&texDesc, nullptr, &pTexture);
	if (FAILED(hResult))
		return false;

	D3D11_SHADER_RESOURCE_VIEW_DESC resourceDesc = { };
	resourceDesc.Format = texDesc.Format;

	resourceDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	resourceDesc.Texture2D.MipLevels = texDesc.MipLevels;

	ID3D11ShaderResourceView* pResourceView;
	hResult = g_D3DDevice.GetDevice()->CreateShaderResourceView(pTexture, &resourceDesc, &pResourceView);
	if (FAILED(hResult))
	{
		pTexture->Release();
		return false;
	}

	pSurface->m_pTile->m_pResourceView = pResourceView;

	if (bUploadTexture)
	{
		g_D3DDevice.GetDeviceContext()->CopyResource(pTexture, pSurface->m_pSurface);
		pSurface->m_pTile->m_pTexture = pTexture;
	}

	return true;
}

static bool d3d_UpdateTileVertexBuffer_Blit(LTRect* pCurSrcRect, LTRect* pCurDestRect, RSurface* pSurface)
{
	SurfaceTile* pTile = pSurface->m_pTile;

	// TODO - memcmp?
	if (!pCurSrcRect->CompareTo(&pTile->m_rcLastSrcRect) || !pCurDestRect->CompareTo(&pTile->m_rcLastDestRect))
	{	
		auto lambdaUpdate = [pCurSrcRect, pCurDestRect, pSurface](D3D11_MAPPED_SUBRESOURCE* pSubResource)
			{
				InternalSurfaceVertex* pVertices = (InternalSurfaceVertex*)pSubResource->pData;

				float fLeft = (float)pCurDestRect->m_nLeft;
				float fRight = (float)pCurDestRect->m_nRight;
				float fTop = (float)pCurDestRect->m_nTop;
				float fBottom = (float)pCurDestRect->m_nBottom;

				pVertices[0].vPosition = { fLeft, fTop, 0.0f };
				pVertices[1].vPosition = { fRight, fBottom, 0.0f };
				pVertices[2].vPosition = { fLeft, fBottom, 0.0f };
				pVertices[3].vPosition = { fRight, fTop, 0.0f };

				float fTexLeft = (float)pCurSrcRect->m_nLeft / (float)pSurface->m_dwWidth;
				float fTexRight = (float)pCurSrcRect->m_nRight / (float)pSurface->m_dwWidth;
				float fTexTop = (float)pCurSrcRect->m_nTop / (float)pSurface->m_dwHeight;
				float fTexBottom = (float)pCurSrcRect->m_nBottom / (float)pSurface->m_dwHeight;

				pVertices[0].vTexCoords = { fTexLeft, fTexTop };
				pVertices[1].vTexCoords = { fTexRight, fTexBottom };
				pVertices[2].vTexCoords = { fTexLeft, fTexBottom };
				pVertices[3].vTexCoords = { fTexRight, fTexTop };
			};

		if (!g_GlobalMgr.UpdateGenericBufferEx(g_GlobalMgr.GetBankedVertexBuffer(pTile->m_dwBankedVBIndex).m_pBuffer,
			D3D11_MAP_WRITE_DISCARD, lambdaUpdate))
		{
			return false;
		}
		
		pTile->m_rcLastSrcRect = *pCurSrcRect;
		pTile->m_rcLastDestRect = *pCurDestRect;
	}

	return true;
}

static bool d3d_WarpPtsChanged(LTWarpPt* pCurWarpPts, LTWarpPt* pOldWarpPts)
{
	for (uint32 i = 0; i < OPTIMIZED_SURFACE_WARP_PTS; i++)
	{
		if (!pCurWarpPts[i].CompareTo(&pOldWarpPts[i]))
			return true;
	}

	return false;
}

static bool d3d_UpdateTileVertexBuffer_Warp(LTRect* pCurSrcRect, LTRect* pCurDestRect, LTWarpPt* pWarpPts, RSurface* pSurface)
{
	SurfaceTile* pTile = pSurface->m_pTile;

	// TODO - memcmp?
	if (!pCurSrcRect->CompareTo(&pTile->m_rcLastSrcRect) || !pCurDestRect->CompareTo(&pTile->m_rcLastDestRect)
		|| d3d_WarpPtsChanged(pWarpPts, pTile->m_aLastWarpPts))
	{
		auto lambdaUpdate = [pCurSrcRect, pCurDestRect, pWarpPts, pSurface](D3D11_MAPPED_SUBRESOURCE* pSubResource)
			{
				DirectX::XMVECTOR vWarpStart = DirectX::XMVectorSet(pWarpPts[0].m_fDestX, pWarpPts[0].m_fDestY, 0.0f, 1.0f);
				DirectX::XMVECTOR vWarpDirX = DirectX::XMVectorSet(pWarpPts[1].m_fDestX, pWarpPts[1].m_fDestY, 0.0f, 1.0f) - vWarpStart;
				DirectX::XMVECTOR vWarpDirY = DirectX::XMVectorSet(pWarpPts[3].m_fDestX, pWarpPts[3].m_fDestY, 0.0f, 1.0f) - vWarpStart;

				float fSrcRectWidth = (float)(pCurSrcRect->m_nRight - pCurSrcRect->m_nLeft);
				float fSrcRectHeight = (float)(pCurSrcRect->m_nBottom - pCurSrcRect->m_nTop);

				DirectX::XMFLOAT3 vWarpDirXTemp, vWarpDirYTemp;
				DirectX::XMStoreFloat3(&vWarpDirXTemp, vWarpDirX);
				DirectX::XMStoreFloat3(&vWarpDirYTemp, vWarpDirY);

				float fScaleX = Vector_Mag(&vWarpDirXTemp) / fSrcRectWidth;
				float fScaleY = Vector_Mag(&vWarpDirYTemp) / fSrcRectHeight;

				Vector_Norm(&vWarpDirXTemp, fScaleX);
				Vector_Norm(&vWarpDirYTemp, fScaleY);

				vWarpDirX = DirectX::XMLoadFloat3(&vWarpDirXTemp);
				vWarpDirY = DirectX::XMLoadFloat3(&vWarpDirYTemp);

				InternalSurfaceVertex* pVertices = (InternalSurfaceVertex*)pSubResource->pData;

				DirectX::XMVECTOR vPosTemp = vWarpStart + vWarpDirX + vWarpDirY;
				DirectX::XMStoreFloat3(&pVertices[0].vPosition, vPosTemp);
				DirectX::XMStoreFloat3(&pVertices[1].vPosition, vPosTemp + (vWarpDirX * fSrcRectWidth) + (vWarpDirY * fSrcRectHeight));
				DirectX::XMStoreFloat3(&pVertices[2].vPosition, vPosTemp + (vWarpDirY * fSrcRectHeight));
				DirectX::XMStoreFloat3(&pVertices[3].vPosition, vPosTemp + (vWarpDirX * fSrcRectWidth));

				pVertices[0].vTexCoords = { 0.0f, 0.0f };
				pVertices[1].vTexCoords = { 1.0f, 1.0f };
				pVertices[2].vTexCoords = { 0.0f, 1.0f };
				pVertices[3].vTexCoords = { 1.0f, 0.0f };
			};

		if (!g_GlobalMgr.UpdateGenericBufferEx(g_GlobalMgr.GetBankedVertexBuffer(pTile->m_dwBankedVBIndex).m_pBuffer,
			D3D11_MAP_WRITE_DISCARD, lambdaUpdate))
		{
			return false;
		}

		pTile->m_rcLastSrcRect = *pCurSrcRect;
		pTile->m_rcLastDestRect = *pCurDestRect;

		for (uint32 i = 0; i < OPTIMIZED_SURFACE_WARP_PTS; i++)
			pTile->m_aLastWarpPts[i] = pWarpPts[i];
	}

	return true;
}

static void d3d_SetBuffersAndTopology(SurfaceTile* pTile)
{
	g_RenderShaderMgr.SetVertexResource(VRS_Main, g_GlobalMgr.GetBankedVertexBuffer(pTile->m_dwBankedVBIndex).m_pBuffer,
		sizeof(InternalSurfaceVertex), 0);
	g_RenderShaderMgr.SetIndexBuffer16(g_GlobalMgr.GetIndexBuffer_Opt2D(), 0);
	g_RenderShaderMgr.SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

bool d3d_OptimizeSurface(HLTBUFFER hSurface, GenericColor dwTransparentColor)
{
	if (hSurface == nullptr)
		return false;

	RSurface* pSurface = (RSurface*)hSurface;

	if (pSurface->m_pTile == nullptr)
	{
		pSurface->m_pTile = new SurfaceTile();
		
		if (!d3d_CreateTileVertexBuffer(pSurface->m_pTile) ||
			!d3d_CreateTileResourceView(pSurface, true))
		{
			d3d_DestroyTiles(pSurface);
			return false;
		}
	}
	else if (pSurface->m_dwFlags & RS_FLAG_STAGING_UPDATED)
	{
		d3d_ReuploadTileSurface(pSurface);
		pSurface->m_dwFlags &= ~RS_FLAG_STAGING_UPDATED;
	}

	pSurface->m_bTileTransparent = (dwTransparentColor.dwVal != 0xFFFFFFFF);
	
	return true;
}

void d3d_Unoptimized2D_SetStates()
{
	g_RenderStateMgr.SetStencilState(STENCIL_STATE_NoZ);
	g_RenderStateMgr.SetBlendState(BLEND_STATE_Alpha);
	g_RenderStateMgr.SetRasterState(RASTER_STATE_Default);

	if (g_CV_FilterUnoptimized.m_Val)
		g_RenderStateMgr.SetSamplerStates(SAMPLER_STATE_Linear);
	else
		g_RenderStateMgr.SetSamplerStates(SAMPLER_STATE_Point);
}

bool d3d_StartUnoptimized2D()
{
	d3d_Unoptimized2D_SetStates();

	g_RenderShaderMgr.SetPerFrameParamsVS_Opt2D(g_D3DDevice.GetModeInfo()->m_dwWidth, g_D3DDevice.GetModeInfo()->m_dwHeight);
	g_RenderShaderMgr.SetPerFrameParamsPS_Opt2D(g_CV_D3D11_GammaLevel.m_Val);

	return true;
}

static void d3d_Optimized2D_SetStates(bool bStart)
{
	if (bStart)
	{
		g_RenderStateMgr.SetStencilState(STENCIL_STATE_NoZ);
		g_RenderStateMgr.SetBlendState(BLEND_STATE_Alpha);
		g_RenderStateMgr.SetRasterState(RASTER_STATE_Default);

		g_dwOptimized2DOldSamplerState = g_RenderStateMgr.GetSamplerState(0);

		if (g_CV_Anisotropic.m_Val)
		{
			if (g_CV_FilterOptimized.m_Val)
				g_RenderStateMgr.SetSamplerStates(SAMPLER_STATE_Anisotropic);
			else
				g_RenderStateMgr.SetSamplerStates(SAMPLER_STATE_Point);
		}
		else if (g_CV_Bilinear.m_Val)
		{
			if (g_CV_FilterOptimized.m_Val)
				g_RenderStateMgr.SetSamplerStates(SAMPLER_STATE_Linear);
			else
				g_RenderStateMgr.SetSamplerStates(SAMPLER_STATE_Point);
		}
		else
		{
			g_RenderStateMgr.SetSamplerStates(SAMPLER_STATE_Point);
		}
	}
	else
	{
		g_RenderStateMgr.SetStencilState(STENCIL_STATE_Default);
		g_RenderStateMgr.SetBlendState(BLEND_STATE_Default);
		g_RenderStateMgr.SetRasterState(RASTER_STATE_Default);

		g_RenderStateMgr.SetSamplerStates(g_dwOptimized2DOldSamplerState);
	}
}

bool d3d_StartOptimized2D()
{
	if (/*g_D3DDevice.GetDevice() == nullptr || */!g_D3DDevice.IsIn3D())
		return false;

	if (g_bInOptimized2D) 
		return true;

	g_RenderShaderMgr.SetPerFrameParamsVS_Opt2D(g_D3DDevice.GetModeInfo()->m_dwWidth, g_D3DDevice.GetModeInfo()->m_dwHeight);
	g_RenderShaderMgr.SetPerFrameParamsPS_Opt2D(g_CV_D3D11_GammaLevel.m_Val);

	d3d_Optimized2D_SetStates(true);
	
	g_bInOptimized2D = true;
	return true;
}

void d3d_EndOptimized2D()
{
	if (/*g_D3DDevice.GetDevice() == nullptr || */!g_D3DDevice.IsIn3D() || !g_bInOptimized2D) 
		return;

	g_GlobalMgr.GetOptimizedSurfaceBatchMgr()->RenderBatch();

	d3d_Optimized2D_SetStates(false);

	g_bInOptimized2D = false;
}

LTBOOL d3d_IsInOptimized2D()
{
	return g_bInOptimized2D;
}

LTBOOL d3d_SetOptimized2DBlend(LTSurfaceBlend eBlend)
{
	g_dwOptimized2DBlend = eBlend;
	return TRUE;
}

LTBOOL d3d_GetOptimized2DBlend(LTSurfaceBlend& eBlend)
{
	eBlend = (LTSurfaceBlend)g_dwOptimized2DBlend;
	return TRUE;
}

LTBOOL d3d_SetOptimized2DColor(HLTCOLOR dwColor)
{
	g_dwOptimized2DColor = 0xFF000000 | (dwColor & 0x00FFFFFF);
	return TRUE;
}

LTBOOL d3d_GetOptimized2DColor(HLTCOLOR& dwColor)
{
	dwColor = g_dwOptimized2DColor;
	return TRUE;
}

void d3d_BlitToScreen3D(BlitRequest* pRequest)
{
	RSurface* pSurface = (RSurface*)pRequest->m_hBuffer;

	if (pSurface->m_pTile == nullptr)
		return;

	if (g_CV_BatchOptimizedSurfaces.m_Val)
	{
		g_GlobalMgr.GetOptimizedSurfaceBatchMgr()->BatchBlit(pRequest, (LTSurfaceBlend)g_dwOptimized2DBlend,
			pSurface, false);
	}
	else
	{
		if (pSurface->m_bTileTransparent || pRequest->m_fAlpha != 1.0f)
			d3d_SetOptimized2DBlendState((LTSurfaceBlend)g_dwOptimized2DBlend);
		else
			g_RenderStateMgr.SetBlendState(BLEND_STATE_Default);

		if (!d3d_UpdateTileVertexBuffer_Blit(pRequest->m_pSrcRect, pRequest->m_pDestRect, pSurface))
			return;

		d3d_SetBuffersAndTopology(pSurface->m_pTile);

		CRenderShader_OptimizedSurface* pRenderShader = g_RenderShaderMgr.GetRenderShader<CRenderShader_OptimizedSurface>();

		uint32 dwTransparentColor = pRequest->m_dwTransparentColor.dwVal;
		DirectX::XMFLOAT3 vTransparentColor =
		{
			RGBA_GETFR(dwTransparentColor) * MATH_ONE_OVER_255,
			RGBA_GETFG(dwTransparentColor) * MATH_ONE_OVER_255,
			RGBA_GETFB(dwTransparentColor) * MATH_ONE_OVER_255,
		};

		DirectX::XMFLOAT3 vOptimized2DColor =
		{
			RGBA_GETFR(g_dwOptimized2DColor) * MATH_ONE_OVER_255,
			RGBA_GETFG(g_dwOptimized2DColor) * MATH_ONE_OVER_255,
			RGBA_GETFB(g_dwOptimized2DColor) * MATH_ONE_OVER_255
		};

		if (!pRenderShader->SetPerObjectParams(pSurface->m_pTile->m_pResourceView, &vTransparentColor, pRequest->m_fAlpha,
			&vOptimized2DColor))
			return;

		pRenderShader->Render();
	}
}

void d3d_WarpToScreen3D(BlitRequest* pRequest)
{
	RSurface* pSurface = (RSurface*)pRequest->m_hBuffer;

	if (pSurface->m_pTile == nullptr)
		return;

	if (g_CV_BatchOptimizedSurfaces.m_Val)
	{
		g_GlobalMgr.GetOptimizedSurfaceBatchMgr()->BatchBlit(pRequest, (LTSurfaceBlend)g_dwOptimized2DBlend,
			pSurface, true);
	}
	else
	{
		if (pSurface->m_bTileTransparent || pRequest->m_fAlpha != 1.0f)
			d3d_SetOptimized2DBlendState((LTSurfaceBlend)g_dwOptimized2DBlend);
		else
			g_RenderStateMgr.SetBlendState(BLEND_STATE_Default);

		if (!d3d_UpdateTileVertexBuffer_Warp(pRequest->m_pSrcRect, pRequest->m_pDestRect, pRequest->m_pWarpPts, pSurface))
			return;

		d3d_SetBuffersAndTopology(pSurface->m_pTile);

		CRenderShader_OptimizedSurface* pRenderShader = g_RenderShaderMgr.GetRenderShader<CRenderShader_OptimizedSurface>();

		uint32 dwTransparentColor = pRequest->m_dwTransparentColor.dwVal;
		DirectX::XMFLOAT3 vTransparentColor =
		{
			RGBA_GETFR(dwTransparentColor) * MATH_ONE_OVER_255,
			RGBA_GETFG(dwTransparentColor) * MATH_ONE_OVER_255,
			RGBA_GETFB(dwTransparentColor) * MATH_ONE_OVER_255,
		};

		DirectX::XMFLOAT3 vOptimized2DColor =
		{
			RGBA_GETFR(g_dwOptimized2DColor) * MATH_ONE_OVER_255,
			RGBA_GETFG(g_dwOptimized2DColor) * MATH_ONE_OVER_255,
			RGBA_GETFB(g_dwOptimized2DColor) * MATH_ONE_OVER_255
		};

		if (!pRenderShader->SetPerObjectParams(pSurface->m_pTile->m_pResourceView, &vTransparentColor, pRequest->m_fAlpha,
			&vOptimized2DColor))
			return;

		pRenderShader->Render();
	}
}

void d3d_BlitToScreen3D(InternalBlitRequest* pRequest)
{
	RSurface* pSurface = pRequest->m_pSurface;

	if (!(pRequest->m_dwFlags & RS_FLAG_ALLOW_NULL_SURFACE) && pSurface->m_pTile == nullptr)
		return;

	switch (pRequest->m_dwShaderID)
	{
		case SHADER_SolidDrawing:
		{
			d3d_PostProcess_SolidDrawing(pSurface->m_pTile->m_pResourceView);
			break;
		}

		case SHADER_ToneMap:
		{
			d3d_PostProcess_ToneMap(pSurface->m_pTile->m_pResourceView);
			break;
		}

		case SHADER_ScreenFX:
		{
			d3d_PostProcess_ScreenFX(pSurface->m_pTile->m_pResourceView);
			break;
		}

		case SHADER_ClearScreen:
		{
			d3d_PostProcess_ClearScreen(pRequest->m_pDestRect, (DirectX::XMFLOAT3*)pRequest->m_pData);
			break;
		}

		case SHADER_FullscreenVideo:
		{
			if (!d3d_UpdateTileVertexBuffer_Blit(pRequest->m_pSrcRect, pRequest->m_pDestRect, pSurface))
				return;

			d3d_SetBuffersAndTopology(pSurface->m_pTile);

			CRenderShader_FullScreenVideo* pRenderShader = g_RenderShaderMgr.GetRenderShader<CRenderShader_FullScreenVideo>();

			pRenderShader->SetPerObjectParams(pSurface->m_pTile->m_pResourceView);
			pRenderShader->Render();

			break;
		}

		case SHADER_OptimizedSurface:
		default:
		{
			if (!d3d_UpdateTileVertexBuffer_Blit(pRequest->m_pSrcRect, pRequest->m_pDestRect, pSurface))
				return;

			d3d_SetBuffersAndTopology(pSurface->m_pTile);

			CRenderShader_OptimizedSurface* pRenderShader = g_RenderShaderMgr.GetRenderShader<CRenderShader_OptimizedSurface>();

			DirectX::XMFLOAT3 vOne = { 1.0f, 1.0f, 1.0f };
			DirectX::XMFLOAT3 vOptimized2DColor =
			{
				RGBA_GETFR(g_dwOptimized2DColor) * MATH_ONE_OVER_255,
				RGBA_GETFG(g_dwOptimized2DColor) * MATH_ONE_OVER_255,
				RGBA_GETFB(g_dwOptimized2DColor) * MATH_ONE_OVER_255
			};

			if (!pRenderShader->SetPerObjectParams(pSurface->m_pTile->m_pResourceView, &vOne, 1.0f, &vOptimized2DColor))
				return;

			pRenderShader->Render();
			break;
		}
	}
}
