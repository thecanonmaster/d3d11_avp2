#ifndef __D3D_DIRECT_DRAW_H__
#define __D3D_DIRECT_DRAW_H__

#ifndef __DDRAW_H__
#include "ddraw.h"
#define __DDRAW_H__
#endif

#include "d3d_surface.h"

class DirectDraw7 : public IDirectDraw7
{

public:

    DirectDraw7() { m_dwRefs = 1; };

    HRESULT __stdcall   QueryInterface(REFIID riid, LPVOID* ppvObject) { __asm { int 3 }; };
    ULONG   __stdcall   AddRef();
    ULONG   __stdcall   Release();

    HRESULT __stdcall   Compact() { __asm { int 3 }; };
    HRESULT __stdcall   CreateClipper(DWORD dwFlags, LPDIRECTDRAWCLIPPER* ppDDClipper, IUnknown* pUnkOuter) { __asm { int 3 }; };
    HRESULT __stdcall   CreatePalette(DWORD dwFlags, LPPALETTEENTRY pDDColorArray, LPDIRECTDRAWPALETTE* ppDDPalette, IUnknown* pUnkOuter) { __asm { int 3 }; }; 
    HRESULT __stdcall   CreateSurface(LPDDSURFACEDESC2 pDDSurfaceDesc2, LPDIRECTDRAWSURFACE7* ppDDSurface, IUnknown* pUnkOuter);
    HRESULT __stdcall   DuplicateSurface(LPDIRECTDRAWSURFACE7 pDDSurface, LPDIRECTDRAWSURFACE7* pDupDDSurface) { __asm { int 3 }; };
    HRESULT __stdcall   EnumDisplayModes(DWORD dwFlags, LPDDSURFACEDESC2 pDDSurfaceDesc2, LPVOID pContext, LPDDENUMMODESCALLBACK2 pEnumModesCallback) { __asm { int 3 }; };
    HRESULT __stdcall   EnumSurfaces(DWORD dwFlags, LPDDSURFACEDESC2 pDDSD2, LPVOID pContext, LPDDENUMSURFACESCALLBACK7 pEnumSurfacesCallback) { __asm { int 3 }; };
    HRESULT __stdcall   FlipToGDISurface() { __asm { int 3 }; };
    HRESULT __stdcall   GetCaps(LPDDCAPS pDDDriverCaps, LPDDCAPS pDDHELCaps) { __asm { int 3 }; };
    HRESULT __stdcall   GetDisplayMode(LPDDSURFACEDESC2 pDDSurfaceDesc2);
    HRESULT __stdcall   GetFourCCCodes(LPDWORD pNumCodes, LPDWORD pCodes) { __asm { int 3 }; };
    HRESULT __stdcall   GetGDISurface(LPDIRECTDRAWSURFACE7* ppGDIDDSSurface) { __asm { int 3 }; };
    HRESULT __stdcall   GetMonitorFrequency(LPDWORD pdwFrequency) { __asm { int 3 }; };
    HRESULT __stdcall   GetScanLine(LPDWORD pdwScanLine) { __asm { int 3 }; };
    HRESULT __stdcall   GetVerticalBlankStatus(LPBOOL pbIsInVB) { __asm { int 3 }; };
    HRESULT __stdcall   Initialize(GUID* pGUID) { __asm { int 3 }; };
    HRESULT __stdcall   RestoreDisplayMode() { __asm { int 3 }; };
    HRESULT __stdcall   SetCooperativeLevel(HWND hWnd, DWORD dwFlags) { __asm { int 3 }; };
    HRESULT __stdcall   SetDisplayMode(DWORD dwWidth, DWORD dwHeight, DWORD dwBPP, DWORD dwRefreshRate, DWORD dwFlags) { __asm { int 3 }; };
    HRESULT __stdcall   WaitForVerticalBlank(DWORD dwFlags, HANDLE hEvent) { __asm { int 3 }; };

    HRESULT __stdcall   GetAvailableVidMem(LPDDSCAPS2 pDDSCaps2, LPDWORD pdwTotal, LPDWORD pdwFree) { __asm { int 3 }; };

    HRESULT __stdcall   GetSurfaceFromDC(HDC hdc, LPDIRECTDRAWSURFACE7* ppDDS) { __asm { int 3 }; };
    HRESULT __stdcall   RestoreAllSurfaces() { __asm { int 3 }; };
    HRESULT __stdcall   TestCooperativeLevel() { __asm { int 3 }; };
    HRESULT __stdcall   GetDeviceIdentifier(LPDDDEVICEIDENTIFIER2 pdddi, DWORD dwFlags) { __asm { int 3 }; };
    HRESULT __stdcall   StartModeTest(LPSIZE pModesToTest, DWORD dwNumEntries, DWORD dwFlags) { __asm { int 3 }; };
    HRESULT __stdcall   EvaluateMode(DWORD dwFlags, DWORD* pSecondsUntilTimeout) { __asm { int 3 }; };

protected:

    ULONG   m_dwRefs;
};

class DirectDrawSurface7 : public IDirectDrawSurface7
{

public:

    DirectDrawSurface7() { m_dwRefs = 1; };

    HRESULT __stdcall   QueryInterface(REFIID riid, LPVOID* ppvObject) { __asm { int 3 }; };
    ULONG   __stdcall   AddRef();
    ULONG   __stdcall   Release();

    HRESULT __stdcall   AddAttachedSurface(LPDIRECTDRAWSURFACE7 pDDSAttachedSurface) { __asm { int 3 }; };
    HRESULT __stdcall   AddOverlayDirtyRect(LPRECT pRect) { __asm { int 3 }; };
    HRESULT __stdcall   Blt(LPRECT pDestRect, LPDIRECTDRAWSURFACE7 pDDSrcSurface, LPRECT pSrcRect, DWORD dwFlags, LPDDBLTFX pDDBltFx) { __asm { int 3 }; };
    HRESULT __stdcall   BltBatch(LPDDBLTBATCH pDDBltBatch, DWORD dwCount, DWORD dwFlags) { __asm { int 3 }; };
    HRESULT __stdcall   BltFast(DWORD dwX, DWORD dwY, LPDIRECTDRAWSURFACE7 pDDSrcSurface, LPRECT pSrcRect, DWORD dwTrans) { __asm { int 3 }; };
    HRESULT __stdcall   DeleteAttachedSurface(DWORD dwFlags, LPDIRECTDRAWSURFACE7 pDDSAttachedSurface) { __asm { int 3 }; };
    HRESULT __stdcall   EnumAttachedSurfaces(LPVOID pContext, LPDDENUMSURFACESCALLBACK7 pEnumSurfacesCallback) { __asm { int 3 }; };
    HRESULT __stdcall   EnumOverlayZOrders(DWORD dwFlags, LPVOID pContext, LPDDENUMSURFACESCALLBACK7 pfnCallback) { __asm { int 3 }; };
    HRESULT __stdcall   Flip(LPDIRECTDRAWSURFACE7 pDDSurfaceTargetOverride, DWORD dwFlags) { __asm { int 3 }; };
    HRESULT __stdcall   GetAttachedSurface(LPDDSCAPS2 pDDSCaps, LPDIRECTDRAWSURFACE7* ppDDAttachedSurface) { __asm { int 3 }; };
    HRESULT __stdcall   GetBltStatus(DWORD dwFlags) { __asm { int 3 }; };
    HRESULT __stdcall   GetCaps(LPDDSCAPS2 pDDSCaps) { __asm { int 3 }; };
    HRESULT __stdcall   GetClipper(LPDIRECTDRAWCLIPPER* ppDDClipper) { __asm { int 3 }; };
    HRESULT __stdcall   GetColorKey(DWORD dwFlags, LPDDCOLORKEY pDDColorKey) { __asm { int 3 }; };
    HRESULT __stdcall   GetDC(HDC* phDC) { __asm { int 3 }; };
    HRESULT __stdcall   GetFlipStatus(DWORD dwFlags) { __asm { int 3 }; };
    HRESULT __stdcall   GetOverlayPosition(LPLONG plX, LPLONG plY) { __asm { int 3 }; };
    HRESULT __stdcall   GetPalette(LPDIRECTDRAWPALETTE* ppDDPalette) { __asm { int 3 }; };
    HRESULT __stdcall   GetPixelFormat(LPDDPIXELFORMAT pDDPixelFormat) { __asm { int 3 }; };
    HRESULT __stdcall   GetSurfaceDesc(LPDDSURFACEDESC2 pDDSurfaceDesc) { __asm { int 3 }; };
    HRESULT __stdcall   Initialize(LPDIRECTDRAW pDD, LPDDSURFACEDESC2 pDDSurfaceDesc) { __asm { int 3 }; };
    HRESULT __stdcall   IsLost() { __asm { int 3 }; };
    HRESULT __stdcall   Lock(LPRECT pDestRect, LPDDSURFACEDESC2 pDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent) { __asm { int 3 }; };
    HRESULT __stdcall   ReleaseDC(HDC hDC) { __asm { int 3 }; };
    HRESULT __stdcall   Restore() { __asm { int 3 }; };
    HRESULT __stdcall   SetClipper(LPDIRECTDRAWCLIPPER pDDClipper) { __asm { int 3 }; };
    HRESULT __stdcall   SetColorKey(DWORD dwFlags, LPDDCOLORKEY pDDColorKey) { __asm { int 3 }; };
    HRESULT __stdcall   SetOverlayPosition(LONG lX, LONG lY) { __asm { int 3 }; };
    HRESULT __stdcall   SetPalette(LPDIRECTDRAWPALETTE pDDPalette) { __asm { int 3 }; };
    HRESULT __stdcall   Unlock(LPRECT pRect) { __asm { int 3 }; };
    HRESULT __stdcall   UpdateOverlay(LPRECT pSrcRect, LPDIRECTDRAWSURFACE7 pDDDestSurface, LPRECT pDestRect, DWORD dwFlags, LPDDOVERLAYFX pDDOverlayFx) { __asm { int 3 }; };
    HRESULT __stdcall   UpdateOverlayDisplay(DWORD dwFlags) { __asm { int 3 }; };
    HRESULT __stdcall   UpdateOverlayZOrder(DWORD dwFlags, LPDIRECTDRAWSURFACE7 pDDSReference) { __asm { int 3 }; };

    HRESULT __stdcall   GetDDInterface(LPVOID* ppDD) { __asm { int 3 }; };
    HRESULT __stdcall   PageLock(DWORD dwFlags) { __asm { int 3 }; };
    HRESULT __stdcall   PageUnlock(DWORD dwFlags) { __asm { int 3 }; };

    HRESULT __stdcall   SetSurfaceDesc(LPDDSURFACEDESC2 pddsd2, DWORD dwFlags) { __asm { int 3 }; };

    HRESULT __stdcall   SetPrivateData(REFGUID guidTag, LPVOID pData, DWORD cbSize, DWORD dwFlags) { __asm { int 3 }; };
    HRESULT __stdcall   GetPrivateData(REFGUID guidTag, LPVOID pBuffer, LPDWORD pcbBufferSize) { __asm { int 3 }; };
    HRESULT __stdcall   FreePrivateData(REFGUID guidTag) { __asm { int 3 }; };
    HRESULT __stdcall   GetUniquenessValue(LPDWORD pValue) { __asm { int 3 }; };
    HRESULT __stdcall   ChangeUniquenessValue() { __asm { int 3 }; };

    HRESULT __stdcall   SetPriority(DWORD dwPriority) { __asm { int 3 }; };
    HRESULT __stdcall   GetPriority(LPDWORD pdwPriority) { __asm { int 3 }; };
    HRESULT __stdcall   SetLOD(DWORD dwMaxLOD) { __asm { int 3 }; };
    HRESULT __stdcall   GetLOD(LPDWORD pdwMaxLOD) { __asm { int 3 }; };

protected:

    ULONG   m_dwRefs;
};

class DDS7_BackBuffer : public DirectDrawSurface7
{

public:

    DDS7_BackBuffer() : DirectDrawSurface7() { };

    HRESULT __stdcall   Blt(LPRECT pDestRect, LPDIRECTDRAWSURFACE7 pDDSrcSurface, LPRECT pSrcRect, DWORD dwFlags, LPDDBLTFX pDDBltFx);
    HRESULT __stdcall   Lock(LPRECT pDestRect, LPDDSURFACEDESC2 pDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent);
    HRESULT __stdcall   Unlock(LPRECT pRect);
};

class DDS7_BinkVideoBuffer : public DirectDrawSurface7
{

public:

    DDS7_BinkVideoBuffer(LPDDSURFACEDESC2 pDDSurfaceDesc);
    ~DDS7_BinkVideoBuffer();

    ULONG   __stdcall   Release();

    HRESULT __stdcall   Blt(LPRECT pDestRect, LPDIRECTDRAWSURFACE7 pDDSrcSurface, LPRECT pSrcRect, DWORD dwFlags, LPDDBLTFX pDDBltFx); // 0x01000c00
    HRESULT __stdcall   Lock(LPRECT pDestRect, LPDDSURFACEDESC2 pDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent);
    HRESULT __stdcall   Unlock(LPRECT pRect);

    RSurface*   GetRSurface() { return m_pSurface;  }

private:
    
    RSurface*   m_pSurface;
};

void* d3d_GetDirectDrawInterface(const char* szQuery);

#endif