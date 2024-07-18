#include "pch.h"

#include "d3d_shader_passthrough.h"
#include "d3d_device.h"
#include "common_stuff.h"
#include "d3d_vertextypes.h"

bool CRenderShader_PassThrough::Init(ID3D11VertexShader* pVertexShader, ID3D11PixelShader* pPixelShader, ID3D10Blob* pVSBlob)
{
	return CRenderShader_Base::Init(pVertexShader, pPixelShader, pVSBlob);
}

void CRenderShader_PassThrough::SetPerObjectParams(ID3D11ShaderResourceView* pMainTexture)
{
	g_RenderShaderMgr.SetShaderResourcePS(SRS_PS_Primary, pMainTexture);
}

bool CRenderShader_PassThrough::Validate(uint32 dwFlags)
{
	return true;
}

void CRenderShader_PassThrough::Render()
{
	g_RenderShaderMgr.SetInputLayout(SHADER_PassThrough, nullptr);
	g_RenderShaderMgr.SetShaderVS(SHADER_PassThrough, m_pVertexShader);
	g_RenderShaderMgr.SetShaderGS(SHADER_PassThrough, nullptr);
	g_RenderShaderMgr.SetShaderPS(SHADER_PassThrough, m_pPixelShader);

	if (!Validate(0))
	{
		AddDebugMessage(0, g_szValidationError, m_szName);
		return;
	}

	g_D3DDevice.GetDeviceContext()->Draw(PASS_THROUGH_VERTICES, 0);
}