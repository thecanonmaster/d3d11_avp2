#include "pch.h"

#include "d3d_shader_clearscreen.h"
#include "d3d_device.h"
#include "common_stuff.h"
#include "rendershadermgr.h"
#include "d3d_vertextypes.h"
#include "globalmgr.h"

CRenderShader_ClearScreen::~CRenderShader_ClearScreen()
{
	RELEASE_INTERFACE(m_pPSPerObjectBuffer, g_szFreeError_PS_PO_CB, m_szName);
}

bool CRenderShader_ClearScreen::Init(ID3D11VertexShader* pVertexShader, ID3D11PixelShader* pPixelShader, ID3D10Blob* pVSBlob)
{
	return CRenderShader_Base::Init(pVertexShader, pPixelShader, pVSBlob) && CreatePSConstantBuffer();
}

bool CRenderShader_ClearScreen::SetPerObjectParams(LTRect* pRect, DirectX::XMFLOAT3* pColor)
{
	auto lambdaUpdate = [pRect, pColor](D3D11_MAPPED_SUBRESOURCE* pSubResource)
		{
			PSInputPerObject* pData = (PSInputPerObject*)pSubResource->pData;
			pData->Init(pRect, pColor);
		};

	if (!g_GlobalMgr.UpdateGenericBufferEx(m_pPSPerObjectBuffer, D3D11_MAP_WRITE_DISCARD, lambdaUpdate))
		return false;

	g_RenderShaderMgr.SetConstantBuffersPS(CBS_Opt2D_PS_ClearScreen, 1, CBI_Opt2D_PS_ClearScreen, &m_pPSPerObjectBuffer);

	return true;
}

bool CRenderShader_ClearScreen::CreatePSConstantBuffer()
{
	m_pPSPerObjectBuffer = g_GlobalMgr.CreateConstantBuffer(sizeof(PSInputPerObject), D3D11_USAGE_DYNAMIC, 
		D3D11_CPU_ACCESS_WRITE);
	return (m_pPSPerObjectBuffer != nullptr);
}

bool CRenderShader_ClearScreen::Validate(uint32 dwFlags)
{
	return g_RenderShaderMgr.ValidateCurrentConstBufferSlotPS(CBS_Opt2D_PS_ClearScreen, CBI_Opt2D_PS_ClearScreen);
}

void CRenderShader_ClearScreen::Render()
{
	g_RenderShaderMgr.SetInputLayout(SHADER_ClearScreen, nullptr);
	g_RenderShaderMgr.SetShaderVS(SHADER_ClearScreen, m_pVertexShader);
	g_RenderShaderMgr.SetShaderGS(SHADER_ClearScreen, nullptr);
	g_RenderShaderMgr.SetShaderPS(SHADER_ClearScreen, m_pPixelShader);

	if (!Validate(0))
	{
		AddDebugMessage(0, g_szValidationError, m_szName);
		return;
	}

	g_D3DDevice.GetDeviceContext()->Draw(CLEAR_SCREEN_VERTICES, 0);
}