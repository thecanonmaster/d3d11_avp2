#include "pch.h"

#include "d3d_shader_particles.h"
#include "d3d_device.h"
#include "common_stuff.h"
#include "rendershadermgr.h"
#include "d3d_vertextypes.h"
#include "globalmgr.h"

CRenderShader_ParticleSystem::~CRenderShader_ParticleSystem()
{
	RELEASE_INTERFACE(m_pVPSPerObjectBuffer, g_szFreeError_VPS_PO_CB, m_szName);
}

bool CRenderShader_ParticleSystem::Init(ID3D11VertexShader* pVertexShader, ID3D11PixelShader* pPixelShader, ID3D10Blob* pVSBlob)
{
	return CRenderShader_Base::Init(pVertexShader, pPixelShader, pVSBlob) &&
		CreateInputLayout(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize()) &&
		CreateVPSConstantBuffer();
}

bool CRenderShader_ParticleSystem::SetPerObjectParams(ID3D11ShaderResourceView* pMainTexture, uint32 dwMode, DirectX::XMFLOAT3* pModeColor, 
	DirectX::XMFLOAT4* pColorScale, DirectX::XMFLOAT3* pParticleUp, DirectX::XMFLOAT3* pParticleRight, XMFloat4x4Trinity* pTransforms)
{
	auto lambdaUpdate = [dwMode, pModeColor, pTransforms, pColorScale, pParticleUp, pParticleRight]
	(D3D11_MAPPED_SUBRESOURCE* pSubResource)
		{
			VPSInputPerObject* pData = (VPSInputPerObject*)pSubResource->pData;
			pData->Init(dwMode, pModeColor, pTransforms, pColorScale, pParticleUp, pParticleRight);
		};

	if (!g_GlobalMgr.UpdateGenericBufferEx(m_pVPSPerObjectBuffer, D3D11_MAP_WRITE_DISCARD, lambdaUpdate))
		return false;

	g_RenderShaderMgr.SetConstantBuffersVS(CBS_3D_VS_ParticleSystem, 1, CBI_3D_VS_ParticleSystem, &m_pVPSPerObjectBuffer);
	g_RenderShaderMgr.SetConstantBuffersPS(CBS_3D_VS_ParticleSystem, 1, CBI_3D_PS_ParticleSystem, &m_pVPSPerObjectBuffer);

	if (pMainTexture != nullptr)
		g_RenderShaderMgr.SetShaderResourcePS(SRS_PS_Primary, pMainTexture);

	return true;
}

bool CRenderShader_ParticleSystem::CreateInputLayout(void* pVSCode, uint32 dwCodeSize)
{
	HRESULT hResult = g_D3DDevice.GetDevice()->CreateInputLayout(
		VertexTypes::TextureInstancedPositionColorScale::c_aInputElements,
		VertexTypes::TextureInstancedPositionColorScale::c_dwInputElements, pVSCode, dwCodeSize, &m_pLayout);

	if (FAILED(hResult))
		return false;

	return true;
}

bool CRenderShader_ParticleSystem::CreateVPSConstantBuffer()
{
	m_pVPSPerObjectBuffer = g_GlobalMgr.CreateConstantBuffer(sizeof(VPSInputPerObject), D3D11_USAGE_DYNAMIC, 
		D3D11_CPU_ACCESS_WRITE);
	return (m_pVPSPerObjectBuffer != nullptr);
}

bool CRenderShader_ParticleSystem::Validate(uint32 dwFlags)
{
	return g_RenderShaderMgr.ValidateCurrentConstBufferSlotVS(CBS_3D_VS_PerFrame, CBI_3D_VS_PerFrame) &&
		g_RenderShaderMgr.ValidateCurrentConstBufferSlotPS(CBS_3D_PS_PerFrame, CBI_3D_PS_PerFrame) &&
		g_RenderShaderMgr.ValidateCurrentConstBufferSlotVS(CBS_3D_VS_ParticleSystem, CBI_3D_VS_ParticleSystem) &&
		g_RenderShaderMgr.ValidateCurrentConstBufferSlotPS(CBS_3D_PS_ParticleSystem, CBI_3D_PS_ParticleSystem);
}

void CRenderShader_ParticleSystem::Render(UINT dwIndexCount, UINT dwInstanceCount)
{
	g_RenderShaderMgr.SetInputLayout(SHADER_ParticleSystem, m_pLayout);
	g_RenderShaderMgr.SetShaderVS(SHADER_ParticleSystem, m_pVertexShader);
	g_RenderShaderMgr.SetShaderGS(SHADER_ParticleSystem, nullptr);
	g_RenderShaderMgr.SetShaderPS(SHADER_ParticleSystem, m_pPixelShader);

	if (!Validate(0))
	{
		AddDebugMessage(0, g_szValidationError, m_szName);
		return;
	}

	g_D3DDevice.GetDeviceContext()->DrawIndexedInstanced(dwIndexCount, dwInstanceCount, 0, 0, 0);
}
