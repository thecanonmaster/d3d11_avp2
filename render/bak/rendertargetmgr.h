#ifndef _RENDERTARGETMGR_H_
#define _RENDERTARGETMGR_H_

#include <vector>

#define RENDER_TARGET_DEFAULT	0

#define INVALID_RENDER_TARGET	UINT_MAX

typedef uint32 HRENDERTARGET;

class CRenderTarget;
typedef std::vector<CRenderTarget*> Array_RenderTargets;

class CRenderTargetMgr
{

public:

	~CRenderTargetMgr();

	void	Term();

	LTBOOL			AddRenderTarget(uint32 dwWidth, uint32 dwHeight, int nDS_Format, HRENDERTARGET hRenderTarget, LTBOOL bInstall);
	CRenderTarget*	GetRenderTarget(HRENDERTARGET hRenderTarget);
	CRenderTarget*	GetDefaultRenderTarget() { return m_RenderTargets[RENDER_TARGET_DEFAULT]; }
	LTBOOL			RemoveRenderTarget(HRENDERTARGET hRenderTarget);

	void	FreeDeviceObjects();

	void	RecreateRenderTargets();

protected:

	Array_RenderTargets	m_RenderTargets;
};

extern CRenderTargetMgr g_RenderTargetMgr;

#endif