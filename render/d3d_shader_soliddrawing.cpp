#include "pch.h"

#include "d3d_shader_soliddrawing.h"
#include "d3d_device.h"
#include "common_stuff.h"
#include "d3d_optimizedsurface.h"
#include "d3d_vertextypes.h"

bool CRenderShader_SolidDrawing::Init(ID3D11VertexShader* pVertexShader, ID3D11PixelShader* pPixelShader, ID3D10Blob* pVSBlob)
{
	return CRenderShader_Base::Init(pVertexShader, pPixelShader, pVSBlob);
}

void CRenderShader_SolidDrawing::SetPerObjectParams(ID3D11ShaderResourceView* pMainTexture)
{
	g_RenderShaderMgr.SetShaderResourcePS(SRS_PS_Primary, pMainTexture);
}

bool CRenderShader_SolidDrawing::Validate(uint32 dwFlags)
{
	return true;
}

void CRenderShader_SolidDrawing::Render()
{
	g_RenderShaderMgr.SetInputLayout(SHADER_SolidDrawing, nullptr);
	g_RenderShaderMgr.SetShaderVS(SHADER_SolidDrawing, m_pVertexShader);
	g_RenderShaderMgr.SetShaderGS(SHADER_SolidDrawing, nullptr);
	g_RenderShaderMgr.SetShaderPS(SHADER_SolidDrawing, m_pPixelShader);

	if (!Validate(0))
		return;

	g_D3DDevice.GetDeviceContext()->Draw(SOLID_DRAWING_VERTICES, 0);
}