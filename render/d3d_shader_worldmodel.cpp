#include "pch.h"

#include "d3d_shader_worldmodel.h"
#include "d3d_device.h"
#include "common_stuff.h"
#include "rendershadermgr.h"
#include "d3d_vertextypes.h"
#include "globalmgr.h"

CRenderShader_WorldModel::~CRenderShader_WorldModel()
{
	RELEASE_INTERFACE(m_pVSPerObjectBuffer, g_szFreeError_VS_PO_CB, m_szName);
	RELEASE_INTERFACE(m_pVSPerSubObjectBuffer, g_szFreeError_VS_PSO_CB, m_szName);
	RELEASE_INTERFACE(m_pPSPerSubObjectBuffer, g_szFreeError_PS_PSO_CB, m_szName);
}

bool CRenderShader_WorldModel::Init(ID3D11VertexShader* pVertexShader, ID3D11PixelShader* pPixelShader, ID3D10Blob* pVSBlob)
{
	return CRenderShader_Base::Init(pVertexShader, pPixelShader, pVSBlob) &&
		CreateInputLayout(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize()) &&
		CreateVSConstantBuffers() && CreatePSConstantBuffer();
}

bool CRenderShader_WorldModel::SetPerObjectParams(XMFloat4x4Trinity* pTransforms, DirectX::XMFLOAT4* pDiffuseColor,
	ID3D11ShaderResourceView* pLMVertexData)
{
	auto lambdaUpdateVS = [pTransforms, pDiffuseColor](D3D11_MAPPED_SUBRESOURCE* pSubResource)
		{
			VSInputPerObject* pData = (VSInputPerObject*)pSubResource->pData;
			pData->Init(pTransforms, pDiffuseColor);
		};

	if (!g_GlobalMgr.UpdateGenericBufferEx(m_pVSPerObjectBuffer, D3D11_MAP_WRITE_DISCARD, lambdaUpdateVS))
		return false;

	g_RenderShaderMgr.SetConstantBuffersVS(CBS_3D_VS_WorldModel, 1, CBI_3D_VS_WorldModel, &m_pVSPerObjectBuffer);

	if (pLMVertexData != nullptr)
	{
		g_RenderShaderMgr.SetShaderResourceVS(SRS_VS_LMVertexData, pLMVertexData);
		g_RenderShaderMgr.SetShaderResourcePS(SRS_PS_LMVertexData, pLMVertexData);
	}

	return true;
}

bool CRenderShader_WorldModel::SetPerSubObjectParams(VPSPerSubObjectParams* pParams, uint32 dwLightCount, 
	uint32* pLightIndices, uint32 dwMSLightCount, uint32* pMSLightIndices)
{
	D3D11_MAPPED_SUBRESOURCE subResourceVS;
	if (!g_GlobalMgr.MapGenericBuffer(m_pVSPerSubObjectBuffer, D3D11_MAP_WRITE_DISCARD, &subResourceVS))
		return false;

	D3D11_MAPPED_SUBRESOURCE subResourcePS;
	if (!g_GlobalMgr.MapGenericBuffer(m_pPSPerSubObjectBuffer, D3D11_MAP_WRITE_DISCARD, &subResourcePS))
	{
		g_GlobalMgr.UnmapGenericBuffer(m_pVSPerSubObjectBuffer);
		return false;
	}

	VSInputPerSubObject* pVSData = (VSInputPerSubObject*)subResourceVS.pData;
	PSInputPerSubObject* pPSData = (PSInputPerSubObject*)subResourcePS.pData;
		
	for (uint32 i = 0; i < WORLD_MODEL_TEXTURES_PER_DRAW; i++)
	{	
		pVSData->InitSingle(i, &pParams[i]);
		pPSData->InitSingle(i, &pParams[i]);
		
		if (pParams[i].m_pTexture != nullptr)
			g_RenderShaderMgr.SetShaderResourcePS(SRS_PS_Other + i, pParams[i].m_pTexture);
	}

	pPSData->InitLightIndices(dwLightCount, pLightIndices, dwMSLightCount, pMSLightIndices);

	g_GlobalMgr.UnmapGenericBuffer(m_pVSPerSubObjectBuffer);
	g_GlobalMgr.UnmapGenericBuffer(m_pPSPerSubObjectBuffer);

	g_RenderShaderMgr.SetConstantBuffersVS(CBS_3D_VS_SubWorldModel, 1, CBI_3D_VS_SubWorldModel, &m_pVSPerSubObjectBuffer);
	g_RenderShaderMgr.SetConstantBuffersPS(CBS_3D_PS_SubWorldModel, 1, CBI_3D_PS_SubWorldModel, &m_pPSPerSubObjectBuffer);
	
	return true;
}

bool CRenderShader_WorldModel::CreateInputLayout(void* pVSCode, uint32 dwCodeSize)
{
	HRESULT hResult = g_D3DDevice.GetDevice()->CreateInputLayout(
		VertexTypes::PositionNormalColorTexture3Index4::c_aInputElements,
		VertexTypes::PositionNormalColorTexture3Index4::c_dwInputElements, pVSCode, dwCodeSize, &m_pLayout);

	if (FAILED(hResult))
		return false;

	return true;
}

bool CRenderShader_WorldModel::CreateVSConstantBuffers()
{
	m_pVSPerObjectBuffer = g_GlobalMgr.CreateConstantBuffer(sizeof(VSInputPerObject), D3D11_USAGE_DYNAMIC,
		D3D11_CPU_ACCESS_WRITE);

	m_pVSPerSubObjectBuffer = g_GlobalMgr.CreateConstantBuffer(sizeof(VSInputPerSubObject), D3D11_USAGE_DYNAMIC,
		D3D11_CPU_ACCESS_WRITE);

	return (m_pVSPerObjectBuffer != nullptr && m_pVSPerSubObjectBuffer != nullptr);
}

bool CRenderShader_WorldModel::CreatePSConstantBuffer()
{
	m_pPSPerSubObjectBuffer = g_GlobalMgr.CreateConstantBuffer(sizeof(PSInputPerSubObject), D3D11_USAGE_DYNAMIC,
		D3D11_CPU_ACCESS_WRITE);

	return (m_pPSPerSubObjectBuffer != nullptr);
}

bool CRenderShader_WorldModel::Validate(uint32 dwFlags)
{
	return g_RenderShaderMgr.ValidateCurrentConstBufferSlotVS(CBS_3D_VS_PerFrame, CBI_3D_VS_PerFrame) &&
		g_RenderShaderMgr.ValidateCurrentConstBufferSlotPS(CBS_3D_PS_PerFrame, CBI_3D_PS_PerFrame) &&
		g_RenderShaderMgr.ValidateCurrentConstBufferSlotVS(CBS_3D_VS_WorldModel, CBI_3D_VS_WorldModel) &&
		g_RenderShaderMgr.ValidateCurrentConstBufferSlotVS(CBS_3D_VS_SubWorldModel, CBI_3D_VS_SubWorldModel) &&
		g_RenderShaderMgr.ValidateCurrentConstBufferSlotPS(CBS_3D_PS_SubWorldModel, CBI_3D_PS_SubWorldModel) &&
		g_RenderShaderMgr.ValidateCurrentConstBufferSlotPS(CBS_3D_PS_DynamicLights, CBI_3D_PS_DynamicLights);
}

void CRenderShader_WorldModel::Render(uint32 dwIndexCount, uint32 dwStartIndex)
{
	g_RenderShaderMgr.SetInputLayout(SHADER_WorldModel, m_pLayout);
	g_RenderShaderMgr.SetShaderVS(SHADER_WorldModel, m_pVertexShader);
	g_RenderShaderMgr.SetShaderGS(SHADER_WorldModel, nullptr);
	g_RenderShaderMgr.SetShaderPS(SHADER_WorldModel, m_pPixelShader);

	if (!Validate(0))
	{
		AddDebugMessage(0, g_szValidationError, m_szName);
		return;
	}

	g_D3DDevice.GetDeviceContext()->DrawIndexed(dwIndexCount, dwStartIndex, 0);
}

void CRenderShader_SkyWorldModel::Render(uint32 dwIndexCount, uint32 dwStartIndex)
{
	g_RenderShaderMgr.SetInputLayout(SHADER_SkyWorldModel, m_pLayout);
	g_RenderShaderMgr.SetShaderVS(SHADER_SkyWorldModel, m_pVertexShader);
	g_RenderShaderMgr.SetShaderGS(SHADER_SkyWorldModel, nullptr);
	g_RenderShaderMgr.SetShaderPS(SHADER_SkyWorldModel, m_pPixelShader);

	if (!Validate(0))
	{
		AddDebugMessage(0, g_szValidationError, m_szName);
		return;
	}

	g_D3DDevice.GetDeviceContext()->DrawIndexed(dwIndexCount, dwStartIndex, 0);
}

CRenderShader_SkyPortal::~CRenderShader_SkyPortal()
{
	RELEASE_INTERFACE(m_pVPSPerObjectBuffer, g_szFreeError_VPS_PO_CB, m_szName);
}

bool CRenderShader_SkyPortal::Init(ID3D11VertexShader* pVertexShader, ID3D11PixelShader* pPixelShader, ID3D10Blob* pVSBlob)
{
	return CRenderShader_Base::Init(pVertexShader, pPixelShader, pVSBlob) &&
		CreateInputLayout(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize()) &&
		CreateVPSConstantBuffer();
}

bool CRenderShader_SkyPortal::SetPerObjectParams(ID3D11ShaderResourceView* pMainTexture, XMFloat4x4Trinity* pTransforms,
	uint32 dwScreenWidth, uint32 dwScreenHeight)
{
	auto lambdaUpdateVS = [pTransforms, dwScreenWidth, dwScreenHeight](D3D11_MAPPED_SUBRESOURCE* pSubResource)
		{
			VPSInputPerObject* pData = (VPSInputPerObject*)pSubResource->pData;
			pData->Init(pTransforms, dwScreenWidth, dwScreenHeight);
		};

	if (!g_GlobalMgr.UpdateGenericBufferEx(m_pVPSPerObjectBuffer, D3D11_MAP_WRITE_DISCARD, lambdaUpdateVS))
		return false;

	g_RenderShaderMgr.SetConstantBuffersVS(CBS_3D_VS_WorldModel, 1, CBI_3D_VS_SkyPortal, &m_pVPSPerObjectBuffer);
	g_RenderShaderMgr.SetConstantBuffersPS(CBS_3D_PS_WorldModel, 1, CBI_3D_PS_SkyPortal, &m_pVPSPerObjectBuffer);

	g_RenderShaderMgr.SetShaderResourcePS(SRS_PS_Other, pMainTexture);

	return true;
}

bool CRenderShader_SkyPortal::CreateInputLayout(void* pVSCode, uint32 dwCodeSize)
{
	HRESULT hResult = g_D3DDevice.GetDevice()->CreateInputLayout(VertexTypes::Position::c_aInputElements,
		VertexTypes::Position::c_dwInputElements, pVSCode, dwCodeSize, &m_pLayout);

	if (FAILED(hResult))
		return false;

	return true;
}

bool CRenderShader_SkyPortal::CreateVPSConstantBuffer()
{
	m_pVPSPerObjectBuffer = g_GlobalMgr.CreateConstantBuffer(sizeof(VPSInputPerObject), D3D11_USAGE_DYNAMIC,
		D3D11_CPU_ACCESS_WRITE);

	return (m_pVPSPerObjectBuffer != nullptr);
}

bool CRenderShader_SkyPortal::Validate(uint32 dwFlags)
{
	return g_RenderShaderMgr.ValidateCurrentConstBufferSlotVS(CBS_3D_VS_WorldModel, CBI_3D_VS_SkyPortal) &&
		g_RenderShaderMgr.ValidateCurrentConstBufferSlotPS(CBS_3D_PS_WorldModel, CBI_3D_PS_SkyPortal);
}

void CRenderShader_SkyPortal::Render(uint32 dwIndexCount)
{
	g_RenderShaderMgr.SetInputLayout(SHADER_SkyPortal, m_pLayout);
	g_RenderShaderMgr.SetShaderVS(SHADER_SkyPortal, m_pVertexShader);
	g_RenderShaderMgr.SetShaderGS(SHADER_SkyPortal, nullptr);
	g_RenderShaderMgr.SetShaderPS(SHADER_SkyPortal, m_pPixelShader);

	if (!Validate(0))
	{
		AddDebugMessage(0, g_szValidationError, m_szName);
		return;
	}

	g_D3DDevice.GetDeviceContext()->DrawIndexed(dwIndexCount, 0, 0);
}