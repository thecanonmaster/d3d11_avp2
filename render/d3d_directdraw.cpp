#include "pch.h"

#include "d3d_directdraw.h"
#include "d3d_device.h"
#include "d3d_surface.h"
#include "common_stuff.h"
#include "rendershadermgr.h"
#include "d3d_utils.h"
#include "rendererconsolevars.h"
#include "rendertargetmgr.h"

DirectDraw7 g_FakeDirectDraw;
DDS7_BackBuffer g_FakeBackBuffer;

ULONG __stdcall DirectDraw7::AddRef()
{
	m_dwRefs++;
	return m_dwRefs;
}

ULONG __stdcall DirectDraw7::Release()
{
	m_dwRefs--;

	if (!m_dwRefs)
	{
		delete this;
		return 0;
	}

	return m_dwRefs;
}

HRESULT __stdcall DirectDraw7::CreateSurface(LPDDSURFACEDESC2 pDDSurfaceDesc2, LPDIRECTDRAWSURFACE7* ppDDSurface, IUnknown* pUnkOuter)
{
	if (pDDSurfaceDesc2->dwFlags != (DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT) || 
		pDDSurfaceDesc2->ddsCaps.dwCaps != (DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY))
		return DDERR_INVALIDPARAMS;

	DDS7_BinkVideoBuffer* pBinkVideoBuffer = new DDS7_BinkVideoBuffer(pDDSurfaceDesc2);
	if (pBinkVideoBuffer == nullptr || pBinkVideoBuffer->GetRSurface() == nullptr)
		return DDERR_INVALIDPARAMS;

	*ppDDSurface = pBinkVideoBuffer;

	return DD_OK;
}

HRESULT __stdcall DirectDraw7::GetDisplayMode(LPDDSURFACEDESC2 pDDSurfaceDesc2)
{
	D3DModeInfo* pModeInfo = g_D3DDevice.GetModeInfo();

	pDDSurfaceDesc2->dwFlags = (DDSD_REFRESHRATE | DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PITCH);
	pDDSurfaceDesc2->dwWidth = pModeInfo->m_dwWidth;
	pDDSurfaceDesc2->dwHeight = pModeInfo->m_dwHeight;
	pDDSurfaceDesc2->lPitch = pModeInfo->GetPitch();
	pDDSurfaceDesc2->dwRefreshRate = (DWORD)pModeInfo->GetRefreshRate();
	pDDSurfaceDesc2->ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
	pDDSurfaceDesc2->ddpfPixelFormat.dwFlags = DDPF_RGB;
	pDDSurfaceDesc2->ddpfPixelFormat.dwRGBBitCount = 32;
	pDDSurfaceDesc2->ddpfPixelFormat.dwRBitMask = 0x00FF0000;
	pDDSurfaceDesc2->ddpfPixelFormat.dwGBitMask = 0x0000FF00;
	pDDSurfaceDesc2->ddpfPixelFormat.dwBBitMask = 0x000000FF;

	return DD_OK;
}

ULONG __stdcall DirectDrawSurface7::AddRef()
{
	m_dwRefs++;
	return m_dwRefs;
}

ULONG __stdcall DirectDrawSurface7::Release()
{
	m_dwRefs--;

	if (!m_dwRefs)
	{
		delete this;
		return 0;
	}

	return m_dwRefs;
}

HRESULT __stdcall DDS7_BackBuffer::Blt(LPRECT pDestRect, LPDIRECTDRAWSURFACE7 pDDSrcSurface, LPRECT pSrcRect, DWORD dwFlags, LPDDBLTFX pDDBltFx)
{
	// TODO - lock_guard?

	if (pDestRect == nullptr || pSrcRect == nullptr || pDDBltFx != nullptr)
		return DDERR_INVALIDPARAMS;
	
	D3DModeInfo* pInfo = g_D3DDevice.GetModeInfo();

	LTRect rcSrc;
	LTRect rcDest;

	if (g_CV_MoviesKeepAspectRatio.m_Val)
	{
		float fSrcRatio = (float)pSrcRect->right / (float)pSrcRect->bottom;
		float fDestRatio = (float)pDestRect->right / (float)pDestRect->bottom;

		if (fDestRatio >= fSrcRatio)
		{
			float fScale = (float)pDestRect->bottom / (float)pSrcRect->bottom;
			int nDestRight = (int)((float)pSrcRect->right * fScale);
			int nDestLeft = (pDestRect->right - nDestRight) >> 1;
			nDestRight += nDestLeft;
			
			rcSrc = { pSrcRect->left, pSrcRect->top, pSrcRect->right, pSrcRect->bottom };
			rcDest = { nDestLeft, pDestRect->top, nDestRight, pDestRect->bottom };
		} 
		else
		{
			float fScale = (float)pDestRect->right / (float)pSrcRect->right;
			int nDestBottom = (int)((float)pSrcRect->bottom * fScale);
			int nDestTop = (pDestRect->bottom - nDestBottom) >> 1;
			nDestBottom += nDestTop;

			rcSrc = { pSrcRect->left, pSrcRect->top, pSrcRect->right, pSrcRect->bottom };
			rcDest = { pDestRect->left, nDestTop, pDestRect->right, nDestBottom };
		}
	}
	else
	{
		rcSrc = { pSrcRect->left, pSrcRect->top, pSrcRect->right, pSrcRect->bottom };
		rcDest = { pDestRect->left, pDestRect->top, pDestRect->right, pDestRect->bottom };
	}


	InternalBlitRequest request;
	request.m_dwShaderID = SHADER_FullscreenVideo;
	request.m_pSurface = ((DDS7_BinkVideoBuffer*)pDDSrcSurface)->GetRSurface();
	request.m_pSrcRect = &rcSrc;
	request.m_pDestRect = &rcDest;

	d3d_BlitToScreen(&request);
	
	return DD_OK;
}

HRESULT __stdcall DDS7_BackBuffer::Lock(LPRECT pDestRect, LPDDSURFACEDESC2 pDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent)
{
	if (pDestRect != nullptr)
		return DDERR_INVALIDPARAMS;
	
	void* pData;
	int nPitch;
	D3DModeInfo* pModeInfo = g_D3DDevice.GetModeInfo();
	if (!d3d_LockScreen(0, 0, pModeInfo->m_dwWidth, pModeInfo->m_dwHeight, &pData, &nPitch))
		return DDERR_CANTLOCKSURFACE;

	pDDSurfaceDesc->dwFlags = (DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PITCH | DDSD_PIXELFORMAT);
	pDDSurfaceDesc->dwWidth = pModeInfo->m_dwWidth;
	pDDSurfaceDesc->dwHeight = pModeInfo->m_dwHeight;
	pDDSurfaceDesc->lpSurface = pData;
	pDDSurfaceDesc->lPitch = nPitch;
	pDDSurfaceDesc->ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
	pDDSurfaceDesc->ddpfPixelFormat.dwFlags = DDPF_RGB;
	pDDSurfaceDesc->ddpfPixelFormat.dwRGBBitCount = 32;
	pDDSurfaceDesc->ddpfPixelFormat.dwRBitMask = 0x00FF0000;
	pDDSurfaceDesc->ddpfPixelFormat.dwGBitMask = 0x0000FF00;
	pDDSurfaceDesc->ddpfPixelFormat.dwBBitMask = 0x000000FF;
	pDDSurfaceDesc->ddsCaps.dwCaps = (DDSCAPS_3DDEVICE | DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM);
	
	return DD_OK;
}

HRESULT __stdcall DDS7_BackBuffer::Unlock(LPRECT pRect)
{
	d3d_UnlockScreen();	
	return DD_OK;
}

DDS7_BinkVideoBuffer::DDS7_BinkVideoBuffer(LPDDSURFACEDESC2 pDDSurfaceDesc) : DirectDrawSurface7()
{
	m_pSurface = (RSurface*)d3d_CreateSurface(pDDSurfaceDesc->dwWidth, pDDSurfaceDesc->dwHeight);
}

DDS7_BinkVideoBuffer::~DDS7_BinkVideoBuffer()
{
	d3d_DeleteSurface(m_pSurface);
}

ULONG __stdcall DDS7_BinkVideoBuffer::Release()
{
	m_dwRefs--;

	if (!m_dwRefs)
	{
		delete this;
		return 0;
	}

	return m_dwRefs;
}

HRESULT __stdcall DDS7_BinkVideoBuffer::Blt(LPRECT pDestRect, LPDIRECTDRAWSURFACE7 pDDSrcSurface, LPRECT pSrcRect, DWORD dwFlags, LPDDBLTFX pDDBltFx)
{
	if (pDestRect != nullptr || pDDSrcSurface != nullptr || pSrcRect != nullptr || pDDBltFx == nullptr ||
		dwFlags != (DDBLT_DDFX | DDBLT_COLORFILL | DDBLT_WAIT))
		return DDERR_INVALIDPARAMS;

	D3D11_MAPPED_SUBRESOURCE subResource;
	HRESULT hResult = g_D3DDevice.GetDeviceContext()->Map(m_pSurface->m_pSurface, 0, D3D11_MAP_WRITE, 0, &subResource);
	if (FAILED(hResult))
		return DDERR_CANTLOCKSURFACE;

	uint32* pData = (uint32*)subResource.pData;
	uint32 dwRowPitch = subResource.RowPitch >> 2;
	for (uint32 y = 0; y < m_pSurface->m_dwHeight; y++)
	{
		uint32* pLine = pData + (y * dwRowPitch);
		for (uint32 x = 0; x < m_pSurface->m_dwWidth; x++)
			pLine[x] = pDDBltFx->dwFillColor;
	}

	g_D3DDevice.GetDeviceContext()->Unmap(m_pSurface->m_pSurface, 0);

 	return DD_OK;
}

HRESULT __stdcall DDS7_BinkVideoBuffer::Lock(LPRECT pDestRect, LPDDSURFACEDESC2 pDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent)
{
	if (pDestRect != nullptr)
		return DDERR_INVALIDPARAMS;

	void* pData = d3d_LockSurface(m_pSurface);
	if (pData == nullptr)
		return DDERR_INVALIDPARAMS;

	pDDSurfaceDesc->dwFlags = (DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PITCH | DDSD_PIXELFORMAT);
	pDDSurfaceDesc->dwWidth = m_pSurface->m_dwWidth;
	pDDSurfaceDesc->dwHeight = m_pSurface->m_dwHeight;
	pDDSurfaceDesc->lpSurface = pData;
	pDDSurfaceDesc->lPitch = m_pSurface->m_dwPitch;
	pDDSurfaceDesc->ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
	pDDSurfaceDesc->ddpfPixelFormat.dwFlags = DDPF_RGB;
	pDDSurfaceDesc->ddpfPixelFormat.dwRGBBitCount = 32;
	pDDSurfaceDesc->ddpfPixelFormat.dwRBitMask = 0x00FF0000;
	pDDSurfaceDesc->ddpfPixelFormat.dwGBitMask = 0x0000FF00;
	pDDSurfaceDesc->ddpfPixelFormat.dwBBitMask = 0x000000FF;
	pDDSurfaceDesc->ddsCaps.dwCaps = (DDSCAPS_OFFSCREENPLAIN | DDSCAPS_LOCALVIDMEM | DDSCAPS_VIDEOMEMORY);
	
	return DD_OK;
}

HRESULT __stdcall DDS7_BinkVideoBuffer::Unlock(LPRECT pRect)
{
	if (pRect != nullptr)
		return DDERR_INVALIDPARAMS;

	d3d_UnlockSurface(m_pSurface);

	return DD_OK;
}

void* d3d_GetDirectDrawInterface(const char* szQuery)
{
	if (!strcmp(szQuery, "LPDIRECTDRAW"))
		return &g_FakeDirectDraw;
	else if (!strcmp(szQuery, "BACKBUFFER"))
		return &g_FakeBackBuffer;

	return nullptr;
}