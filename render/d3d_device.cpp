#include "pch.h"

#include "d3d_device.h"
#include "common_stuff.h"
#include "d3d_utils.h"
#include "rendererconsolevars.h"
#include "renderstatemgr.h"
#include "rendertargetmgr.h"
#include "rendertarget.h"
#include "rendershadermgr.h"
#include "d3d_optimizedsurface.h"
#include "d3d_postprocess.h"
#include "globalmgr.h"
#include "rendermodemgr.h"

static const char* g_szFreeError_SD_RV = "Failed to free solid drawing shader resource view (count = %d)";
static const char* g_szFreeError_SD_RS = "Failed to free solid drawing RAM surface (count = %d)";
static const char* g_szFreeError_SD_GS = "Failed to free solid drawing GPU surface (count = % d)";
static const char* g_szFreeError_BCS = "Failed to free backbuffer copy surface (count = %d)";
static const char* g_szFreeError_SCH = "Failed to free D3D swap chain (count = %d)";
static const char* g_szFreeError_DCO = "Failed to free D3D device context (count = %d)";
static const char* g_szFreeError_DEV = "Failed to free D3D device (count = %d)";

CD3D_Device g_D3DDevice;

bool CD3D_Device::CreateDevice(D3DAdapterInfo* pAdapterInfo, D3DOutputInfo* pOutputInfo, D3DModeInfo* pModeInfo)
{
	FreeDevice();

	m_pAdapterInfo = pAdapterInfo;
	m_pOutputInfo = pOutputInfo;
	m_pModeInfo = pModeInfo;

	DXGI_SWAP_CHAIN_DESC swapChainDesc = { };
	SetSwapChainParams(&swapChainDesc, pModeInfo);

	ID3D11Device* pDevice;
	ID3D11DeviceContext* pDeviceContext;

	D3D_FEATURE_LEVEL eFeatureLevel = D3D_FEATURE_LEVEL_11_1;
	UINT dwFlags = 0; // D3D11_CREATE_DEVICE_SINGLETHREADED;

#if defined( DEBUG ) || defined( _DEBUG )
	dwFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	// TODO - too hacky
	if (!g_bRunWindowed)
	{
		LONG dwCurrStyle = GetWindowLong(g_hWnd, GWL_STYLE);
		SetWindowLong(g_hWnd, GWL_STYLE, dwCurrStyle | WS_CAPTION | WS_TABSTOP);
	}

	IDXGIAdapter1* pAdapter = g_D3DShell.GetDXGIAdapter(pAdapterInfo);
	HRESULT hResult = D3D11CreateDeviceAndSwapChain(pAdapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, dwFlags, &eFeatureLevel, 1,
		D3D11_SDK_VERSION, &swapChainDesc, &m_pSwapChain, &pDevice, nullptr, &pDeviceContext);

	pAdapter->Release();

	if (FAILED(hResult))
	{	
		FreeDevice();
		return false;
	}

	hResult = pDevice->QueryInterface(__uuidof(ID3D11Device1), (void**)&m_pDevice);
	pDevice->Release();

	if (FAILED(hResult))
	{
		FreeDevice();
		return false;
	}

	hResult = pDeviceContext->QueryInterface(__uuidof(ID3D11DeviceContext1), (void**)&m_pDeviceContext);
	pDeviceContext->Release();

	if (FAILED(hResult))
	{
		FreeDevice();
		return false;
	}

	for (int i = 0; i < SC_BUFFER_COUNT_TRIPLE; i++)
	{
		if (i == SC_BUFFER_COUNT_TRIPLE - 1 && swapChainDesc.BufferCount < SC_BUFFER_COUNT_TRIPLE)
			continue;
		
		hResult = g_D3DDevice.GetSwapChain()->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&m_BackBufferData.m_pSurfaces[i]);
		if (FAILED(hResult))
		{
			FreeDevice();
			return false;
		}
	}

	if (!g_RenderTargetMgr.Init())
	{
		AddDebugMessage(0, "Can't initialize RenderTargetMgr!");
		FreeDevice();
		return false;
	}

	if (!g_RenderStateMgr.Init())
	{
		AddDebugMessage(0, "Can't initialize RenderStateMgr!");
		FreeDevice();
		return false;
	}

	if (!g_RenderShaderMgr.Init())
	{
		AddDebugMessage(0, "Can't initialize RenderShaderMgr!");
		FreeDevice();
		return false;
	}

	g_RenderModeMgr.Init();

	SetDefaultRenderStates();
	SetupFullViewport();

	return true;
}

void CD3D_Device::ResetDeviceVars()
{
	m_pDevice = nullptr;
	m_pSwapChain = nullptr;
	m_pDeviceContext = nullptr;

	m_pAdapterInfo = nullptr;
	m_pOutputInfo = nullptr;
	m_pModeInfo = nullptr;

	m_SolidDrawingData = { };
	m_BackBufferData = { };

	m_bIn3D = false;
}

void CD3D_Device::FreeDevice()
{	
	/*if (m_pDeviceContext != nullptr)
	{
		g_RenderTargetMgr.RT_RemoveFromDevice();
		m_pDeviceContext->Flush();
	}*/

	g_GlobalMgr.FreeAll();

	g_RenderTargetMgr.FreeAll();
	g_RenderStateMgr.FreeAll();
	g_RenderShaderMgr.FreeAll();

	RELEASE_INTERFACE(m_SolidDrawingData.m_pShaderResourceView, g_szFreeError_SD_RV);
	RELEASE_INTERFACE(m_SolidDrawingData.m_pSurfaceRAM, g_szFreeError_SD_RS);
	RELEASE_INTERFACE(m_SolidDrawingData.m_pSurfaceGPU, g_szFreeError_SD_GS);
	RELEASE_INTERFACE(m_BackBufferData.m_pCopySurface, g_szFreeError_BCS);

	for (int i = 0; i < SC_BUFFER_COUNT_TRIPLE; i++)
	{
		if (m_BackBufferData.m_pSurfaces[i] != nullptr)
			m_BackBufferData.m_pSurfaces[i]->Release();
	}

	if (m_pSwapChain != nullptr)
		m_pSwapChain->SetFullscreenState(FALSE, nullptr);

	RELEASE_INTERFACE(m_pSwapChain, g_szFreeError_SCH);
	RELEASE_INTERFACE(m_pDeviceContext, g_szFreeError_DCO);

#if defined( DEBUG ) || defined( _DEBUG )
	/*if (m_pDevice != nullptr)
	{
		ID3D11Debug* pD3DDebug = nullptr;
		m_pDevice->QueryInterface(__uuidof(ID3D11Debug), (void**)&pD3DDebug);
		pD3DDebug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL); // D3D11_RLDO_IGNORE_INTERNAL
		pD3DDebug->Release();
	}*/
#endif

	RELEASE_INTERFACE(m_pDevice, g_szFreeError_DEV);

	ResetDeviceVars();
}

void CD3D_Device::Reset()
{
	ResetDeviceVars();
}

void CD3D_Device::FreeAll()
{
	FreeDevice();
	Reset();
}

bool CD3D_Device::Start3D()
{
	if (g_D3DDevice.m_bIn3D /*|| g_D3DDevice.GetDevice() == nullptr*/)
		return false;

	g_D3DDevice.m_bIn3D = true;

	return true;
}

bool CD3D_Device::End3D()
{
	if (!g_D3DDevice.m_bIn3D /*|| g_D3DDevice.GetDevice() == nullptr*/)
		return false;

	g_D3DDevice.m_bIn3D = false;

	return true;
}

bool CD3D_Device::IsIn3D()
{
	return g_D3DDevice.m_bIn3D;
}

void CD3D_Device::SetDefaultRenderStates()
{
	g_RenderStateMgr.SetStencilState(STENCIL_STATE_Default);
	g_RenderStateMgr.SetBlendState(BLEND_STATE_Default);
	//g_RenderStateMgr.SetRasterState(g_CV_Wireframe.m_Val ? RASTER_STATE_Wireframe : RASTER_STATE_Default);
	g_RenderStateMgr.SetRasterState(g_CV_Wireframe.m_Val ? RASTER_STATE_CullbackWireframe : RASTER_STATE_Cullback);

	if (g_CV_Anisotropic.m_Val)
		g_RenderStateMgr.SetSamplerStates(SAMPLER_STATE_Anisotropic);
	else if (g_CV_Bilinear.m_Val)
		g_RenderStateMgr.SetSamplerStates(SAMPLER_STATE_Linear);
	else
		g_RenderStateMgr.SetSamplerStates(SAMPLER_STATE_Point);
}

void CD3D_Device::SetupFullViewport(float fMinZ, float fMaxZ)
{
	SetupViewport(0, m_pModeInfo->m_dwWidth, m_pModeInfo->m_dwHeight, 0, fMinZ, fMaxZ);
}

bool CD3D_Device::SetMode(D3DModeInfo* pMode)
{
	DXGI_MODE_DESC desc;

	desc.Width = pMode->m_dwWidth;
	desc.Height = pMode->m_dwHeight;
	desc.RefreshRate.Numerator = pMode->m_dwNumerator;
	desc.RefreshRate.Denominator = pMode->m_dwDenominator;
	desc.ScanlineOrdering = (DXGI_MODE_SCANLINE_ORDER)pMode->m_dwScanlineOrdering;
	desc.Scaling = (DXGI_MODE_SCALING)pMode->m_dwScaling;
	desc.Format = (DXGI_FORMAT)pMode->m_dwFormat;

	HRESULT hResult = m_pSwapChain->ResizeTarget(&desc);
	if (FAILED(hResult))
	{
		AddDebugMessage(0, "Can't set D3D mode! (ResizeTarget)");
		return false;
	}

	UINT dwFlags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	hResult = m_pSwapChain->ResizeBuffers(0, desc.Width, desc.Height, desc.Format, dwFlags);
	if (FAILED(hResult))
	{
		AddDebugMessage(0, "Can't set D3D mode! (ResizeBuffers)");
		return false;
	}

	m_pModeInfo = pMode;

	m_rcViewport.left = 0;
	m_rcViewport.right = 0;
	m_rcViewport.top = 0;
	m_rcViewport.bottom = 0;
	m_fViewportMinZ = 0.0f;
	m_fViewportMaxZ = 0.0f;

	m_pSwapChain->SetFullscreenState(!g_bRunWindowed, nullptr);

	return true;
}

void CD3D_Device::SetSwapChainParams(DXGI_SWAP_CHAIN_DESC* pDesc, D3DModeInfo* pModeInfo)
{
	pDesc->BufferCount = g_CV_TripleBuffer.m_Val ? SC_BUFFER_COUNT_TRIPLE : SC_BUFFER_COUNT_DEFAULT;
	pDesc->BufferDesc.Width = pModeInfo->m_dwWidth;
	pDesc->BufferDesc.Height = pModeInfo->m_dwHeight;
	pDesc->BufferDesc.Format = (DXGI_FORMAT)pModeInfo->m_dwSCFormat;

	if (g_CV_VSync.m_Val)
	{
		pDesc->BufferDesc.RefreshRate.Numerator = pModeInfo->m_dwNumerator;
		pDesc->BufferDesc.RefreshRate.Denominator = pModeInfo->m_dwDenominator;
	}
	else
	{
		pDesc->BufferDesc.RefreshRate.Numerator = 0;
		pDesc->BufferDesc.RefreshRate.Denominator = 1;
	}

	pDesc->BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

	pDesc->OutputWindow = g_hWnd;

	pDesc->SampleDesc.Count = 1;
	pDesc->SampleDesc.Quality = 0;

	pDesc->Windowed = g_bRunWindowed;

	pDesc->BufferDesc.ScanlineOrdering = (DXGI_MODE_SCANLINE_ORDER)pModeInfo->m_dwScanlineOrdering;
	pDesc->BufferDesc.Scaling = g_CV_D3D11_ModeScaling.m_Val != -1 ? (DXGI_MODE_SCALING)g_CV_D3D11_ModeScaling.m_Val : DXGI_MODE_SCALING_UNSPECIFIED;

	pDesc->SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;

	pDesc->Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
}

void CD3D_Device::SetupTempViewport(uint32 dwLeft, uint32 dwRight, uint32 dwTop, uint32 dwBottom, float fMinZ, float fMaxZ)
{
	D3D11_VIEWPORT viewport;

	viewport.Width = (float)(dwRight - dwLeft);
	viewport.Height = (float)(dwTop - dwBottom);
	viewport.MinDepth = fMinZ;
	viewport.MaxDepth = fMaxZ;
	viewport.TopLeftX = (float)dwLeft;
	viewport.TopLeftY = (float)dwBottom;

	m_pDeviceContext->RSSetViewports(1, &viewport);
}

void CD3D_Device::SetupViewport(uint32 dwLeft, uint32 dwRight, uint32 dwTop, uint32 dwBottom, float fMinZ, float fMaxZ)
{
	if (m_rcViewport.left == (LONG)dwLeft && m_rcViewport.right == (LONG)dwRight &&
		m_rcViewport.top == (LONG)dwTop && m_rcViewport.bottom == (LONG)dwBottom &&
		m_fViewportMinZ == fMinZ && m_fViewportMaxZ == fMaxZ)
	{
		return;
	}

	m_rcViewport.left = dwLeft;
	m_rcViewport.right = dwRight;
	m_rcViewport.top = dwTop;
	m_rcViewport.bottom = dwBottom;

	m_fViewportMinZ = fMinZ;
	m_fViewportMaxZ = fMaxZ;
	
	D3D11_VIEWPORT viewport;
	
	viewport.Width = (float)(dwRight - dwLeft);
	viewport.Height = (float)(dwTop - dwBottom);
	viewport.MinDepth = fMinZ;
	viewport.MaxDepth = fMaxZ;
	viewport.TopLeftX = (float)dwLeft;
	viewport.TopLeftY = (float)dwBottom;

	m_pDeviceContext->RSSetViewports(1, &viewport);
}

void CD3D_Device::RestoreViewport()
{
	D3D11_VIEWPORT viewport;

	viewport.Width = (float)(m_rcViewport.right - m_rcViewport.left);
	viewport.Height = (float)(m_rcViewport.top - m_rcViewport.bottom);
	viewport.MinDepth = m_fViewportMinZ;
	viewport.MaxDepth = m_fViewportMaxZ;
	viewport.TopLeftX = (float)m_rcViewport.left;
	viewport.TopLeftY = (float)m_rcViewport.bottom;

	m_pDeviceContext->RSSetViewports(1, &viewport);
}

bool CD3D_Device::CreateBackBufferCopy(uint32 dwWidth, uint32 dwHeight)
{
	if (m_BackBufferData.m_dwWidth != dwWidth || m_BackBufferData.m_dwHeight != dwHeight)
	{
		D3D11_TEXTURE2D_DESC desc = { };

		desc.Width = dwWidth;
		desc.Height = dwHeight;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = (DXGI_FORMAT)g_D3DDevice.GetModeInfo()->m_dwFormat;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = D3D11_USAGE_STAGING;
		desc.BindFlags = 0;
		desc.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE;
		desc.MiscFlags = 0;

		ID3D11Texture2D* pBackBufferCopy;
		HRESULT hResult = g_D3DDevice.GetDevice()->CreateTexture2D(&desc, nullptr, &pBackBufferCopy);

		if (FAILED(hResult))
			return false;

		m_BackBufferData.m_pCopySurface = pBackBufferCopy;
		m_BackBufferData.m_dwWidth = dwWidth;
		m_BackBufferData.m_dwHeight = dwHeight;
	}

	return true;
}

bool CD3D_Device::CreateSolidDrawingSurface(uint32 dwWidth, uint32 dwHeight, bool bForceRecreate, int nInitByte)
{
	ID3D11Texture2D* pSurfaceRAM = nullptr;
	ID3D11Texture2D* pSurfaceGPU = nullptr;
	ID3D11ShaderResourceView* pResourceView = nullptr;

	bool bDimsChanged = m_SolidDrawingData.m_dwWidth != dwWidth || m_SolidDrawingData.m_dwHeight != dwHeight;

	if (bDimsChanged || bForceRecreate)
	{
		if (m_SolidDrawingData.m_pSurfaceRAM != nullptr)
		{
			m_SolidDrawingData.m_pSurfaceRAM->Release();
			m_SolidDrawingData.m_pSurfaceRAM = nullptr;
		}

		D3D11_TEXTURE2D_DESC texDesc = { };

		texDesc.Width = dwWidth;
		texDesc.Height = dwHeight;
		texDesc.MipLevels = 1;
		texDesc.ArraySize = 1;
		texDesc.Format = (DXGI_FORMAT)g_D3DDevice.GetModeInfo()->m_dwFormat;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;
		texDesc.Usage = D3D11_USAGE_STAGING;
		texDesc.BindFlags = 0;
		texDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		texDesc.MiscFlags = 0;

		HRESULT hResult = g_D3DDevice.GetDevice()->CreateTexture2D(&texDesc, nullptr, &pSurfaceRAM);

		if (FAILED(hResult))
			return false;

		texDesc.Usage = D3D11_USAGE_DEFAULT;
		texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		texDesc.CPUAccessFlags = 0;

		hResult = g_D3DDevice.GetDevice()->CreateTexture2D(&texDesc, nullptr, &pSurfaceGPU);

		if (FAILED(hResult))
		{
			pSurfaceRAM->Release();
			return false;
		}

		D3D11_SHADER_RESOURCE_VIEW_DESC resourceDesc = { };
		resourceDesc.Format = texDesc.Format;

		resourceDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		resourceDesc.Texture2D.MipLevels = texDesc.MipLevels;
		
		hResult = g_D3DDevice.GetDevice()->CreateShaderResourceView(pSurfaceGPU, &resourceDesc, &pResourceView);

		if (FAILED(hResult))
		{
			pSurfaceRAM->Release();
			pSurfaceGPU->Release();
			return false;
		}

		m_SolidDrawingData.m_pSurfaceRAM = pSurfaceRAM;
		m_SolidDrawingData.m_pSurfaceGPU = pSurfaceGPU;
		m_SolidDrawingData.m_pShaderResourceView = pResourceView;
		m_SolidDrawingData.m_dwWidth = dwWidth;
		m_SolidDrawingData.m_dwHeight = dwHeight;
	}

	if (pSurfaceRAM != nullptr && nInitByte > -1)
	{
		D3D11_MAPPED_SUBRESOURCE subResource;
		HRESULT hResult = m_pDeviceContext->Map(pSurfaceRAM, 0, D3D11_MAP_WRITE, 0, &subResource);
		if (FAILED(hResult))
		{
			pSurfaceRAM->Release();
			pSurfaceGPU->Release();
			pResourceView->Release();
			return false;
		}

		memset(subResource.pData, nInitByte, subResource.DepthPitch);

		m_pDeviceContext->Unmap(pSurfaceRAM, 0);
	}

	return true;
}

void CD3D_Device::PreSwapJobs()
{
	if (g_dwScreenLocksPerFrame)
	{
		m_pDeviceContext->CopyResource(m_SolidDrawingData.m_pSurfaceGPU, m_SolidDrawingData.m_pSurfaceRAM);
		d3d_PostProcess_SolidDrawing(m_SolidDrawingData.m_pShaderResourceView);
	}

	ID3D11ShaderResourceView* pPrevTargetSRV;
	if (g_RenderTargetMgr.GetCurrentRenderTarget() == RENDER_TARGET_Main1)
		pPrevTargetSRV = g_RenderTargetMgr.GetRenderTarget(RENDER_TARGET_Main1)->GetShaderResourceView();
	else
		pPrevTargetSRV = g_RenderTargetMgr.GetRenderTarget(RENDER_TARGET_Main2)->GetShaderResourceView();

	//if (g_RenderTargetMgr.GetCurrentRenderTarget() == RENDER_TARGET_Default)
	//	Sleep(0);
	//ID3D11Resource* pResource;
	//pPrevTargetSRV->GetResource(&pResource);
	//d3d_SaveTextureAsTGA("zzz.tga", (ID3D11Texture2D*)pResource, 0, true);

	g_RenderTargetMgr.RT_InstallOnDevice(RENDER_TARGET_Default);
	d3d_PostProcess_ToneMap(pPrevTargetSRV);

	g_RenderShaderMgr.ClearShaderResourcesPS(SRS_PS_Primary, 1);
}

void CD3D_Device::PostSwapJobs()
{
	g_dwScreenLocksPerFrame = 0;

	g_RenderTargetMgr.RT_InstallOnDevice(RENDER_TARGET_Main1);

	g_RenderShaderMgr.ResetCurrentData();
}
