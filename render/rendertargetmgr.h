#ifndef _RENDERTARGETMGR_H_
#define _RENDERTARGETMGR_H_

#ifndef __D3D11_H__
#include <d3d11_1.h>
#define __D3D11_H__
#endif

#include <vector>

enum RenderTarget
{
	RENDER_TARGET_Invalid = -1,
	RENDER_TARGET_Default = 0,
	RENDER_TARGET_Main1,
	RENDER_TARGET_Main2,
	RENDER_TARGET_Bloom1,
	RENDER_TARGET_Bloom2,
	RENDER_TARGET_MAX
};

class CRenderTarget;
typedef std::vector<CRenderTarget*> Array_CRenderTarget;

class CRenderTargetMgr
{

public:

	CRenderTargetMgr()
	{
		m_eCurrentRenderTarget = RENDER_TARGET_Invalid;
	}

	~CRenderTargetMgr();

	bool	Init();
	void	FreeAll();

	bool			CreateRenderTarget(uint32 dwWidth, uint32 dwHeight, int nDS_Format, uint32 dwFlags, 
		ID3D11DepthStencilView* pExtraDSV);
	bool			RecreateRenderTarget(RenderTarget eRenderTarget, uint32 dwWidth, uint32 dwHeight, int nDS_Format,
		uint32 dwFlags, ID3D11DepthStencilView* pExtraDSV);

	RenderTarget	GetCurrentRenderTarget() { return m_eCurrentRenderTarget; }
	CRenderTarget*	GetCurrentRenderTargetObj() { return m_aRenderTarget[m_eCurrentRenderTarget]; }
	CRenderTarget*	GetRenderTarget(RenderTarget eRenderTarget) { return m_aRenderTarget[eRenderTarget]; };

	void	RT_InstallOnDevice(RenderTarget* aeRenderTarget, uint32 dwCount);
	void	RT_InstallOnDevice(RenderTarget eRenderTarget);
	void	RT_RemoveFromDevice();

protected:

	RenderTarget	m_eCurrentRenderTarget;

	Array_CRenderTarget	m_aRenderTarget;
};

extern CRenderTargetMgr g_RenderTargetMgr;

#endif