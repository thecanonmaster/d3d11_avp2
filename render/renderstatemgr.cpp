#include "pch.h"

#include "renderstatemgr.h"
#include "d3d_device.h"
#include "common_stuff.h"
#include "rendererconsolevars.h"
#include "rendershadermgr.h"

CRenderStateMgr g_RenderStateMgr;

CRenderStateMgr::~CRenderStateMgr()
{
	FreeAll();
}

bool CRenderStateMgr::Init()
{
	LTBOOL bRet = TRUE;
	
	bRet &= CreateStencilState(CSS_DEPTH_ENABLED | CSS_DEPTH_WRITE_ENABLED); // Default
	bRet &= CreateStencilState(CSS_DEPTH_WRITE_ENABLED); // NoZ
	bRet &= CreateStencilState(CSS_DEPTH_ENABLED); // NoZWrite

	bRet &= CreateBlendState(false, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA); // Default
	bRet &= CreateBlendState(true, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA); // Alpha
	bRet &= CreateBlendState(true, D3D11_BLEND_ONE, D3D11_BLEND_ZERO); // Solid
	bRet &= CreateBlendState(true, D3D11_BLEND_ONE, D3D11_BLEND_ONE); // Add
	bRet &= CreateBlendState(true, D3D11_BLEND_DEST_COLOR, D3D11_BLEND_ZERO); // Multiply
	bRet &= CreateBlendState(true, D3D11_BLEND_DEST_COLOR, D3D11_BLEND_SRC_COLOR); // Multiply2
	bRet &= CreateBlendState(true, D3D11_BLEND_ZERO, D3D11_BLEND_INV_SRC_COLOR); // Mask
	bRet &= CreateBlendState(true, D3D11_BLEND_ONE, D3D11_BLEND_INV_SRC_COLOR); // MaskAdd
	bRet &= CreateBlendState(true, D3D11_BLEND_ZERO, D3D11_BLEND_SRC_COLOR); // Multiply3D
	bRet &= CreateBlendState(true, D3D11_BLEND_INV_SRC_COLOR, D3D11_BLEND_INV_SRC_COLOR); // Invert

	bRet &= CreateRasterState(CRS_NO_CULL); // Default
	bRet &= CreateRasterState(CRS_WIREFRAME | CRS_NO_CULL); // Wireframe
	bRet &= CreateRasterState(0); // Cullback
	bRet &= CreateRasterState(CRS_WIREFRAME); // Cullback + Wireframe
	bRet &= CreateRasterState(CRS_CULL_CCW); // CCW Cullback
	bRet &= CreateRasterState(CRS_NO_DEPTH_CLIP); // Cullback + No Depth Clip

	bRet &= CreateSamplerStates();

	return bRet;
}

LTBOOL CRenderStateMgr::CreateSamplerStates()
{
	LTBOOL bRet = TRUE;

	bRet &= CreateSamplerState(D3D11_FILTER_MIN_MAG_MIP_POINT, 0, m_fCurMipMapBias, D3D11_TEXTURE_ADDRESS_WRAP);
	bRet &= CreateSamplerState(D3D11_FILTER_MIN_MAG_MIP_LINEAR, 0, m_fCurMipMapBias, D3D11_TEXTURE_ADDRESS_WRAP);
	bRet &= CreateSamplerState(D3D11_FILTER_ANISOTROPIC, m_nCurMaxAnisotropy, m_fCurMipMapBias, 
		D3D11_TEXTURE_ADDRESS_WRAP);
	bRet &= CreateSamplerState(D3D11_FILTER_MIN_MAG_MIP_POINT, 0, m_fCurMipMapBias, D3D11_TEXTURE_ADDRESS_CLAMP);
	bRet &= CreateSamplerState(D3D11_FILTER_MIN_MAG_MIP_LINEAR, 0, m_fCurMipMapBias, D3D11_TEXTURE_ADDRESS_CLAMP);
	bRet &= CreateSamplerState(D3D11_FILTER_ANISOTROPIC, m_nCurMaxAnisotropy, m_fCurMipMapBias, 
		D3D11_TEXTURE_ADDRESS_CLAMP);

	return bRet;
}

void CRenderStateMgr::RecreateSamplerStates()
{
	if (m_nCurMaxAnisotropy == g_CV_Anisotropic.m_Val && m_fCurMipMapBias == g_CV_MipMapBias.m_Val)
		return;

	m_nCurMaxAnisotropy = g_CV_Anisotropic.m_Val;
	m_fCurMipMapBias = g_CV_MipMapBias.m_Val;

	FreeSamplerStates();
	CreateSamplerStates();
}

void CRenderStateMgr::FreeAll()
{
	for (const auto pState : m_StencilStates)
	{
		uint32 dwRefCount = pState->Release();
		if (dwRefCount)
			AddDebugMessage(0, g_szFreeError_ST, dwRefCount);
	}

	m_StencilStates.clear();

	for (const auto pState : m_BlendStates)
	{
		uint32 dwRefCount = pState->Release();
		if (dwRefCount)
			AddDebugMessage(0, g_szFreeError_BL, dwRefCount);
	}

	m_BlendStates.clear();

	for (const auto pState : m_RasterStates)
	{
		uint32 dwRefCount = pState->Release();
		if (dwRefCount)
			AddDebugMessage(0, g_szFreeError_RA, dwRefCount);
	}

	m_RasterStates.clear();

	FreeSamplerStates();
}

void CRenderStateMgr::FreeSamplerStates()
{
	for (const auto pState : m_SamplerStates)
	{
		uint32 dwRefCount = pState->Release();
		if (dwRefCount)
			AddDebugMessage(0, g_szFreeError_SA, dwRefCount);
	}

	m_SamplerStates.clear();
}

LTBOOL CRenderStateMgr::CreateStencilState(uint32 dwFlags)
{
	D3D11_DEPTH_STENCIL_DESC desc = { };

	desc.DepthEnable = (dwFlags & CSS_DEPTH_ENABLED);
	desc.DepthWriteMask = (dwFlags & CSS_DEPTH_WRITE_ENABLED) ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
	desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

	desc.StencilEnable = TRUE;
	desc.StencilReadMask = 0xFF;
	desc.StencilWriteMask = 0xFF;

	desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	desc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	ID3D11DepthStencilState* pState;
	HRESULT hResult = g_D3DDevice.GetDevice()->CreateDepthStencilState(&desc, &pState);
	if (FAILED(hResult))
		return FALSE;

	m_StencilStates.push_back(pState);

	return TRUE;
}

LTBOOL CRenderStateMgr::CreateBlendState(bool bBlendEnabled, uint32 dwSrcBlend, uint32 dwDestBlend)
{
	D3D11_BLEND_DESC1 desc = { };

	desc.RenderTarget[0].BlendEnable = bBlendEnabled;
	desc.RenderTarget[0].SrcBlend = (D3D11_BLEND)dwSrcBlend;
	desc.RenderTarget[0].DestBlend = (D3D11_BLEND)dwDestBlend;
	desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	ID3D11BlendState1* pState;
	HRESULT hResult = g_D3DDevice.GetDevice()->CreateBlendState1(&desc, &pState);
	if (FAILED(hResult))
		return FALSE;

	m_BlendStates.push_back(pState);

	return TRUE;
}

LTBOOL CRenderStateMgr::CreateRasterState(uint32 dwFlags)
{
	D3D11_RASTERIZER_DESC1 desc = { };

	desc.FillMode = !(dwFlags & CRS_WIREFRAME) ? D3D11_FILL_SOLID : D3D11_FILL_WIREFRAME;
	desc.CullMode = (dwFlags & CRS_NO_CULL) ? D3D11_CULL_NONE : D3D11_CULL_BACK;

	desc.FrontCounterClockwise = (dwFlags & CRS_CULL_CCW);

	desc.DepthBias = 0;
	desc.DepthBiasClamp = 0.0f;
	desc.SlopeScaledDepthBias = 0.0f;
	desc.DepthClipEnable = !(dwFlags & CRS_NO_DEPTH_CLIP);

	desc.ScissorEnable = FALSE;
	desc.AntialiasedLineEnable = FALSE;
	desc.MultisampleEnable = FALSE;
	desc.ForcedSampleCount = 0;

	ID3D11RasterizerState1* pState;
	HRESULT hResult = g_D3DDevice.GetDevice()->CreateRasterizerState1(&desc, &pState);
	if (FAILED(hResult))
		return FALSE;

	m_RasterStates.push_back(pState);

	return TRUE;
}

LTBOOL CRenderStateMgr::CreateSamplerState(D3D11_FILTER eFilter, int nMaxAnisotropy, float fMipMapBias, D3D11_TEXTURE_ADDRESS_MODE eAddressMode)
{
	D3D11_SAMPLER_DESC desc = { };

	desc.Filter = eFilter;
	desc.AddressU = eAddressMode;
	desc.AddressV = eAddressMode;
	desc.AddressW = eAddressMode;
	desc.MipLODBias = fMipMapBias;
	desc.MaxAnisotropy = nMaxAnisotropy;
	desc.MinLOD = 0;
	desc.MaxLOD = MAX_DTX_MIPMAPS - 1;

	ID3D11SamplerState* pState;
	HRESULT hResult = g_D3DDevice.GetDevice()->CreateSamplerState(&desc, &pState);
	if (FAILED(hResult))
		return FALSE;

	m_SamplerStates.push_back(pState);

	return TRUE;
}

void CRenderStateMgr::SaveAllStates()
{
	SaveStencilState();
	SaveBlendState();
	SaveRasterState();
	SaveSamplerStates();
}

void CRenderStateMgr::RestoreAllStates()
{
	RestoreStencilState();
	RestoreBlendState();
	RestoreRasterState();
	RestoreSamplerStates();
}

void CRenderStateMgr::SavePrimaryStates()
{
	SaveStencilState();
	SaveBlendState();
	SaveRasterState();
}

void CRenderStateMgr::RestorePrimaryStates()
{
	RestoreStencilState();
	RestoreBlendState();
	RestoreRasterState();
}

void CRenderStateMgr::SetStencilState(StencilState eState)
{
	if (m_eStencilState == eState)
		return;

	m_eStencilState = eState;
	g_D3DDevice.GetDeviceContext()->OMSetDepthStencilState(m_StencilStates[eState], 1);
}

void CRenderStateMgr::SetBlendState(BlendState eState)
{
	if (m_eBlendState == eState)
		return;
	
	float afBlendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	m_eBlendState = eState;
	g_D3DDevice.GetDeviceContext()->OMSetBlendState(m_BlendStates[eState], afBlendFactor, 0xFFFFFFFF);
}

void CRenderStateMgr::SetRasterState(RasterState eState)
{
	if (m_eRasterState == eState)
		return;

	m_eRasterState = eState;
	g_D3DDevice.GetDeviceContext()->RSSetState(m_RasterStates[eState]);
}

void CRenderStateMgr::SetSamplerStates(SamplerState eState1)
{
	RecreateSamplerStates();
	
	SamplerState eState2 = (SamplerState)((int)eState1 + MAX_SAMPLERS + 1);
	if (m_aeSamplerState[0] == eState1 && m_aeSamplerState[1] == eState2)
		return;

	m_aeSamplerState[0] = eState1;
	m_aeSamplerState[1] = eState2;

	ID3D11SamplerState* apSamplers[] = { m_SamplerStates[eState1], m_SamplerStates[eState2] };
	g_D3DDevice.GetDeviceContext()->PSSetSamplers(0, 2, apSamplers);
}

void CRenderStateMgr::SaveSamplerStates()
{
	m_aeSavedSamplerState[0] = m_aeSamplerState[0];
	m_aeSavedSamplerState[1] = m_aeSamplerState[1];
}

void CRenderStateMgr::RestoreSamplerStates()
{
	SetSamplerStates(m_aeSavedSamplerState[0]);
}
