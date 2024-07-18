#include "pch.h"

#include "d3d_surface.h"
#include "d3d_device.h"
#include "common_stuff.h"
#include "d3d_optimizedsurface.h"
#include "d3d_utils.h"
#include "rendererconsolevars.h"
#include "d3d_init.h"
#include "rendertargetmgr.h"
#include "rendertarget.h"
#include "d3d_shader_optimizedsurface.h"
#include "rendershadermgr.h"
#include "d3d_postprocess.h"
#include "renderstatemgr.h"
#include "globalmgr.h"
#include "d3d_draw.h"

static bool g_bScreenLocked = false;
uint32 g_dwScreenLocksPerFrame = 0;

static ID3D11Texture2D* d3d_CreateStagingTexture(int nWidth, int nHeight, uint32 dwCPUFlags)
{
	D3D11_TEXTURE2D_DESC desc = { };

	desc.Width = nWidth;
	desc.Height = nHeight;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = (DXGI_FORMAT)g_D3DDevice.GetModeInfo()->m_dwFormat;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_STAGING;
	desc.BindFlags = 0;
	desc.CPUAccessFlags = dwCPUFlags;
	desc.MiscFlags = 0;

	ID3D11Texture2D* pTexture = nullptr;
	HRESULT hResult = g_D3DDevice.GetDevice()->CreateTexture2D(&desc, nullptr, &pTexture);

	if (FAILED(hResult))
		return nullptr;

	return pTexture;
}

HLTBUFFER d3d_CreateSurface(int nWidth, int nHeight)
{
	/*if (g_D3DDevice.GetDevice() == nullptr)
		return nullptr;*/

	if (nWidth == 0 || nHeight == 0) 
		return nullptr;

	RSurface* pSurface = new RSurface();

	ID3D11Texture2D* pTexture;
	pTexture = d3d_CreateStagingTexture(nWidth, nHeight, D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE);

	if (pTexture == nullptr)
		return nullptr;
	
	pSurface->m_pSurface = pTexture;
	pSurface->m_dwWidth = nWidth;
	pSurface->m_dwHeight = nHeight;

	D3D11_MAPPED_SUBRESOURCE subResource;
	HRESULT hResult = g_D3DDevice.GetDeviceContext()->Map(pSurface->m_pSurface, 0, D3D11_MAP_READ, 0, &subResource);
	if (FAILED(hResult))
	{
		pSurface->m_dwPitch = pSurface->m_dwWidth << 2;
		AddDebugMessage(0, "Failed to obtain D3D surface's row pitch (%d x %d)", nWidth, nHeight);
	}
	else 
	{
		pSurface->m_dwPitch = subResource.RowPitch;
	}

	g_D3DDevice.GetDeviceContext()->Unmap(pSurface->m_pSurface, 0);

	return (HLTBUFFER)pSurface;
}

void d3d_DeleteSurface(HLTBUFFER hSurface)
{
	if (hSurface == nullptr)
		return;

	RSurface* pSurface = (RSurface*)hSurface;

	d3d_DestroyTiles(pSurface);

	if (pSurface->m_pSurface != nullptr)
	{
		uint32 dwRefCount = pSurface->m_pSurface->Release();
		if (dwRefCount)
			AddDebugMessage(0, "Failed to free D3D surface (count = %d)", dwRefCount);
	}

	delete pSurface;
}

void d3d_GetSurfaceInfo(HLTBUFFER hSurface, uint32* pWidth, uint32* pHeight, int* pPitch)
{
	if (hSurface == nullptr)
		return;

	RSurface* pSurface = (RSurface*)hSurface;

	if (pWidth != nullptr)	
		*pWidth = pSurface->m_dwWidth;

	if (pHeight != nullptr)
		*pHeight = pSurface->m_dwHeight;

	if (pPitch != nullptr)
		*pPitch = pSurface->m_dwPitch;
}

void* d3d_LockSurface(HLTBUFFER hSurface)
{
	if (hSurface == nullptr)
		return nullptr;

	RSurface* pSurface = (RSurface*)hSurface;

	if (pSurface->m_pSurface == nullptr || (pSurface->m_dwFlags & RS_FLAG_LOCKED))
		return nullptr;

	D3D11_MAPPED_SUBRESOURCE subResource;
	HRESULT hResult = g_D3DDevice.GetDeviceContext()->Map(pSurface->m_pSurface, 0, D3D11_MAP_READ_WRITE, 0, &subResource);
	if (FAILED(hResult))
		return nullptr;

	pSurface->m_dwFlags |= RS_FLAG_LOCKED;

	return subResource.pData;
}

void d3d_UnlockSurface(HLTBUFFER hSurface)
{
	if (hSurface == nullptr)
		return;

	RSurface* pSurface = (RSurface*)hSurface;

	if (pSurface->m_pSurface == nullptr)
		return;

	g_D3DDevice.GetDeviceContext()->Unmap(pSurface->m_pSurface, 0);

	pSurface->m_dwFlags &= ~RS_FLAG_LOCKED;
	pSurface->m_dwFlags |= RS_FLAG_STAGING_UPDATED;
}

bool d3d_SaveTextureAsTGA(const char* szFilename, ID3D11Texture2D* pTexture, uint32 dwArrayIndex, bool bPurgeAlpha)
{
	D3D11_TEXTURE2D_DESC desc;
	pTexture->GetDesc(&desc);

	desc.BindFlags = 0;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	desc.Usage = D3D11_USAGE_STAGING;

	ID3D11Texture2D* pCopyBuffer = nullptr;
	HRESULT hResult = g_D3DDevice.GetDevice()->CreateTexture2D(&desc, nullptr, &pCopyBuffer);

	if (FAILED(hResult))
	{
		AddDebugMessage(0, "Failed to create texture buffer for TGA save (%08X)", hResult);
		return false;
	}

	g_D3DDevice.GetDeviceContext()->CopyResource(pCopyBuffer, pTexture);

	D3D11_MAPPED_SUBRESOURCE subResource;
	hResult = g_D3DDevice.GetDeviceContext()->Map(pCopyBuffer, 0, D3D11_MAP_READ, 0, &subResource);
	if (FAILED(hResult))
	{
		pCopyBuffer->Release();
		AddDebugMessage(0, "Failed to lock texture buffer for TGA save (%08X)", hResult);
		return false;
	}

	FILE* pFile = nullptr;
	fopen_s(&pFile, szFilename, "wb");
	if (pFile == nullptr)
	{
		pCopyBuffer->Release();
		AddDebugMessage(0, "Failed to open file (%s) for TGA save", szFilename);
		return false;
	}

	uint32* pBuffer = (uint32*)calloc(desc.Width * desc.Height, sizeof(uint32));
	if (pBuffer == nullptr)
	{
		fclose(pFile);
		pCopyBuffer->Release();
		AddDebugMessage(0, "Failed to allocate memory buffer for TGA save");
		return false;
	}

	TGA_Header tgaHeader(desc.Width, desc.Height, true);
	TGA_Footer tgaFooter;

	fwrite(&tgaHeader, sizeof(TGA_Header), 1, pFile);

	uint32 dwPitchFix = (subResource.RowPitch - (desc.Width << 2)) >> 2;
	uint32* pCur = pBuffer;
	uint32* pData = (uint32*)subResource.pData + (desc.Width * desc.Height * dwArrayIndex);
	for (uint32 y = 0; y < desc.Height; y++)
	{
		uint32* pLine = pData + ((desc.Height - y - 1) * (desc.Width + dwPitchFix));
		for (uint32 x = 0; x < desc.Width; x++)
		{
			*pCur = bPurgeAlpha ? *pLine | 0xFF000000 : *pLine;
			pCur++;
			pLine++;
		}
	}

	fwrite(pBuffer, desc.Width * desc.Height << 2, 1, pFile);

	fwrite(&tgaFooter, sizeof(TGA_Footer), 1, pFile);

	fclose(pFile);

	g_D3DDevice.GetDeviceContext()->Unmap(pCopyBuffer, 0);
	pCopyBuffer->Release();

	free(pBuffer);

	return true;
}

void d3d_MakeScreenShot(const char* szFilename)
{
#if defined(DEBUG) || defined(_DEBUG)
	if (g_CV_ShowPosStats.m_Val > 0.0f)
	{
		AddDebugMessage(0, "[%f] ---- POS STATS BREAK ----", g_fLastClientTime);
		return;
	}
#endif	
	
	d3d_SaveTextureAsTGA(szFilename, g_D3DDevice.GetBackBufferData()->m_pSurfaces[1], 0, true);
	AddConsoleMessage("Screenshot: created %s successfully.", szFilename);
}

void d3d_GetScreenFormat(PFormat* pFormat)
{
	return d3d_D3DFormatToPFormat(g_D3DDevice.GetModeInfo()->m_dwFormat, pFormat);
}

void d3d_BlitFromScreen(BlitRequest* pRequest)
{
	D3DModeInfo* pModeInfo = g_D3DDevice.GetModeInfo();

	if (pRequest->m_pSrcRect->m_nLeft != 0 || pRequest->m_pSrcRect->m_nTop != 0 || 
		pRequest->m_pSrcRect->m_nRight != (int)pModeInfo->m_dwWidth ||
		pRequest->m_pSrcRect->m_nBottom != (int)pModeInfo->m_dwHeight ||
		pRequest->m_pDestRect->m_nLeft != 0 || pRequest->m_pDestRect->m_nTop != 0 ||
		pRequest->m_pDestRect->m_nRight != (int)pModeInfo->m_dwWidth ||
		pRequest->m_pDestRect->m_nBottom != (int)pModeInfo->m_dwHeight)
	{
		NotImplementedMessage("d3d_BlitFromScreen - partial");
		return;
	}

	RSurface* pSurface = (RSurface*)pRequest->m_hBuffer;
	g_D3DDevice.GetDeviceContext()->CopyResource(pSurface->m_pSurface, g_D3DDevice.GetBackBufferData()->m_pSurfaces[1]);
}

void d3d_SwapBuffers(uint32 dwFlags)
{
	/*if (g_D3DDevice.GetDevice() == nullptr)
		return;*/

	g_D3DDevice.PreSwapJobs();
	g_D3DDevice.GetSwapChain()->Present(g_CV_VSync.m_Val, 0);
	g_D3DDevice.PostSwapJobs();
}

bool d3d_LockScreen(int nLeft, int nTop, int nRight, int nBottom, void** pData, int* pPitch)
{
	D3DModeInfo* pModeInfo = g_D3DDevice.GetModeInfo();

	if (nLeft != 0 || nTop != 0 ||
		nRight != (int)pModeInfo->m_dwWidth || nBottom != (int)pModeInfo->m_dwHeight)
	{
		NotImplementedMessage("d3d_LockScreen - partial");
		return false;
	}

	if (g_bScreenLocked)
		return false;

	ID3D11Texture2D* pBackBuffer = g_RenderTargetMgr.GetCurrentRenderTargetObj()->GetRenderTexture();

	D3D11_TEXTURE2D_DESC desc;
	pBackBuffer->GetDesc(&desc);

	if (!g_D3DDevice.CreateBackBufferCopy(desc.Width, desc.Height))
		return false;

	ID3D11Texture2D* pBackBufferCopy = g_D3DDevice.GetBackBufferData()->m_pCopySurface;

	g_D3DDevice.GetDeviceContext()->CopyResource(pBackBufferCopy, pBackBuffer);

	D3D11_MAPPED_SUBRESOURCE subResource = { };
	HRESULT hResult = g_D3DDevice.GetDeviceContext()->Map(pBackBufferCopy, 0, D3D11_MAP_WRITE, 0, &subResource);
	if (FAILED(hResult))
	{
		AddDebugMessage(0, "Failed to lock backbuffer copy surface (%08X)", hResult);
		return false;
	}

	*pData = subResource.pData;

	if (pPitch)
		*pPitch = subResource.RowPitch;
	
	g_bScreenLocked = true;

	return true;
}

void d3d_UnlockScreen()
{
	if (!g_bScreenLocked)
		return;

	ID3D11Texture2D* pBackBuffer = g_RenderTargetMgr.GetCurrentRenderTargetObj()->GetRenderTexture();
	ID3D11Texture2D* pBackBufferCopy = g_D3DDevice.GetBackBufferData()->m_pCopySurface;

	if (pBackBufferCopy != nullptr)
	{
		g_D3DDevice.GetDeviceContext()->Unmap(pBackBufferCopy, 0);
		g_D3DDevice.GetDeviceContext()->CopyResource(pBackBuffer, pBackBufferCopy);
	}

	g_bScreenLocked = false;
}

bool d3d_LockScreen_New(int nLeft, int nTop, int nRight, int nBottom, void** pData, int* pPitch)
{
	D3DModeInfo* pModeInfo = g_D3DDevice.GetModeInfo();

	if (nLeft != 0 || nTop != 0 ||
		nRight != (int)pModeInfo->m_dwWidth || nBottom != (int)pModeInfo->m_dwHeight)
	{
		NotImplementedMessage("d3d_LockScreen - partial");
		return false;
	}

	if (g_bScreenLocked)
		return false;

	if (!g_D3DDevice.CreateSolidDrawingSurface(pModeInfo->m_dwWidth, pModeInfo->m_dwHeight, false, -1))
	{
		AddDebugMessage(0, "Failed to create solid drawing surface");
		return false;
	}

	D3D11_MAPPED_SUBRESOURCE subResource;
	HRESULT hResult = g_D3DDevice.GetDeviceContext()->Map(g_D3DDevice.GetSolidDrawingData()->m_pSurfaceRAM, 0, D3D11_MAP_WRITE, 0, &subResource);
	if (FAILED(hResult))
	{
		AddDebugMessage(0, "Failed to lock solid drawing surface (%08X)", hResult);
		return false;
	}

	memset(subResource.pData, 255, subResource.DepthPitch);

	*pData = subResource.pData;

	if (pPitch)
		*pPitch = subResource.RowPitch;

	g_bScreenLocked = true;

	return true;
}

void d3d_UnlockScreen_New()
{
	if (!g_bScreenLocked)
		return;

	g_GlobalMgr.GetOptimizedSurfaceBatchMgr()->RenderBatch();

	SolidDrawingData* pSolidDrawingData = g_D3DDevice.GetSolidDrawingData();
	g_D3DDevice.GetDeviceContext()->Unmap(pSolidDrawingData->m_pSurfaceRAM, 0);

	g_RenderStateMgr.SaveAllStates();

	g_D3DDevice.GetDeviceContext()->CopyResource(pSolidDrawingData->m_pSurfaceGPU, pSolidDrawingData->m_pSurfaceRAM);
	d3d_PostProcess_SolidDrawing(g_D3DDevice.GetSolidDrawingData()->m_pShaderResourceView);

	g_RenderStateMgr.RestoreAllStates();

	g_bScreenLocked = false;
}

bool d3d_LockScreen_Fast(int nLeft, int nTop, int nRight, int nBottom, void** pData, int* pPitch)
{
	D3DModeInfo* pModeInfo = g_D3DDevice.GetModeInfo();
	
	if (nLeft != 0 || nTop != 0 || 		
		nRight != (int)pModeInfo->m_dwWidth || nBottom != (int)pModeInfo->m_dwHeight)
	{
		NotImplementedMessage("d3d_LockScreen - partial");
		return false;
	}
	
	if (g_bScreenLocked) 
		return false;

	g_dwScreenLocksPerFrame++;

	if (!g_D3DDevice.CreateSolidDrawingSurface(pModeInfo->m_dwWidth, pModeInfo->m_dwHeight, false, -1))
	{
		AddDebugMessage(0, "Failed to create solid drawing surface");
		return false;
	}

	D3D11_MAPPED_SUBRESOURCE subResource;
	HRESULT hResult = g_D3DDevice.GetDeviceContext()->Map(g_D3DDevice.GetSolidDrawingData()->m_pSurfaceRAM, 0, D3D11_MAP_WRITE, 0, &subResource);
	if (FAILED(hResult))
	{
		AddDebugMessage(0, "Failed to lock solid drawing surface (%08X)", hResult);
		return false;
	}

	if (g_dwScreenLocksPerFrame == 1)
		memset(subResource.pData, 255, subResource.DepthPitch);

	// TODO - singlethreaded mode crash
	/*static void* s_pData = nullptr;
	if (s_pData == nullptr)
		s_pData = subResource.pData;
	if (s_pData != subResource.pData)
		Sleep(0);*/

	*pData = subResource.pData;

	if (pPitch)
		*pPitch = subResource.RowPitch;

	g_bScreenLocked = true;

	return true;
}

void d3d_UnlockScreen_Fast()
{
	if (!g_bScreenLocked) 
		return;

	g_D3DDevice.GetDeviceContext()->Unmap(g_D3DDevice.GetSolidDrawingData()->m_pSurfaceRAM, 0);

	g_bScreenLocked = false;
}

void d3d_BlitToScreen(BlitRequest* pRequest)
{
	if (pRequest == nullptr || pRequest->m_hBuffer == nullptr)
		return;

	RSurface* pSurface = (RSurface*)pRequest->m_hBuffer;

	if (g_bInOptimized2D && g_D3DDevice.IsIn3D())
	{
		//d3d_OptimizeSurface(pRequest->m_hBuffer, (pRequest->m_dwBlitOptions == BLIT_TRANSPARENT) ? pRequest->m_dwTransparentColor : 0xFFFFFFFF);
		d3d_OptimizeSurface(pRequest->m_hBuffer, pRequest->m_dwTransparentColor);
	}

	if (pSurface->m_pTile != nullptr && g_bInOptimized2D && g_D3DDevice.IsIn3D())
	{
		d3d_BlitToScreen3D(pRequest);
	}
	else
	{
		if (g_RenderShaderMgr.GetCurrentConstBufferSlotVS(CBS_Opt2D_VS_PerFrame) != CBI_Opt2D_VS_PerFrame)
			d3d_StartUnoptimized2D();

		d3d_Unoptimized2D_SetStates();
		d3d_OptimizeSurface(pRequest->m_hBuffer, 0xFFFFFFFF);
		d3d_BlitToScreen3D(pRequest);
	}
}

void d3d_BlitToScreen(InternalBlitRequest* pRequest)
{
	d3d_OptimizeSurface(pRequest->m_pSurface, 0xFFFFFFFF);

	if (!(pRequest->m_dwFlags & RS_FLAG_ALLOW_NULL_SURFACE) && pRequest->m_pSurface->m_pTile == nullptr)
		return;

	d3d_BlitToScreen3D(pRequest);
}

bool d3d_WarpToScreen(BlitRequest* pRequest)
{
	if (pRequest->m_nWarpPts != OPTIMIZED_SURFACE_WARP_PTS)
		NotImplementedMessage("d3d_WarpToScreen - m_nWarpPts != 4");
	
	if (pRequest == nullptr || pRequest->m_hBuffer == nullptr)
		return false;

	RSurface* pSurface = (RSurface*)pRequest->m_hBuffer;

	if (pSurface->m_pTile != nullptr && g_bInOptimized2D && g_D3DDevice.IsIn3D())
	{
		d3d_WarpToScreen3D(pRequest);
		return true;
	}

	return false;
}