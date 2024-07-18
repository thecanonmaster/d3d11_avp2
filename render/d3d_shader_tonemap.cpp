#include "pch.h"

#include "d3d_shader_tonemap.h"
#include "d3d_device.h"
#include "common_stuff.h"
#include "rendershadermgr.h"
#include "d3d_vertextypes.h"

bool CRenderShader_ToneMap::Init(ID3D11VertexShader* pVertexShader, ID3D11PixelShader* pPixelShader, ID3D10Blob* pVSBlob)
{
	return CRenderShader_Base::Init(pVertexShader, pPixelShader, pVSBlob);
}

void CRenderShader_ToneMap::SetPerObjectParams(ID3D11ShaderResourceView* pMainTexture)
{
	g_RenderShaderMgr.SetShaderResourcePS(SRS_PS_Primary, pMainTexture);
}

bool CRenderShader_ToneMap::Validate(uint32 dwFlags)
{
	return g_RenderShaderMgr.ValidateCurrentConstBufferSlotPS(CBS_Opt2D_PS_PerFrame, CBI_Opt2D_PS_PerFrame);
}

void CRenderShader_ToneMap::Render()
{
	g_RenderShaderMgr.SetInputLayout(SHADER_ToneMap, nullptr);
	g_RenderShaderMgr.SetShaderVS(SHADER_ToneMap, m_pVertexShader);
	g_RenderShaderMgr.SetShaderGS(SHADER_ToneMap, nullptr);
	g_RenderShaderMgr.SetShaderPS(SHADER_ToneMap, m_pPixelShader);

	// TODO - fails on resolution change/render reset (when there are no 2D elements?)
	if (!Validate(0) && g_RenderShaderMgr.GetCurrentConstBufferSlotVS(CBS_Opt2D_PS_PerFrame) != CBI_VS_Invalid)
	{
		AddDebugMessage(0, g_szValidationError, m_szName);
		return;
	}

	g_D3DDevice.GetDeviceContext()->Draw(TONE_MAP_VERTICES, 0);
}