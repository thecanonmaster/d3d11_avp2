#ifndef __D3D_DEVICE_H__
#define __D3D_DEVICE_H__

#ifndef __D3D11_H__
#include <d3d11_1.h>
#define __D3D11_H__
#endif

#include "d3d_shell.h"
#include "d3d_surface.h"
#include "framelimiter.h"

#define SC_BUFFER_COUNT_DEFAULT	2
#define SC_BUFFER_COUNT_TRIPLE	3

#define VIEWPORT_REALLY_CLOSE_MIN_Z	0.0f
#define VIEWPORT_REALLY_CLOSE_MAX_Z	0.1f
#define VIEWPORT_DEFAULT_MIN_Z		0.1f
#define VIEWPORT_DEFAULT_MAX_Z		1.0f

struct SolidDrawingData
{
	ID3D11Texture2D*			m_pSurfaceRAM;
	ID3D11Texture2D*			m_pSurfaceGPU;
	ID3D11ShaderResourceView*	m_pShaderResourceView;
	uint32						m_dwWidth;
	uint32						m_dwHeight;
};

struct BackBufferData
{
	ID3D11Texture2D*	m_pSurfaces[SC_BUFFER_COUNT_TRIPLE];
	ID3D11Texture2D*	m_pCopySurface;
	uint32				m_dwWidth;
	uint32				m_dwHeight;
};

class CD3D_Device
{

public:

	CD3D_Device()
	{	
		Reset();
	}

	~CD3D_Device()
	{
		FreeAll();
	}

	bool	CreateDevice(D3DAdapterInfo* pAdapterInfo, D3DOutputInfo* pOutputInfo, D3DModeInfo* pModeInfo);

	void	ResetDeviceVars();
	void	FreeDevice();
	void	Reset();
	void	FreeAll();

	static bool	Start3D();
	static bool	End3D();
	static bool	IsIn3D();

	void	SetDefaultRenderStates();
	void	SetupFullViewport(float fMinZ = VIEWPORT_DEFAULT_MIN_Z, float fMaxZ = VIEWPORT_DEFAULT_MAX_Z);

	bool	SetMode(D3DModeInfo* pMode);
	void	SetSwapChainParams(DXGI_SWAP_CHAIN_DESC* pDesc, D3DModeInfo* pModeInfo);
	void	SetupTempViewport(uint32 dwLeft, uint32 dwRight, uint32 dwTop, uint32 dwBottom, float fMinZ, float fMaxZ);
	void	SetupViewport(uint32 dwLeft, uint32 dwRight, uint32 dwTop, uint32 dwBottom, float fMinZ, float fMaxZ);
	void	RestoreViewport();

	D3DAdapterInfo*	GetAdapterInfo() { return m_pAdapterInfo; }
	D3DOutputInfo*	GetOutputInfo() { return m_pOutputInfo; }
	D3DModeInfo*	GetModeInfo() { return m_pModeInfo; }

	bool				CreateSolidDrawingSurface(uint32 dwWidth, uint32 dwHeight, bool bForceRecreate, int nInitByte);
	SolidDrawingData*	GetSolidDrawingData() { return &m_SolidDrawingData; }

	BackBufferData*	GetBackBufferData() { return &m_BackBufferData; };
	bool			CreateBackBufferCopy(uint32 dwWidth, uint32 dwHeight);

	IDXGISwapChain*			GetSwapChain() { return m_pSwapChain; }
	ID3D11Device1*			GetDevice() { return m_pDevice; }
	ID3D11DeviceContext1*	GetDeviceContext() { return m_pDeviceContext; }

	void	PreSwapJobs();
	void	PostSwapJobs();

	FrameLimiter*	GetFrameLimiter() { return &m_FrameLimiter; }

	bool	m_bIn3D;

private:

	D3DAdapterInfo* m_pAdapterInfo;
	D3DOutputInfo*	m_pOutputInfo;
	D3DModeInfo*	m_pModeInfo;

	SolidDrawingData	m_SolidDrawingData;
	BackBufferData		m_BackBufferData;

	IDXGISwapChain*	m_pSwapChain;

	ID3D11Device1*			m_pDevice;
	ID3D11DeviceContext1*	m_pDeviceContext;

	RECT	m_rcViewport;
	float	m_fViewportMinZ;
	float	m_fViewportMaxZ;

	FrameLimiter	m_FrameLimiter;
};

extern CD3D_Device g_D3DDevice;

#endif