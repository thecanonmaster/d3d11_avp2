#include "pch.h"

#include "d3d_shader_model.h"
#include "d3d_device.h"
#include "common_stuff.h"
#include "rendershadermgr.h"
#include "d3d_vertextypes.h"
#include "globalmgr.h"

CRenderShader_Model::~CRenderShader_Model()
{
	RELEASE_INTERFACE(m_pGeometryShader, g_szFreeError_GS, m_szName);
	RELEASE_INTERFACE(m_pVSPerObjectBuffer, g_szFreeError_VS_PO_CB, m_szName);
	RELEASE_INTERFACE(m_pGSPerObjectBuffer, g_szFreeError_GS_PO_CB, m_szName);
	RELEASE_INTERFACE(m_pPSPerObjectBuffer, g_szFreeError_PS_PO_CB, m_szName);
}

bool CRenderShader_Model::Init(ID3D11VertexShader* pVertexShader, ID3D11PixelShader* pPixelShader, ID3D10Blob* pVSBlob)
{
	return CRenderShader_Base::Init(pVertexShader, pPixelShader, pVSBlob) &&
		CreateInputLayout(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize()) &&
		CreateVSConstantBuffer() && CreatePSConstantBuffer();
}

bool CRenderShader_Model::ExtraInit(ID3D11GeometryShader* pGeometryShader)
{
	m_pGeometryShader = pGeometryShader;

#ifdef MODEL_PIECELESS_RENDERING
	return CreateGSConstantBuffer();
#else
	return true;
#endif
}

bool CRenderShader_Model::SetPerObjectParamsVPS(VGPSPerObjectParams* pParams)
{
	auto lambdaUpdateVS = [pParams](D3D11_MAPPED_SUBRESOURCE* pSubResource)
		{
			VSInputPerObject* pData = (VSInputPerObject*)pSubResource->pData;
			pData->Init(pParams);
		};

	if (!g_GlobalMgr.UpdateGenericBufferEx(m_pVSPerObjectBuffer, D3D11_MAP_WRITE_DISCARD, lambdaUpdateVS))
		return false;

	auto lambdaUpdatePS = [pParams](D3D11_MAPPED_SUBRESOURCE* pSubResource)
		{
			PSInputPerObject* pData = (PSInputPerObject*)pSubResource->pData;
			pData->Init(pParams);
		};

	if (!g_GlobalMgr.UpdateGenericBufferEx(m_pPSPerObjectBuffer, D3D11_MAP_WRITE_DISCARD, lambdaUpdatePS))
		return false;

	g_RenderShaderMgr.SetConstantBuffersVS(CBS_3D_VS_Model, 1, CBI_3D_VS_Model, &m_pVSPerObjectBuffer);
	g_RenderShaderMgr.SetConstantBuffersPS(CBS_3D_PS_Model, 1, CBI_3D_PS_Model, &m_pPSPerObjectBuffer);

	g_RenderShaderMgr.SetShaderResourcesPS(SRS_PS_Other, MAX_MODEL_TEXTURES, pParams->m_apTextures);

	return true;
}

bool CRenderShader_Model::SetPerObjectParamsGS(uint32 dwHiddenPartsCount, uint32* pHiddenPrimitiveIDs)
{
	auto lambdaUpdateGS = [dwHiddenPartsCount, pHiddenPrimitiveIDs](D3D11_MAPPED_SUBRESOURCE* pSubResource)
		{
			GSInputPerObject* pData = (GSInputPerObject*)pSubResource->pData;
			pData->Init(dwHiddenPartsCount, pHiddenPrimitiveIDs);
		};

	if (!g_GlobalMgr.UpdateGenericBufferEx(m_pGSPerObjectBuffer, D3D11_MAP_WRITE_DISCARD, lambdaUpdateGS))
		return false;

	g_RenderShaderMgr.SetConstantBuffersGS(CBS_3D_GS_Model, 1, CBI_3D_GS_Model, &m_pGSPerObjectBuffer);

	return true;
}

bool CRenderShader_Model::CreateInputLayout(void* pVSCode, uint32 dwCodeSize)
{
	HRESULT hResult = g_D3DDevice.GetDevice()->CreateInputLayout(
		VertexTypes::PositionNormalTextureSkinnedIndex::c_aInputElements,
		VertexTypes::PositionNormalTextureSkinnedIndex::c_dwInputElements, pVSCode, dwCodeSize, &m_pLayout);

	if (FAILED(hResult))
		return false;

	return true;
}

bool CRenderShader_Model::CreateVSConstantBuffer()
{
	m_pVSPerObjectBuffer = g_GlobalMgr.CreateConstantBuffer(sizeof(VSInputPerObject), D3D11_USAGE_DYNAMIC,
		D3D11_CPU_ACCESS_WRITE);

	return (m_pVSPerObjectBuffer != nullptr);
}

bool CRenderShader_Model::CreateGSConstantBuffer()
{
	m_pGSPerObjectBuffer = g_GlobalMgr.CreateConstantBuffer(sizeof(GSInputPerObject), D3D11_USAGE_DYNAMIC,
		D3D11_CPU_ACCESS_WRITE);

	return (m_pGSPerObjectBuffer != nullptr);
}

bool CRenderShader_Model::CreatePSConstantBuffer()
{
	m_pPSPerObjectBuffer = g_GlobalMgr.CreateConstantBuffer(sizeof(PSInputPerObject), D3D11_USAGE_DYNAMIC,
		D3D11_CPU_ACCESS_WRITE);

	return (m_pPSPerObjectBuffer != nullptr);
}

bool CRenderShader_Model::Validate(uint32 dwFlags)
{
	return g_RenderShaderMgr.ValidateCurrentConstBufferSlotVS(CBS_3D_VS_PerFrame, CBI_3D_VS_PerFrame) &&
		g_RenderShaderMgr.ValidateCurrentConstBufferSlotPS(CBS_3D_PS_PerFrame, CBI_3D_PS_PerFrame) &&
		g_RenderShaderMgr.ValidateCurrentConstBufferSlotVS(CBS_3D_VS_Model, CBI_3D_VS_Model) &&
#ifdef MODEL_PIECELESS_RENDERING
		g_RenderShaderMgr.ValidateCurrentConstBufferSlotGS(CBS_3D_GS_Model, CBI_3D_GS_Model) &&
#endif
		g_RenderShaderMgr.ValidateCurrentConstBufferSlotPS(CBS_3D_PS_Model, CBI_3D_PS_Model);
}

void CRenderShader_Model::Render(uint32 dwIndexCount, uint32 dwStartIndex)
{
	g_RenderShaderMgr.SetInputLayout(SHADER_Model, m_pLayout);
	g_RenderShaderMgr.SetShaderVS(SHADER_Model, m_pVertexShader);
#ifdef MODEL_PIECELESS_RENDERING
	g_RenderShaderMgr.SetShaderGS(SHADER_Model, m_pGeometryShader);
#else
	g_RenderShaderMgr.SetShaderGS(SHADER_Model, nullptr);
#endif
	g_RenderShaderMgr.SetShaderPS(SHADER_Model, m_pPixelShader);

	if (!Validate(0))
	{
		AddDebugMessage(0, g_szValidationError, m_szName);
		return;
	}

	g_D3DDevice.GetDeviceContext()->DrawIndexed(dwIndexCount, dwStartIndex, 0);
}
