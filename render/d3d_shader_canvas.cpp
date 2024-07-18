#include "pch.h"

#include "d3d_shader_canvas.h"
#include "d3d_device.h"
#include "common_stuff.h"
#include "rendershadermgr.h"
#include "d3d_vertextypes.h"
#include "draw_canvas.h"
#include "globalmgr.h"

CRenderShader_Canvas::~CRenderShader_Canvas()
{
	RELEASE_INTERFACE(m_pVPSPerObjectBuffer, g_szFreeError_VPS_PO_CB, m_szName);
}

bool CRenderShader_Canvas::Init(ID3D11VertexShader* pVertexShader, ID3D11PixelShader* pPixelShader, ID3D10Blob* pVSBlob)
{
	return CRenderShader_Base::Init(pVertexShader, pPixelShader, pVSBlob) &&
		CreateInputLayout(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize()) &&
		CreateVPSConstantBuffer();
}

bool CRenderShader_Canvas::SetPerObjectParams(ID3D11ShaderResourceView* pMainTexture, uint32 dwRenderMode, 
	DirectX::XMFLOAT3* pModeColor, XMFloat4x4Trinity* pTransforms)
{
	auto lambdaUpdateVPS = [dwRenderMode, pModeColor, pTransforms](D3D11_MAPPED_SUBRESOURCE* pSubResource)
		{
			VPSInputPerObject* pData = (VPSInputPerObject*)pSubResource->pData;
			pData->Init(dwRenderMode, pModeColor, pTransforms);
		};

	if (!g_GlobalMgr.UpdateGenericBufferEx(m_pVPSPerObjectBuffer, D3D11_MAP_WRITE_DISCARD, lambdaUpdateVPS))
		return false;

	g_RenderShaderMgr.SetConstantBuffersVS(CBS_3D_VS_Canvas, 1, CBI_3D_VS_Canvas, &m_pVPSPerObjectBuffer);
	g_RenderShaderMgr.SetConstantBuffersPS(CBS_3D_PS_Canvas, 1, CBI_3D_PS_Canvas, &m_pVPSPerObjectBuffer);

	if (pMainTexture != nullptr)
		g_RenderShaderMgr.SetShaderResourcePS(SRS_PS_Primary, pMainTexture);

	return true;
}

bool CRenderShader_Canvas::CreateInputLayout(void* pVSCode, uint32 dwCodeSize)
{
	HRESULT hResult = g_D3DDevice.GetDevice()->CreateInputLayout(VertexTypes::PositionColorTexture::c_aInputElements,
		VertexTypes::PositionColorTexture::c_dwInputElements, pVSCode, dwCodeSize, &m_pLayout);

	if (FAILED(hResult))
		return false;

	return true;
}

bool CRenderShader_Canvas::CreateVPSConstantBuffer()
{
	m_pVPSPerObjectBuffer = g_GlobalMgr.CreateConstantBuffer(sizeof(VPSInputPerObject), D3D11_USAGE_DYNAMIC, 
		D3D11_CPU_ACCESS_WRITE);
	return (m_pVPSPerObjectBuffer != nullptr);
}

bool CRenderShader_Canvas::Validate(uint32 dwFlags)
{
	return g_RenderShaderMgr.ValidateCurrentConstBufferSlotVS(CBS_3D_VS_PerFrame, CBI_3D_VS_PerFrame) &&
		g_RenderShaderMgr.ValidateCurrentConstBufferSlotPS(CBS_3D_PS_PerFrame, CBI_3D_PS_PerFrame) &&
		g_RenderShaderMgr.ValidateCurrentConstBufferSlotVS(CBS_3D_VS_Canvas, CBI_3D_VS_Canvas) &&
		g_RenderShaderMgr.ValidateCurrentConstBufferSlotPS(CBS_3D_PS_Canvas, CBI_3D_PS_Canvas);
}

void CRenderShader_Canvas::Render(uint32 dwVertexCount)
{
	g_RenderShaderMgr.SetInputLayout(SHADER_Canvas, m_pLayout);
	g_RenderShaderMgr.SetShaderVS(SHADER_Canvas, m_pVertexShader);
	g_RenderShaderMgr.SetShaderGS(SHADER_Canvas, nullptr);
	g_RenderShaderMgr.SetShaderPS(SHADER_Canvas, m_pPixelShader);

	if (!Validate(0))
	{
		AddDebugMessage(0, g_szValidationError, m_szName);
		return;
	}

	g_D3DDevice.GetDeviceContext()->Draw(dwVertexCount, 0);
}

CRenderShader_CanvasBatch::~CRenderShader_CanvasBatch()
{
	RELEASE_INTERFACE(m_pVPSPerObjectBuffer, g_szFreeError_VPS_PO_CB, m_szName);
}

bool CRenderShader_CanvasBatch::Init(ID3D11VertexShader* pVertexShader, ID3D11PixelShader* pPixelShader, ID3D10Blob* pVSBlob)
{
	return CRenderShader_Base::Init(pVertexShader, pPixelShader, pVSBlob) &&
		CreateInputLayout(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize()) &&
		CreateVPSConstantBuffer();
}

bool CRenderShader_CanvasBatch::SetPerObjectParams(ID3D11ShaderResourceView** ppTextures, uint32 dwTextureCount, 
	XMFloat4x4Trinity* pTransforms, XMFloat4x4Trinity* pTransformsRC, VPSPerObjectParams* pParams, uint32 dwCanvasCount)
{
	auto lambdaUpdate = [pTransforms, pTransformsRC, pParams, dwCanvasCount](D3D11_MAPPED_SUBRESOURCE* pSubResource)
		{
			VPSInputPerObject* pData = (VPSInputPerObject*)pSubResource->pData;
			pData->Init(pTransforms, pTransformsRC, pParams, dwCanvasCount);
		};

	if (!g_GlobalMgr.UpdateGenericBufferEx(m_pVPSPerObjectBuffer, D3D11_MAP_WRITE_DISCARD, lambdaUpdate))
		return false;

	g_RenderShaderMgr.SetConstantBuffersVS(CBS_3D_VS_Canvas, 1, CBI_3D_VS_Canvas, &m_pVPSPerObjectBuffer);
	g_RenderShaderMgr.SetConstantBuffersPS(CBS_3D_PS_Canvas, 1, CBI_3D_PS_Canvas, &m_pVPSPerObjectBuffer);

	for (uint32 i = 0; i < dwTextureCount; i++)
		g_RenderShaderMgr.SetShaderResourcePS(SRS_PS_Other + i, ppTextures[i]);

	return true;
}

bool CRenderShader_CanvasBatch::CreateInputLayout(void* pVSCode, uint32 dwCodeSize)
{
	HRESULT hResult = g_D3DDevice.GetDevice()->CreateInputLayout(VertexTypes::PositionColorTextureIndex::c_aInputElements,
		VertexTypes::PositionColorTextureIndex::c_dwInputElements, pVSCode, dwCodeSize, &m_pLayout);

	if (FAILED(hResult))
		return false;

	return true;
}

bool CRenderShader_CanvasBatch::CreateVPSConstantBuffer()
{
	m_pVPSPerObjectBuffer = g_GlobalMgr.CreateConstantBuffer(sizeof(VPSInputPerObject), D3D11_USAGE_DYNAMIC,
		D3D11_CPU_ACCESS_WRITE);
	return (m_pVPSPerObjectBuffer != nullptr);
}

bool CRenderShader_CanvasBatch::Validate(uint32 dwFlags)
{
	return g_RenderShaderMgr.ValidateCurrentConstBufferSlotVS(CBS_3D_VS_PerFrame, CBI_3D_VS_PerFrame) &&
		g_RenderShaderMgr.ValidateCurrentConstBufferSlotPS(CBS_3D_PS_PerFrame, CBI_3D_PS_PerFrame) &&
		g_RenderShaderMgr.ValidateCurrentConstBufferSlotVS(CBS_3D_VS_Canvas, CBI_3D_VS_Canvas) &&
		g_RenderShaderMgr.ValidateCurrentConstBufferSlotPS(CBS_3D_PS_Canvas, CBI_3D_PS_Canvas);
}

void CRenderShader_CanvasBatch::Render(uint32 dwVertexCount, uint32 dwStartVertex)
{
	g_RenderShaderMgr.SetInputLayout(SHADER_Canvas, m_pLayout);
	g_RenderShaderMgr.SetShaderVS(SHADER_Canvas, m_pVertexShader);
	g_RenderShaderMgr.SetShaderGS(SHADER_Canvas, nullptr);
	g_RenderShaderMgr.SetShaderPS(SHADER_Canvas, m_pPixelShader);

	if (!Validate(0))
	{
		AddDebugMessage(0, g_szValidationError, m_szName);
		return;
	}

	g_D3DDevice.GetDeviceContext()->Draw(dwVertexCount, dwStartVertex);
}