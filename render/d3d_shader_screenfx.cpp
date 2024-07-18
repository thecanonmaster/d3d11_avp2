#include "pch.h"

#include "d3d_shader_screenfx.h"
#include "d3d_device.h"
#include "common_stuff.h"
#include "rendershadermgr.h"
#include "d3d_vertextypes.h"

bool CRenderShader_ScreenFX::Init(ID3D11VertexShader* pVertexShader, ID3D11PixelShader* pPixelShader, ID3D10Blob* pVSBlob)
{
	return CRenderShader_Base::Init(pVertexShader, pPixelShader, pVSBlob);
}

void CRenderShader_ScreenFX::SetPerObjectParams(ID3D11ShaderResourceView* pMainTexture)
{
	g_RenderShaderMgr.SetShaderResourcePS(SRS_PS_Primary, pMainTexture);
}

bool CRenderShader_ScreenFX::Validate(uint32 dwFlags)
{
	return g_RenderShaderMgr.ValidateCurrentConstBufferSlotPS(CBS_3D_PS_PerFrame, CBI_3D_PS_PerFrame);
}

void CRenderShader_ScreenFX::Render()
{
	g_RenderShaderMgr.SetInputLayout(SHADER_ScreenFX, nullptr);
	g_RenderShaderMgr.SetShaderVS(SHADER_ScreenFX, m_pVertexShader);
	g_RenderShaderMgr.SetShaderGS(SHADER_ScreenFX, nullptr);
	g_RenderShaderMgr.SetShaderPS(SHADER_ScreenFX, m_pPixelShader);

	if (!Validate(0))
	{
		AddDebugMessage(0, g_szValidationError, m_szName);
		return;
	}

	g_D3DDevice.GetDeviceContext()->Draw(SCREEN_FX_VERTICES, 0);
}