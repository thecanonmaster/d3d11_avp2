#ifndef __D3D_SURFACE_H__
#define __D3D_SURFACE_H__

#ifndef __D3D11_H__
#include <d3d11_1.h>
#define __D3D11_H__
#endif

#include "rendershadermgr.h"

class SurfaceTile;

#pragma pack(push, 1)
struct TGA_Header
{
	TGA_Header(uint16 wWidth, uint16 wHeight, bool bAlpha)
	{
		memset(this, 0, sizeof(TGA_Header));

		m_nImageType = 2;
		m_wWidth = wWidth;
		m_wHeight = wHeight;

		if (bAlpha)
		{
			m_nPixelDepth = 32;
			m_nDescriptor = 8;
		}
		else
		{
			m_nPixelDepth = 24;
		}
	}

	uint8	m_nIDLen;
	uint8	m_nColorMapType;
	uint8	m_nImageType;
	uint8	m_anColorMapSpec[5];
	uint16	m_wOriginX;
	uint16	m_wOriginY;
	uint16	m_wWidth;
	uint16	m_wHeight;
	uint8	m_nPixelDepth;
	uint8	m_nDescriptor;
};

struct TGA_Footer
{
	TGA_Footer() : m_szSignature("TRUEVISION-XFILE")
	{
		m_dwExtensionOffset = 0;
		m_dwDevAreaOffset = 0;
		m_szSignature[16] = '.';
		m_nNull = 0;
	}

	uint32	m_dwExtensionOffset;
	uint32	m_dwDevAreaOffset;
	char	m_szSignature[17];
	uint8	m_nNull;
};
#pragma pack(pop)

#define RS_FLAG_STAGING_UPDATED		(1<<0)
#define RS_FLAG_LOCKED				(1<<1)
#define RS_FLAG_ALLOW_NULL_SURFACE	(1<<2)

class RSurface
{

public:

	RSurface()
	{
		m_pSurface = nullptr;
		m_pTile = nullptr;
		m_dwWidth = 0;
		m_dwHeight = 0;
		m_dwPitch = 0;
		m_bTileTransparent = false;
		m_dwFlags = 0;
	}

	ID3D11Texture2D*	m_pSurface;

	SurfaceTile*	m_pTile;

	uint32	m_dwWidth;
	uint32	m_dwHeight;
	uint32	m_dwPitch;

	bool	m_bTileTransparent;

	uint32	m_dwFlags;
};

struct InternalBlitRequest
{
	InternalBlitRequest()
	{
		m_pSurface = nullptr;
		m_dwFlags = 0;
		m_dwShaderID = SHADER_Invalid;
		m_pSrcRect = nullptr;
		m_pDestRect = nullptr;
		m_pData = nullptr;
	}

	RSurface*	m_pSurface;

	uint32	m_dwFlags;

	uint32	m_dwShaderID;

	LTRect* m_pSrcRect;
	LTRect* m_pDestRect;

	void*	m_pData;
};

HLTBUFFER d3d_CreateSurface(int nWidth, int nHeight);
void d3d_DeleteSurface(HLTBUFFER hSurface);
void d3d_GetSurfaceInfo(HLTBUFFER hSurface, uint32* pWidth, uint32* pHeight, int* pPitch);
void* d3d_LockSurface(HLTBUFFER hSurface);
void d3d_UnlockSurface(HLTBUFFER hSurface);

void d3d_MakeScreenShot(const char* szFilename);
bool d3d_SaveTextureAsTGA(const char* szFilename, ID3D11Texture2D* pTexture, uint32 dwArrayIndex, bool bPurgeAlpha);
void d3d_GetScreenFormat(PFormat* pFormat);
void d3d_BlitFromScreen(BlitRequest* pRequest);
void d3d_BlitToScreen(InternalBlitRequest* pRequest);
void d3d_SwapBuffers(uint32 dwFlags);
bool d3d_LockScreen(int nLeft, int nTop, int nRight, int nBottom, void** pData, int* pPitch);
void d3d_UnlockScreen();
bool d3d_LockScreen_New(int nLeft, int nTop, int nRight, int nBottom, void** pData, int* pPitch);
void d3d_UnlockScreen_New();
bool d3d_LockScreen_Fast(int nLeft, int nTop, int nRight, int nBottom, void** pData, int* pPitch);
void d3d_UnlockScreen_Fast();

void d3d_BlitToScreen(BlitRequest* pRequest);
bool d3d_WarpToScreen(BlitRequest* pRequest);

extern uint32 g_dwScreenLocksPerFrame;

#endif