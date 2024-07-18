#include "pch.h"

#include "d3d_shader_fullscreenvideo.h"
#include "d3d_shader_optimizedsurface.h"
#include "d3d_device.h"
#include "common_stuff.h"
#include "rendershadermgr.h"
#include "d3d_vertextypes.h"
#include "d3d_optimizedsurface.h"

bool CRenderShader_FullScreenVideo::Init(ID3D11VertexShader* pVertexShader, ID3D11PixelShader* pPixelShader, ID3D10Blob* pVSBlob)
{
	return CRenderShader_Base::Init(pVertexShader, pPixelShader, pVSBlob) && 
		CreateInputLayout(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize());
}

void CRenderShader_FullScreenVideo::SetPerObjectParams(ID3D11ShaderResourceView* pMainTexture)
{
	g_RenderShaderMgr.SetShaderResourcePS(SRS_PS_Primary, pMainTexture);
}

bool CRenderShader_FullScreenVideo::CreateInputLayout(void* pVSCode, uint32 dwCodeSize)
{
	HRESULT hResult = g_D3DDevice.GetDevice()->CreateInputLayout(VertexTypes::PositionTexture::c_aInputElements,
		VertexTypes::PositionTexture::c_dwInputElements, pVSCode, dwCodeSize, &m_pLayout);
	if (FAILED(hResult))
		return false;

	return true;
}

bool CRenderShader_FullScreenVideo::Validate(uint32 dwFlags)
{
	return g_RenderShaderMgr.ValidateCurrentConstBufferSlotVS(CBS_Opt2D_VS_PerFrame, CBI_Opt2D_VS_PerFrame);
}

void CRenderShader_FullScreenVideo::Render()
{
	g_RenderShaderMgr.SetInputLayout(SHADER_FullscreenVideo, m_pLayout);
	g_RenderShaderMgr.SetShaderVS(SHADER_FullscreenVideo, m_pVertexShader);
	g_RenderShaderMgr.SetShaderGS(SHADER_FullscreenVideo, nullptr);
	g_RenderShaderMgr.SetShaderPS(SHADER_FullscreenVideo, m_pPixelShader);

	if (!Validate(0) && g_RenderShaderMgr.GetCurrentConstBufferSlotVS(CBS_Opt2D_PS_PerFrame) != CBI_VS_Invalid)
	{
		AddDebugMessage(0, g_szValidationError, m_szName);
		return;
	}

	g_D3DDevice.GetDeviceContext()->DrawIndexed(OPTIMIZED_SURFACE_INDICES, 0, 0);
}
