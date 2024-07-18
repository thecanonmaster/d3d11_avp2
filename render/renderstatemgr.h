#ifndef __RENDERSTATE_MGR_H__
#define __RENDERSTATE_MGR_H__

#ifndef __D3D11_H__
#include <d3d11_1.h>
#define __D3D11_H__
#endif

#define MAX_SAMPLERS	2

static const char* g_szFreeError_ST = "Failed to free D3D stencil state (count = %d)";
static const char* g_szFreeError_BL = "Failed to free D3D blend state (count = %d)";
static const char* g_szFreeError_RA = "Failed to free D3D raster state (count = %d)";
static const char* g_szFreeError_SA = "Failed to free D3D sampler state (count = %d)";

#define CSS_DEPTH_ENABLED		(1<<0)
#define CSS_DEPTH_WRITE_ENABLED	(1<<1)

enum StencilState
{
	STENCIL_STATE_Invalid = -1,
	STENCIL_STATE_Default = 0,
	STENCIL_STATE_NoZ,
	STENCIL_STATE_NoZWrite,
	STENCIL_STATE_Max
};

enum BlendState
{
	BLEND_STATE_Invalid = -1,
	BLEND_STATE_Default = 0,
	BLEND_STATE_Alpha,
	BLEND_STATE_Solid,
	BLEND_STATE_Add,
	BLEND_STATE_Multiply,
	BLEND_STATE_Multiply2,
	BLEND_STATE_Mask,
	BLEND_STATE_MaskAdd,
	BLEND_STATE_Multiply3D,
	BLEND_STATE_Invert,
	BLEND_STATE_Max
};

#define CRS_WIREFRAME		(1<<0)
#define CRS_NO_CULL			(1<<1)
#define CRS_CULL_CCW		(1<<2)
#define CRS_NO_DEPTH_CLIP	(1<<3)

enum RasterState
{
	RASTER_STATE_Invalid = -1,
	RASTER_STATE_Default = 0,
	RASTER_STATE_Wireframe,
	RASTER_STATE_Cullback,
	RASTER_STATE_CullbackWireframe,
	RASTER_STATE_CullbackCCW,
	RASTER_STATE_CullbackNoDepthClip,
	RASTER_STATE_Max
};

enum SamplerState
{
	SAMPLER_STATE_Invalid = -1,
	SAMPLER_STATE_Point = 0,
	SAMPLER_STATE_Linear,
	SAMPLER_STATE_Anisotropic,
	SAMPLER_STATE_PointClamp,
	SAMPLER_STATE_LinearClamp,
	SAMPLER_STATE_AnisotropicClamp,
	SAMPLER_STATE_Max
};

typedef std::vector<ID3D11DepthStencilState*> Array_PStencilState;
typedef std::vector<ID3D11BlendState1*> Array_PBlendState;
typedef std::vector<ID3D11RasterizerState1*> Array_PRasterState;
typedef std::vector<ID3D11SamplerState*> Array_PSamplerState;

class CRenderStateMgr
{

public:

	CRenderStateMgr() 
	{ 
		m_eStencilState = STENCIL_STATE_Invalid;
		m_eBlendState = BLEND_STATE_Invalid;
		m_eRasterState = RASTER_STATE_Invalid;

		for (int i = 0; i < MAX_SAMPLERS; i++)
			m_aeSamplerState[i] = SAMPLER_STATE_Invalid;

		m_fCurMipMapBias = 0.0f;
		m_nCurMaxAnisotropy = D3D11_MAX_MAXANISOTROPY;
		
		m_eSavedStencilState = STENCIL_STATE_Invalid;
		m_eSavedBlendState = BLEND_STATE_Invalid;
		m_eSavedRasterState = RASTER_STATE_Invalid;

		for (int i = 0; i < MAX_SAMPLERS; i++)
			m_aeSavedSamplerState[i] = SAMPLER_STATE_Invalid;
	}

	~CRenderStateMgr();

	bool	Init();
	void	FreeAll();
	void	FreeSamplerStates();

	LTBOOL	CreateStencilState(uint32 dwFlags);
	LTBOOL	CreateBlendState(bool bBlendEnabled, uint32 dwSrcBlend, uint32 dwDestBlend);
	LTBOOL	CreateRasterState(uint32 dwFlags);
	LTBOOL	CreateSamplerStates();
	void	RecreateSamplerStates();
	LTBOOL	CreateSamplerState(D3D11_FILTER eFilter, int nMaxAnisotropy, float fMipMapBias, D3D11_TEXTURE_ADDRESS_MODE eAddressMode);

	void	SaveAllStates();
	void	RestoreAllStates();

	void	SavePrimaryStates();
	void	RestorePrimaryStates();

	StencilState	GetStencilState() { return m_eStencilState; }
	BlendState		GetBlendState() { return m_eBlendState; }
	RasterState		GetRasterState() { return m_eRasterState; }
	SamplerState	GetSamplerState(uint32 dwSlot) { return m_aeSamplerState[dwSlot]; }

	StencilState	GetSavedStencilState() { return m_eSavedStencilState; }
	BlendState		GetSavedBlendState() { return m_eSavedBlendState; }
	RasterState		GetSavedRasterState() { return m_eSavedRasterState; }
	SamplerState	GetSavedSamplerState(uint32 dwSlot) { return m_aeSavedSamplerState[dwSlot]; }

	void	SetStencilState(StencilState eState);
	void	SetBlendState(BlendState eState);
	void	SetRasterState(RasterState eState);
	void	SetSamplerStates(SamplerState eState1);

	void	SaveStencilState() { m_eSavedStencilState = m_eStencilState; };
	void	SaveBlendState() { m_eSavedBlendState = m_eBlendState; };
	void	SaveRasterState() { m_eSavedRasterState = m_eRasterState; };
	void	SaveSamplerStates();

	void	RestoreStencilState() { SetStencilState(m_eSavedStencilState); }
	void	RestoreBlendState() { SetBlendState(m_eSavedBlendState); }
	void	RestoreRasterState() { SetRasterState(m_eSavedRasterState); }
	void	RestoreSamplerStates();

private:

	StencilState	m_eStencilState;
	BlendState		m_eBlendState;
	RasterState		m_eRasterState;
	SamplerState	m_aeSamplerState[MAX_SAMPLERS];

	Array_PStencilState	m_StencilStates;
	Array_PBlendState	m_BlendStates;
	Array_PRasterState	m_RasterStates;
	Array_PSamplerState	m_SamplerStates;

	float	m_fCurMipMapBias;
	int		m_nCurMaxAnisotropy;

	StencilState	m_eSavedStencilState;
	BlendState		m_eSavedBlendState;
	RasterState		m_eSavedRasterState;
	SamplerState	m_aeSavedSamplerState[MAX_SAMPLERS];
};

extern CRenderStateMgr g_RenderStateMgr;

#endif