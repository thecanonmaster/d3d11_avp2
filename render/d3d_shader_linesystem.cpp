#include "pch.h"

#include "d3d_shader_linesystem.h"
#include "d3d_device.h"
#include "common_stuff.h"
#include "rendershadermgr.h"
#include "d3d_vertextypes.h"
#include "globalmgr.h"

CRenderShader_LineSystem::~CRenderShader_LineSystem()
{
	RELEASE_INTERFACE(m_pVPSPerObjectBuffer, g_szFreeError_VPS_PO_CB, m_szName);
}

bool CRenderShader_LineSystem::Init(ID3D11VertexShader* pVertexShader, ID3D11PixelShader* pPixelShader, ID3D10Blob* pVSBlob)
{
	return CRenderShader_Base::Init(pVertexShader, pPixelShader, pVSBlob) &&
		CreateInputLayout(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize()) &&
		CreateVPSConstantBuffer();
}

bool CRenderShader_LineSystem::SetPerObjectParams(uint32 dwRenderMode, XMFloat4x4Trinity* pTransforms, float fObjectAlpha)
{
	auto lambdaUpdateVS = [dwRenderMode, pTransforms, fObjectAlpha](D3D11_MAPPED_SUBRESOURCE* pSubResource)
		{
			VPSInputPerObject* pData = (VPSInputPerObject*)pSubResource->pData;
			pData->Init(dwRenderMode, pTransforms, fObjectAlpha);
		};

	if (!g_GlobalMgr.UpdateGenericBufferEx(m_pVPSPerObjectBuffer, D3D11_MAP_WRITE_DISCARD, lambdaUpdateVS))
		return false;

	g_RenderShaderMgr.SetConstantBuffersVS(CBS_3D_VS_LineSystem, 1, CBI_3D_VS_LineSystem, &m_pVPSPerObjectBuffer);
	g_RenderShaderMgr.SetConstantBuffersPS(CBS_3D_PS_LineSystem, 1, CBI_3D_PS_LineSystem, &m_pVPSPerObjectBuffer);

	return true;
}

bool CRenderShader_LineSystem::CreateInputLayout(void* pVSCode, uint32 dwCodeSize)
{
	HRESULT hResult = g_D3DDevice.GetDevice()->CreateInputLayout(VertexTypes::PositionColor::c_aInputElements,
		VertexTypes::PositionColor::c_dwInputElements, pVSCode, dwCodeSize, &m_pLayout);

	if (FAILED(hResult))
		return false;

	return true;
}

bool CRenderShader_LineSystem::CreateVPSConstantBuffer()
{
	m_pVPSPerObjectBuffer = g_GlobalMgr.CreateConstantBuffer(sizeof(VPSInputPerObject), D3D11_USAGE_DYNAMIC, 
		D3D11_CPU_ACCESS_WRITE);
	return (m_pVPSPerObjectBuffer != nullptr);
}

bool CRenderShader_LineSystem::Validate(uint32 dwFlags)
{
	return g_RenderShaderMgr.ValidateCurrentConstBufferSlotVS(CBS_3D_VS_PerFrame, CBI_3D_VS_PerFrame) &&
		g_RenderShaderMgr.ValidateCurrentConstBufferSlotPS(CBS_3D_PS_PerFrame, CBI_3D_PS_PerFrame) &&
		g_RenderShaderMgr.ValidateCurrentConstBufferSlotVS(CBS_3D_VS_LineSystem, CBI_3D_VS_LineSystem) &&
		g_RenderShaderMgr.ValidateCurrentConstBufferSlotPS(CBS_3D_PS_LineSystem, CBI_3D_PS_LineSystem);
}

void CRenderShader_LineSystem::Render(UINT dwVertexCount)
{
	g_RenderShaderMgr.SetInputLayout(SHADER_LineSystem, m_pLayout);
	g_RenderShaderMgr.SetShaderVS(SHADER_LineSystem, m_pVertexShader);
	g_RenderShaderMgr.SetShaderGS(SHADER_LineSystem, nullptr);
	g_RenderShaderMgr.SetShaderPS(SHADER_LineSystem, m_pPixelShader);

	if (!Validate(0))
	{
		AddDebugMessage(0, g_szValidationError, m_szName);
		return;
	}

	g_D3DDevice.GetDeviceContext()->Draw(dwVertexCount, 0);
}
