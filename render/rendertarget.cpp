#include "pch.h"

#include "rendertarget.h"
#include "d3d_device.h"
#include "common_stuff.h"
#include "d3d_surface.h"

CRenderTarget::CRenderTarget()
{
	m_pRenderTexture = nullptr;
	m_pRenderTargetView = nullptr;
	m_pDepthStencilView = nullptr;
	m_pShaderResourceView = nullptr;

	m_sInitParams.m_dwWidth = 0;
	m_sInitParams.m_dwHeight = 0;
	m_sInitParams.m_nDS_Format = 0;
}

CRenderTarget::~CRenderTarget()
{
	Term();
}

bool CRenderTarget::Init(uint32 dwWidth, uint32 dwHeight, int nDS_Format, uint32 dwFlags, ID3D11DepthStencilView* pExtraDSV)
{
	m_sInitParams.m_dwWidth = dwWidth;
	m_sInitParams.m_dwHeight = dwHeight;
	m_sInitParams.m_nDS_Format = nDS_Format;
	m_sInitParams.m_dwFlags = dwFlags;
	m_sInitParams.pExtraDSV = pExtraDSV;

	return Recreate();
}

void CRenderTarget::Term()
{
	RELEASE_INTERFACE(m_pRenderTargetView, g_szFreeError_RT_RTV);
	RELEASE_INTERFACE(m_pDepthStencilView, g_szFreeError_RT_DSV);
	RELEASE_INTERFACE(m_pRenderTexture, g_szFreeError_RT_TEX);
	RELEASE_INTERFACE(m_pShaderResourceView, g_szFreeError_RT_SRV);
}

void CRenderTarget::SetRenderTextureParams(D3D11_TEXTURE2D_DESC* pDesc)
{
	pDesc->Width = m_sInitParams.m_dwWidth;
	pDesc->Height = m_sInitParams.m_dwHeight;
	pDesc->MipLevels = 1;
	pDesc->ArraySize = 1;
	pDesc->Format = (DXGI_FORMAT)g_D3DDevice.GetModeInfo()->m_dwFormat;
	pDesc->SampleDesc.Count = 1;
	pDesc->Usage = D3D11_USAGE_DEFAULT;
	pDesc->BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	pDesc->CPUAccessFlags = 0;
	pDesc->MiscFlags = 0;
}

void CRenderTarget::SetDepthBufferParams(D3D11_TEXTURE2D_DESC* pDesc)
{
	pDesc->Width = m_sInitParams.m_dwWidth;
	pDesc->Height = m_sInitParams.m_dwHeight;
	pDesc->MipLevels = 1;
	pDesc->ArraySize = 1;
	pDesc->Format = (DXGI_FORMAT)m_sInitParams.m_nDS_Format;
	pDesc->SampleDesc.Count = 1;
	pDesc->SampleDesc.Quality = 0;
	pDesc->Usage = D3D11_USAGE_DEFAULT;
	pDesc->BindFlags = D3D11_BIND_DEPTH_STENCIL;
	pDesc->CPUAccessFlags = 0;
	pDesc->MiscFlags = 0;
}

void CRenderTarget::SetDepthStencilViewParams(D3D11_DEPTH_STENCIL_VIEW_DESC* pDesc)
{
	pDesc->Format = (DXGI_FORMAT)m_sInitParams.m_nDS_Format;
	pDesc->ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	pDesc->Flags = 0;
	pDesc->Texture2D.MipSlice = 0;
}

void CRenderTarget::SetShaderResourceViewParams(D3D11_SHADER_RESOURCE_VIEW_DESC* pDesc)
{
	pDesc->Format = (DXGI_FORMAT)g_D3DDevice.GetModeInfo()->m_dwFormat;
	pDesc->ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	pDesc->Texture2D.MostDetailedMip = 0;
	pDesc->Texture2D.MipLevels = 1;
}

bool CRenderTarget::Recreate()
{
	Term();
	HRESULT hResult = 0;

	if (m_sInitParams.m_dwFlags & RT_FLAG_DEPTH_STENCIL)
	{
		D3D11_TEXTURE2D_DESC depthBufferDesc = { };
		SetDepthBufferParams(&depthBufferDesc);

		ID3D11Texture2D* pDepthStencilBuffer;
		hResult = g_D3DDevice.GetDevice()->CreateTexture2D(&depthBufferDesc, nullptr, &pDepthStencilBuffer);
		if (FAILED(hResult))
			return false;

		D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = { };
		SetDepthStencilViewParams(&depthStencilViewDesc);

		hResult = g_D3DDevice.GetDevice()->CreateDepthStencilView(pDepthStencilBuffer, &depthStencilViewDesc, &m_pDepthStencilView);
		if (FAILED(hResult))
		{
			pDepthStencilBuffer->Release();
			return false;
		}

		pDepthStencilBuffer->Release();
	}

	if (m_sInitParams.m_dwFlags & RT_FLAG_SHADER_RESOURCE)
	{
		D3D11_TEXTURE2D_DESC textureDesc = { };
		SetRenderTextureParams(&textureDesc);

		hResult = g_D3DDevice.GetDevice()->CreateTexture2D(&textureDesc, nullptr, &m_pRenderTexture);
		if (FAILED(hResult))
			return false;

		D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceDesc = { };
		SetShaderResourceViewParams(&shaderResourceDesc);

		hResult = g_D3DDevice.GetDevice()->CreateShaderResourceView(m_pRenderTexture, &shaderResourceDesc, &m_pShaderResourceView);
		if (FAILED(hResult))
			return false;
	}

	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = { };
	rtvDesc.Format = (DXGI_FORMAT)g_D3DDevice.GetModeInfo()->m_dwFormat;
	rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

	ID3D11Resource* pResource = (m_sInitParams.m_dwFlags & RT_FLAG_SHADER_RESOURCE) ? m_pRenderTexture : 
		g_D3DDevice.GetBackBufferData()->m_pSurfaces[0];

	hResult = g_D3DDevice.GetDevice()->CreateRenderTargetView(pResource, &rtvDesc, &m_pRenderTargetView);
	if (FAILED(hResult))
		return false;

	return true;
}

void CRenderTarget::InstallOnDevice()
{
	g_D3DDevice.GetDeviceContext()->OMSetRenderTargets(1, &m_pRenderTargetView, GetExtraDSV());
}

void CRenderTarget::ClearRenderTargetView(float* pColor)
{
	g_D3DDevice.GetDeviceContext()->ClearRenderTargetView(m_pRenderTargetView, pColor);
}

void CRenderTarget::ClearDepthStencilView(uint32 dwFlags, float fDepth, uint8 nStencil)
{
	g_D3DDevice.GetDeviceContext()->ClearDepthStencilView(GetExtraDSV(), dwFlags, fDepth, nStencil);
}