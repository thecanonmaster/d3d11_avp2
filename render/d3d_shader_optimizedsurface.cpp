#include "pch.h"

#include "d3d_shader_optimizedsurface.h"
#include "d3d_device.h"
#include "common_stuff.h"
#include "rendershadermgr.h"
#include "d3d_vertextypes.h"
#include "globalmgr.h"

CRenderShader_OptimizedSurface::~CRenderShader_OptimizedSurface()
{
	RELEASE_INTERFACE(m_pPSPerObjectBuffer, g_szFreeError_PS_PO_CB, m_szName);
}

bool CRenderShader_OptimizedSurface::Init(ID3D11VertexShader* pVertexShader, ID3D11PixelShader* pPixelShader, ID3D10Blob* pVSBlob)
{
	return CRenderShader_Base::Init(pVertexShader, pPixelShader, pVSBlob) &&
		CreateInputLayout(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize()) &&
		CreatePSConstantBuffer();
}

bool CRenderShader_OptimizedSurface::SetPerObjectParams(ID3D11ShaderResourceView* pMainTexture, DirectX::XMFLOAT3* pTransparentColor, 
	float fAlpha, DirectX::XMFLOAT3* pBaseColor)
{
	auto lambdaUpdate = [pTransparentColor, fAlpha, pBaseColor](D3D11_MAPPED_SUBRESOURCE* pSubResource)
		{
			PSInputPerObject* pData = (PSInputPerObject*)pSubResource->pData;
			pData->Init(pTransparentColor, fAlpha, pBaseColor);
		};

	if (!g_GlobalMgr.UpdateGenericBufferEx(m_pPSPerObjectBuffer, D3D11_MAP_WRITE_DISCARD, lambdaUpdate))
		return false;

	g_RenderShaderMgr.SetConstantBuffersPS(CBS_Opt2D_PS_OptimizedSurface, 1, CBI_Opt2D_PS_OptimizedSurface, &m_pPSPerObjectBuffer);
	g_RenderShaderMgr.SetShaderResourcePS(SRS_PS_Primary, pMainTexture);

	return true;
}

bool CRenderShader_OptimizedSurface::CreateInputLayout(void* pVSCode, uint32 dwCodeSize)
{
	HRESULT hResult = g_D3DDevice.GetDevice()->CreateInputLayout(VertexTypes::PositionTexture::c_aInputElements,
		VertexTypes::PositionTexture::c_dwInputElements, pVSCode, dwCodeSize, &m_pLayout);

	if (FAILED(hResult))
		return false;

	return true;
}

bool CRenderShader_OptimizedSurface::CreatePSConstantBuffer()
{
	m_pPSPerObjectBuffer = g_GlobalMgr.CreateConstantBuffer(sizeof(PSInputPerObject), D3D11_USAGE_DYNAMIC, 
		D3D11_CPU_ACCESS_WRITE);
	return (m_pPSPerObjectBuffer != nullptr);
}

bool CRenderShader_OptimizedSurface::Validate(uint32 dwFlags)
{
	return g_RenderShaderMgr.ValidateCurrentConstBufferSlotVS(CBS_Opt2D_VS_PerFrame, CBI_Opt2D_VS_PerFrame) &&
		g_RenderShaderMgr.ValidateCurrentConstBufferSlotPS(CBS_Opt2D_PS_OptimizedSurface, CBI_Opt2D_PS_OptimizedSurface);
}

void CRenderShader_OptimizedSurface::Render()
{
	g_RenderShaderMgr.SetInputLayout(SHADER_OptimizedSurface, m_pLayout);
	g_RenderShaderMgr.SetShaderVS(SHADER_OptimizedSurface, m_pVertexShader);
	g_RenderShaderMgr.SetShaderGS(SHADER_OptimizedSurface, nullptr);
	g_RenderShaderMgr.SetShaderPS(SHADER_OptimizedSurface, m_pPixelShader);

	if (!Validate(0))
	{
		AddDebugMessage(0, g_szValidationError, m_szName);
		return;
	}

	g_D3DDevice.GetDeviceContext()->DrawIndexed(OPTIMIZED_SURFACE_INDICES, 0, 0);
}

CRenderShader_OptimizedSurfaceBatch::~CRenderShader_OptimizedSurfaceBatch()
{
	RELEASE_INTERFACE(m_pPSPerObjectBuffer, g_szFreeError_PS_PO_CB, m_szName);
}

bool CRenderShader_OptimizedSurfaceBatch::Init(ID3D11VertexShader* pVertexShader, ID3D11PixelShader* pPixelShader, ID3D10Blob* pVSBlob)
{
	return CRenderShader_Base::Init(pVertexShader, pPixelShader, pVSBlob) &&
		CreateInputLayout(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize()) &&
		CreatePSConstantBuffer();
}

bool CRenderShader_OptimizedSurfaceBatch::SetPerObjectParams(ID3D11ShaderResourceView** ppTextures, uint32 dwTextureCount,
	PSPerObjectParams* pParams, uint32 dwSurfaceCount)
{
	auto lambdaUpdate = [pParams, dwSurfaceCount](D3D11_MAPPED_SUBRESOURCE* pSubResource)
		{
			PSInputPerObject* pData = (PSInputPerObject*)pSubResource->pData;
			pData->InitAll(pParams, dwSurfaceCount);
		};

	if (!g_GlobalMgr.UpdateGenericBufferEx(m_pPSPerObjectBuffer, D3D11_MAP_WRITE_DISCARD, lambdaUpdate))
		return false;

	g_RenderShaderMgr.SetConstantBuffersPS(CBS_Opt2D_PS_OptimizedSurface, 1, CBI_Opt2D_PS_OptimizedSurface, 
		&m_pPSPerObjectBuffer);

	for (uint32 i = 0; i < dwTextureCount; i++)
		g_RenderShaderMgr.SetShaderResourcePS(SRS_PS_Other + i, ppTextures[i]);
	
	return true;
}

bool CRenderShader_OptimizedSurfaceBatch::CreateInputLayout(void* pVSCode, uint32 dwCodeSize)
{
	HRESULT hResult = g_D3DDevice.GetDevice()->CreateInputLayout(VertexTypes::PositionTextureIndex::c_aInputElements,
		VertexTypes::PositionTextureIndex::c_dwInputElements, pVSCode, dwCodeSize, &m_pLayout);

	if (FAILED(hResult))
		return false;

	return true;
}

bool CRenderShader_OptimizedSurfaceBatch::CreatePSConstantBuffer()
{
	m_pPSPerObjectBuffer = g_GlobalMgr.CreateConstantBuffer(sizeof(PSInputPerObject), D3D11_USAGE_DYNAMIC,
		D3D11_CPU_ACCESS_WRITE);
	return (m_pPSPerObjectBuffer != nullptr);
}

bool CRenderShader_OptimizedSurfaceBatch::Validate(uint32 dwFlags)
{
	return g_RenderShaderMgr.ValidateCurrentConstBufferSlotVS(CBS_Opt2D_VS_PerFrame, CBI_Opt2D_VS_PerFrame) &&
		g_RenderShaderMgr.ValidateCurrentConstBufferSlotPS(CBS_Opt2D_PS_OptimizedSurface, CBI_Opt2D_PS_OptimizedSurface);
}

void CRenderShader_OptimizedSurfaceBatch::Render(uint32 dwIndexCount, uint32 dwStartIndex)
{
	g_RenderShaderMgr.SetInputLayout(SHADER_OptimizedSurfaceBatch, m_pLayout);
	g_RenderShaderMgr.SetShaderVS(SHADER_OptimizedSurfaceBatch, m_pVertexShader);
	g_RenderShaderMgr.SetShaderGS(SHADER_OptimizedSurfaceBatch, nullptr);
	g_RenderShaderMgr.SetShaderPS(SHADER_OptimizedSurfaceBatch, m_pPixelShader);

	if (!Validate(0))
	{
		AddDebugMessage(0, g_szValidationError, m_szName);
		return;
	}

	g_D3DDevice.GetDeviceContext()->DrawIndexed(dwIndexCount, dwStartIndex, 0);
}