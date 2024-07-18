#include "pch.h"

#include "d3d_shader_polygrid.h"
#include "d3d_device.h"
#include "common_stuff.h"
#include "rendershadermgr.h"
#include "d3d_vertextypes.h"
#include "globalmgr.h"

CRenderShader_PolyGrid::~CRenderShader_PolyGrid()
{
	RELEASE_INTERFACE(m_pVPSPerObjectBuffer, g_szFreeError_VPS_PO_CB, m_szName);
}

bool CRenderShader_PolyGrid::Init(ID3D11VertexShader* pVertexShader, ID3D11PixelShader* pPixelShader, ID3D10Blob* pVSBlob)
{
	return CRenderShader_Base::Init(pVertexShader, pPixelShader, pVSBlob) &&
		CreateInputLayout(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize()) &&
		CreateVPSConstantBuffer();
}

bool CRenderShader_PolyGrid::SetPerObjectParams(ID3D11ShaderResourceView* pMainTexture, ID3D11ShaderResourceView* pEnvTexture, 
	uint32 dwMode, DirectX::XMFLOAT3* pModeColor, DirectX::XMFLOAT4* pColorScale, XMFloat4x4Trinity* pTransforms)
{
	auto lambdaUpdateVS = [dwMode, pModeColor, pTransforms, pColorScale](D3D11_MAPPED_SUBRESOURCE* pSubResource)
		{
			VPSInputPerObject* pData = (VPSInputPerObject*)pSubResource->pData;
			pData->Init(dwMode, pModeColor, pTransforms, pColorScale);
		};

	if (!g_GlobalMgr.UpdateGenericBufferEx(m_pVPSPerObjectBuffer, D3D11_MAP_WRITE_DISCARD, lambdaUpdateVS))
		return false;

	g_RenderShaderMgr.SetConstantBuffersVS(CBS_3D_VS_PolyGrid, 1, CBI_3D_VS_PolyGrid, &m_pVPSPerObjectBuffer);
	g_RenderShaderMgr.SetConstantBuffersPS(CBS_3D_PS_PolyGrid, 1, CBI_3D_PS_PolyGrid, &m_pVPSPerObjectBuffer);

	if (pMainTexture != nullptr)
		g_RenderShaderMgr.SetShaderResourcePS(SRS_PS_Primary, pMainTexture);

	if (pEnvTexture != nullptr)
		g_RenderShaderMgr.SetShaderResourcePS(SRS_PS_Other, pEnvTexture);

	return true;
}

bool CRenderShader_PolyGrid::CreateInputLayout(void* pVSCode, uint32 dwCodeSize)
{
	HRESULT hResult = g_D3DDevice.GetDevice()->CreateInputLayout(
		VertexTypes::PositionNormalColorTexture::c_aInputElements,
		VertexTypes::PositionNormalColorTexture::c_dwInputElements, pVSCode, dwCodeSize, &m_pLayout);

	if (FAILED(hResult))
		return false;

	return true;
}

bool CRenderShader_PolyGrid::CreateVPSConstantBuffer()
{
	m_pVPSPerObjectBuffer = g_GlobalMgr.CreateConstantBuffer(sizeof(VPSInputPerObject), D3D11_USAGE_DYNAMIC, 
		D3D11_CPU_ACCESS_WRITE);	
	return (m_pVPSPerObjectBuffer != nullptr);
}

bool CRenderShader_PolyGrid::Validate(uint32 dwFlags)
{
	return g_RenderShaderMgr.ValidateCurrentConstBufferSlotVS(CBS_3D_VS_PerFrame, CBI_3D_VS_PerFrame) &&
		g_RenderShaderMgr.ValidateCurrentConstBufferSlotPS(CBS_3D_PS_PerFrame, CBI_3D_PS_PerFrame) &&
		g_RenderShaderMgr.ValidateCurrentConstBufferSlotVS(CBS_3D_VS_PolyGrid, CBI_3D_VS_PolyGrid) &&
		g_RenderShaderMgr.ValidateCurrentConstBufferSlotPS(CBS_3D_PS_PolyGrid, CBI_3D_PS_PolyGrid);
}

void CRenderShader_PolyGrid::Render(UINT dwIndexCount)
{
	g_RenderShaderMgr.SetInputLayout(SHADER_PolyGrid, m_pLayout);
	g_RenderShaderMgr.SetShaderVS(SHADER_PolyGrid, m_pVertexShader);
	g_RenderShaderMgr.SetShaderGS(SHADER_PolyGrid, nullptr);
	g_RenderShaderMgr.SetShaderPS(SHADER_PolyGrid, m_pPixelShader);

	if (!Validate(0))
	{
		AddDebugMessage(0, g_szValidationError, m_szName);
		return;
	}

	g_D3DDevice.GetDeviceContext()->DrawIndexed(dwIndexCount, 0, 0);
}
