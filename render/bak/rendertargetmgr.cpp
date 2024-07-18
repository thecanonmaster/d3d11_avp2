#include "pch.h"

#include "rendertargetmgr.h"
#include "rendertarget.h"
#include "common_init.h"

CRenderTargetMgr g_RenderTargetMgr;

CRenderTargetMgr::~CRenderTargetMgr()
{
	Term();
}

void CRenderTargetMgr::Term()
{
	for (const auto pTarget : m_RenderTargets)
		delete pTarget;

	m_RenderTargets.clear();
}

LTBOOL CRenderTargetMgr::AddRenderTarget(uint32 dwWidth, uint32 dwHeight, int nDS_Format, HRENDERTARGET hRenderTarget, LTBOOL bInstall)
{
	if (hRenderTarget >= m_RenderTargets.size())
		m_RenderTargets.insert(m_RenderTargets.end(), hRenderTarget - m_RenderTargets.size() + 1, nullptr);

	if (m_RenderTargets[hRenderTarget] != nullptr)
	{
		g_pStruct->ConsolePrint((char*)"RenderTarget %d already exists", hRenderTarget);
		return FALSE;
	}

	CRenderTarget* pRenderTarget = new CRenderTarget();
	if (pRenderTarget)
	{
		if (pRenderTarget->Init(dwWidth, dwHeight, nDS_Format))
		{
			m_RenderTargets[hRenderTarget] = pRenderTarget;

			if (bInstall)
				pRenderTarget->InstallOnDevice();

			return TRUE;
		}
		else
		{
			g_pStruct->ConsolePrint((char*)"Failed to create RenderTarget %d", hRenderTarget);
		}
	}

	return FALSE;
}

CRenderTarget* CRenderTargetMgr::GetRenderTarget(HRENDERTARGET hRenderTarget)
{
	if (hRenderTarget < m_RenderTargets.size())
		return m_RenderTargets[hRenderTarget];
	
	return nullptr;
}

LTBOOL CRenderTargetMgr::RemoveRenderTarget(HRENDERTARGET hRenderTarget)
{
	if (hRenderTarget < m_RenderTargets.size())
	{
		CRenderTarget* pTarget = m_RenderTargets[hRenderTarget];
		if (pTarget != nullptr)
		{
			delete pTarget;
			m_RenderTargets[hRenderTarget] = nullptr;

			return TRUE;
		}
		else
		{
			return FALSE;
		}
	}

	return FALSE;
}

void CRenderTargetMgr::FreeDeviceObjects()
{
	for (const auto pTarget : m_RenderTargets)
		pTarget->Term();
}

void CRenderTargetMgr::RecreateRenderTargets()
{
	for (const auto pTarget : m_RenderTargets)
		pTarget->Recreate();
}