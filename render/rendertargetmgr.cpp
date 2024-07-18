#include "pch.h"

#include "rendertargetmgr.h"
#include "rendertarget.h"
#include "common_init.h"
#include "d3d_device.h"

CRenderTargetMgr g_RenderTargetMgr;

CRenderTargetMgr::~CRenderTargetMgr()
{
	FreeAll();
}

bool CRenderTargetMgr::Init()
{
	bool bRet = true;

	bRet &= CreateRenderTarget(g_D3DDevice.GetModeInfo()->m_dwWidth, g_D3DDevice.GetModeInfo()->m_dwHeight,
		DXGI_FORMAT_D24_UNORM_S8_UINT, RT_FLAG_DEPTH_STENCIL, nullptr); // Default

	ID3D11DepthStencilView* pExtraDSV = m_aRenderTarget[RENDER_TARGET_Default]->GetDepthStencilView();
	bRet &= CreateRenderTarget(g_D3DDevice.GetModeInfo()->m_dwWidth, g_D3DDevice.GetModeInfo()->m_dwHeight,
		DXGI_FORMAT_D24_UNORM_S8_UINT, RT_FLAG_SHADER_RESOURCE | RT_FLAG_EXTRA_DEPTH_STENCIL, pExtraDSV); // Main1

	bRet &= CreateRenderTarget(g_D3DDevice.GetModeInfo()->m_dwWidth, g_D3DDevice.GetModeInfo()->m_dwHeight, 
		DXGI_FORMAT_D24_UNORM_S8_UINT, RT_FLAG_SHADER_RESOURCE | RT_FLAG_INSTALL, nullptr); // Main2

	bRet &= CreateRenderTarget(g_D3DDevice.GetModeInfo()->m_dwWidth, g_D3DDevice.GetModeInfo()->m_dwHeight,
		DXGI_FORMAT_D24_UNORM_S8_UINT, RT_FLAG_SHADER_RESOURCE, nullptr); // Bloom1

	bRet &= CreateRenderTarget(g_D3DDevice.GetModeInfo()->m_dwWidth, g_D3DDevice.GetModeInfo()->m_dwHeight,
		DXGI_FORMAT_D24_UNORM_S8_UINT, RT_FLAG_SHADER_RESOURCE, nullptr); // Bloom2

	return bRet;
}

void CRenderTargetMgr::FreeAll()
{
	for (CRenderTarget* pTarget : m_aRenderTarget)
	{
		pTarget->Term();
		delete pTarget;
	}

	m_aRenderTarget.clear();
}

void CRenderTargetMgr::RT_InstallOnDevice(RenderTarget* aeRenderTarget, uint32 dwCount)
{
	ID3D11DepthStencilView* pDSV = m_aRenderTarget[aeRenderTarget[0]]->GetExtraDSV();
	ID3D11RenderTargetView* apRTV[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT];

	for (uint32 i = 0; i < dwCount; i++)
		apRTV[i] = m_aRenderTarget[aeRenderTarget[i]]->GetRenderTargetView();

	g_D3DDevice.GetDeviceContext()->OMSetRenderTargets(dwCount, apRTV, pDSV);
}

void CRenderTargetMgr::RT_InstallOnDevice(RenderTarget eRenderTarget)
{
	m_eCurrentRenderTarget = eRenderTarget;
	m_aRenderTarget[eRenderTarget]->InstallOnDevice();
}

void CRenderTargetMgr::RT_RemoveFromDevice()
{
	m_eCurrentRenderTarget = RENDER_TARGET_Invalid;

	ID3D11RenderTargetView* pNullRTV = nullptr;
	g_D3DDevice.GetDeviceContext()->OMSetRenderTargets(1, &pNullRTV, nullptr);
}

bool CRenderTargetMgr::CreateRenderTarget(uint32 dwWidth, uint32 dwHeight, int nDS_Format, uint32 dwFlags,
	ID3D11DepthStencilView* pExtraDSV)
{
	CRenderTarget* pTarget = new CRenderTarget();
	if (pTarget->Init(dwWidth, dwHeight, nDS_Format, dwFlags, pExtraDSV))
	{
		m_aRenderTarget.push_back(pTarget);

		if (dwFlags & RT_FLAG_INSTALL)
		{
			m_eCurrentRenderTarget = (RenderTarget)(m_aRenderTarget.size() - 1);
			pTarget->InstallOnDevice();
		}

		return true;
	}

	return false;
}

bool CRenderTargetMgr::RecreateRenderTarget(RenderTarget eRenderTarget, uint32 dwWidth, uint32 dwHeight, int nDS_Format,
	uint32 dwFlags, ID3D11DepthStencilView* pExtraDSV)
{
	CRenderTarget* pTarget = m_aRenderTarget[eRenderTarget];
	if (pTarget->Init(dwWidth, dwHeight, nDS_Format, dwFlags, pExtraDSV))
	{
		if (dwFlags & RT_FLAG_INSTALL)
		{
			m_eCurrentRenderTarget = (RenderTarget)(m_aRenderTarget.size() - 1);
			pTarget->InstallOnDevice();
		}

		return true;
	}
	
	return false;
}
