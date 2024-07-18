#ifndef _RENDERTARGET_H_
#define _RENDERTARGET_H_

#ifndef __D3D11_H__
#include <d3d11_1.h>
#define __D3D11_H__
#endif

#define RT_FLAG_INSTALL				(1<<0)
#define RT_FLAG_DEPTH_STENCIL		(1<<1)
#define RT_FLAG_SHADER_RESOURCE		(1<<2)
#define RT_FLAG_EXTRA_DEPTH_STENCIL	(1<<3)

static const char* g_szFreeError_RT_RTV = "Failed to free D3D render target view (count = %d)";
static const char* g_szFreeError_RT_DSV = "Failed to free D3D depth stencil view (count = %d)";
static const char* g_szFreeError_RT_TEX = "Failed to free D3D render texture (count = %d)";
static const char* g_szFreeError_RT_SRV = "Failed to free D3D shader resource view (count = % d)";

struct RenderTargetParams
{
	uint32					m_dwWidth;
	uint32					m_dwHeight;
	int						m_nDS_Format;
	uint32					m_dwFlags;
	ID3D11DepthStencilView*	pExtraDSV;
};

class CRenderTarget
{

public:

	CRenderTarget();
	~CRenderTarget();

	bool	Init(uint32 dwWidth, uint32 dwHeight, int nDS_Format, uint32 dwFlags, ID3D11DepthStencilView* pExtraDSV);
	void	Term();
	bool	Recreate();

	void	SetRenderTextureParams(D3D11_TEXTURE2D_DESC* pDesc);
	void	SetDepthBufferParams(D3D11_TEXTURE2D_DESC* pDesc);
	void	SetDepthStencilViewParams(D3D11_DEPTH_STENCIL_VIEW_DESC* pDesc);
	void	SetShaderResourceViewParams(D3D11_SHADER_RESOURCE_VIEW_DESC* pDesc);

	ID3D11DepthStencilView* GetExtraDSV() 
	{ 
		return (m_sInitParams.m_dwFlags & RT_FLAG_EXTRA_DEPTH_STENCIL) ? 
			m_sInitParams.pExtraDSV : m_pDepthStencilView; 
	}

	void	InstallOnDevice();

	ID3D11RenderTargetView*		GetRenderTargetView() { return m_pRenderTargetView;  }
	ID3D11DepthStencilView*		GetDepthStencilView() { return m_pDepthStencilView;  }
	ID3D11Texture2D*			GetRenderTexture() { return m_pRenderTexture; }
	ID3D11ShaderResourceView*	GetShaderResourceView() { return m_pShaderResourceView; }

	RenderTargetParams*	GetInitParams() { return &m_sInitParams; }

	void	ClearRenderTargetView(float* pColor);
	void	ClearDepthStencilView(uint32 dwFlags, float fDepth, uint8 nStencil);

private:

	RenderTargetParams	m_sInitParams;

	ID3D11RenderTargetView*		m_pRenderTargetView;
	ID3D11DepthStencilView*		m_pDepthStencilView;
	ID3D11Texture2D*			m_pRenderTexture;
	ID3D11ShaderResourceView*	m_pShaderResourceView;
};

#endif