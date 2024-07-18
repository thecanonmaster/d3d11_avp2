#include "pch.h"

#include "d3d_postprocess.h"
#include "d3d_device.h"
#include "renderstatemgr.h"
#include "rendershadermgr.h"
#include "rendertargetmgr.h"
#include "rendertarget.h"
#include "d3d_optimizedsurface.h"
#include "d3d_shader_soliddrawing.h"
#include "d3d_shader_tonemap.h"
#include "d3d_shader_screenfx.h"
#include "d3d_shader_clearscreen.h"
#include "d3d_shader_passthrough.h"
#include "d3d_shader_bloom.h"
#include "rendererconsolevars.h"

#define PPS_FLAG_ALPHA			(1<<0)
#define PPS_FLAG_POINT_FILTER	(1<<1)
#define PPS_FLAG_FULL_VIEWPORT	(1<<2)

static void d3d_PostProcess_SetStates(uint32 dwFlags)
{
	if (dwFlags & PPS_FLAG_FULL_VIEWPORT)
		g_D3DDevice.SetupFullViewport();
	
	g_RenderStateMgr.SetStencilState(STENCIL_STATE_NoZ);
	g_RenderStateMgr.SetBlendState((dwFlags & PPS_FLAG_ALPHA) ? BLEND_STATE_Alpha : BLEND_STATE_Default);
	g_RenderStateMgr.SetRasterState(RASTER_STATE_Default);
	g_RenderStateMgr.SetSamplerStates((dwFlags & PPS_FLAG_POINT_FILTER) ? SAMPLER_STATE_Point : SAMPLER_STATE_Linear);

	g_RenderShaderMgr.SetVertexResource(VRS_Main, nullptr, 0, 0);
	g_RenderShaderMgr.SetIndexBuffer16(nullptr, 0);
	g_RenderShaderMgr.SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

bool d3d_PostProcess_SolidDrawing(ID3D11ShaderResourceView* pSRV)
{
	d3d_PostProcess_SetStates(PPS_FLAG_ALPHA | PPS_FLAG_POINT_FILTER | PPS_FLAG_FULL_VIEWPORT);

	CRenderShader_SolidDrawing* pRenderShader = g_RenderShaderMgr.GetRenderShader<CRenderShader_SolidDrawing>();

	pRenderShader->SetPerObjectParams(pSRV);
	pRenderShader->Render();

	return true;
}

bool d3d_PostProcess_ToneMap(ID3D11ShaderResourceView* pSRV)
{
	d3d_PostProcess_SetStates(PPS_FLAG_FULL_VIEWPORT);

	CRenderShader_ToneMap* pRenderShader = g_RenderShaderMgr.GetRenderShader<CRenderShader_ToneMap>();

	pRenderShader->SetPerObjectParams(pSRV);
	pRenderShader->Render();

	return true;
}

bool d3d_PostProcess_ScreenFX(ID3D11ShaderResourceView* pSRV)
{
	d3d_PostProcess_SetStates(PPS_FLAG_FULL_VIEWPORT);

	CRenderShader_ScreenFX* pRenderShader = g_RenderShaderMgr.GetRenderShader<CRenderShader_ScreenFX>();

	pRenderShader->SetPerObjectParams(pSRV);
	pRenderShader->Render();

	return true;
}

bool d3d_PostProcess_ClearScreen(LTRect* pRect, DirectX::XMFLOAT3* pColor)
{
	d3d_PostProcess_SetStates(PPS_FLAG_ALPHA | PPS_FLAG_FULL_VIEWPORT);

	CRenderShader_ClearScreen* pRenderShader = g_RenderShaderMgr.GetRenderShader<CRenderShader_ClearScreen>();

	if (!pRenderShader->SetPerObjectParams(pRect, pColor))
		return false;

	pRenderShader->Render();

	return true;
}

bool d3d_PostProcess_PassThrough(ID3D11ShaderResourceView* pSRV)
{
	d3d_PostProcess_SetStates(PPS_FLAG_FULL_VIEWPORT);

	CRenderShader_PassThrough* pRenderShader = g_RenderShaderMgr.GetRenderShader<CRenderShader_PassThrough>();

	pRenderShader->SetPerObjectParams(pSRV);
	pRenderShader->Render();

	return true;
}

bool d3d_PostProcess_BloomExtract(ID3D11ShaderResourceView* pSRV)
{
	d3d_PostProcess_SetStates(PPS_FLAG_FULL_VIEWPORT);

	CRenderShader_BloomExtract* pRenderShader = g_RenderShaderMgr.GetRenderShader<CRenderShader_BloomExtract>();

	pRenderShader->SetPerFrameParams(g_CV_BloomThreshold.m_Val, g_CV_BloomBaseSaturation.m_Val, 
		g_CV_BloomSaturation.m_Val,g_CV_BloomBaseIntensity.m_Val, g_CV_BloomIntensity.m_Val);

	pRenderShader->SetPerObjectParams(pSRV);

	pRenderShader->Render();
	
	return true;
}

inline float GaussianDistribution(float fX, float fY, float fRho)
{
	return expf(-(fX * fX + fY * fY) / (2 * fRho * fRho)) / sqrtf(2 * DirectX::XM_PI * fRho * fRho);
}

bool d3d_PostProcess_BloomBlur(ID3D11ShaderResourceView* pSRV, bool bHorizontal, uint32 dwWidth, uint32 dwHeight)
{
	d3d_PostProcess_SetStates(PPS_FLAG_FULL_VIEWPORT);

	float fU = 0.0f;
	float fV = 0.0f;

	if (bHorizontal)
		fU = 1.0f / float(dwWidth);
	else
		fV = 1.0f / float(dwHeight);

	DirectX::XMFLOAT4 avWeight[MAX_BLOOM_SAMPLES];
	DirectX::XMFLOAT4 avOffset[MAX_BLOOM_SAMPLES];

	float fBrightness = g_CV_BloomBlurBrightness.m_Val;
	float fSize = g_CV_BloomBlurSize.m_Val;
	float fWeight = fBrightness * GaussianDistribution(0, 0, fSize);
	avWeight[0] = { fWeight, fWeight, fWeight, 1.0f };
	avOffset[0].x = avOffset[0].y = avOffset[0].z = avOffset[0].w = 0.0f;

	for (uint32 i = 1; i < 8; i++)
	{
		fWeight = fBrightness * GaussianDistribution((float)i, 0, fSize);

		avWeight[i] = { fWeight, fWeight, fWeight, 1.0f };
		avOffset[i] = { (float)i * fU, (float)i * fV, 0.0f, 0.0f };
	}

	for (uint32 i = 8; i < 15; i++)
	{
		avWeight[i] = avWeight[i - 7];
		avOffset[i] = { -avOffset[i - 7].x, -avOffset[i - 7].y, 0.0f, 0.0f };
	}

	CRenderShader_BloomBlur* pRenderShader = g_RenderShaderMgr.GetRenderShader<CRenderShader_BloomBlur>();

	pRenderShader->SetPerObjectParams(pSRV, avWeight, avOffset);

	pRenderShader->Render();
	
	return true;
}

bool d3d_PostProcess_BloomCombine(ID3D11ShaderResourceView* pMainSRV, ID3D11ShaderResourceView* pBloomSRV)
{
	d3d_PostProcess_SetStates(PPS_FLAG_FULL_VIEWPORT);

	CRenderShader_BloomCombine* pRenderShader = g_RenderShaderMgr.GetRenderShader<CRenderShader_BloomCombine>();

	pRenderShader->SetPerObjectParams(pMainSRV, pBloomSRV, 
		g_RenderShaderMgr.GetRenderShader<CRenderShader_BloomExtract>()->GetPSPerFrameBuffer());

	pRenderShader->Render();
	
	return true;
}
