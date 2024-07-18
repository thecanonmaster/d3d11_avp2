#include "pch.h"

#include "d3d_shader_sprite.h"
#include "d3d_device.h"
#include "common_stuff.h"
#include "rendershadermgr.h"
#include "d3d_vertextypes.h"
#include "draw_sprite.h"
#include "globalmgr.h"

CRenderShader_Sprite::~CRenderShader_Sprite()
{
	RELEASE_INTERFACE(m_pVPSPerObjectBuffer, g_szFreeError_VPS_PO_CB, m_szName);
}

bool CRenderShader_Sprite::Init(ID3D11VertexShader* pVertexShader, ID3D11PixelShader* pPixelShader, ID3D10Blob* pVSBlob)
{
	return CRenderShader_Base::Init(pVertexShader, pPixelShader, pVSBlob) &&
		CreateInputLayout(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize()) &&
		CreateVPSConstantBuffer();
}

bool CRenderShader_Sprite::SetPerObjectParams(ID3D11ShaderResourceView* pMainTexture, uint32 dwMode,
	DirectX::XMFLOAT3* pModeColor, DirectX::XMFLOAT4* pDiffuseColor, XMFloat4x4Trinity* pTransforms)
{
	auto lambdaUpdateVS = [dwMode, pModeColor, pDiffuseColor, pTransforms](D3D11_MAPPED_SUBRESOURCE* pSubResource)
		{
			VPSInputPerObject* pData = (VPSInputPerObject*)pSubResource->pData;
			pData->Init(dwMode, pModeColor, pDiffuseColor, pTransforms);
		};

	if (!g_GlobalMgr.UpdateGenericBufferEx(m_pVPSPerObjectBuffer, D3D11_MAP_WRITE_DISCARD, lambdaUpdateVS))
		return false;

	g_RenderShaderMgr.SetConstantBuffersVS(CBS_3D_VS_Sprite, 1, CBI_3D_VS_Sprite, &m_pVPSPerObjectBuffer);
	g_RenderShaderMgr.SetConstantBuffersPS(CBS_3D_PS_Sprite, 1, CBI_3D_PS_Sprite, &m_pVPSPerObjectBuffer);

	if (pMainTexture != nullptr)
		g_RenderShaderMgr.SetShaderResourcePS(SRS_PS_Primary, pMainTexture);

	return true;
}

bool CRenderShader_Sprite::CreateInputLayout(void* pVSCode, uint32 dwCodeSize)
{
	HRESULT hResult = g_D3DDevice.GetDevice()->CreateInputLayout(VertexTypes::PositionTexture::c_aInputElements,
		VertexTypes::PositionTexture::c_dwInputElements, pVSCode, dwCodeSize, &m_pLayout);

	if (FAILED(hResult))
		return false;

	return true;
}

bool CRenderShader_Sprite::CreateVPSConstantBuffer()
{
	m_pVPSPerObjectBuffer = g_GlobalMgr.CreateConstantBuffer(sizeof(VPSInputPerObject), D3D11_USAGE_DYNAMIC, 
		D3D11_CPU_ACCESS_WRITE);
	return (m_pVPSPerObjectBuffer != nullptr);
}

bool CRenderShader_Sprite::Validate(uint32 dwFlags)
{
	return g_RenderShaderMgr.ValidateCurrentConstBufferSlotVS(CBS_3D_VS_PerFrame, CBI_3D_VS_PerFrame) &&
		g_RenderShaderMgr.ValidateCurrentConstBufferSlotPS(CBS_3D_PS_PerFrame, CBI_3D_PS_PerFrame) &&
		g_RenderShaderMgr.ValidateCurrentConstBufferSlotVS(CBS_3D_VS_Sprite, CBI_3D_VS_Sprite) && 
		g_RenderShaderMgr.ValidateCurrentConstBufferSlotPS(CBS_3D_PS_Sprite, CBI_3D_PS_Sprite);
}

void CRenderShader_Sprite::Render(uint32 dwIndexCount)
{
	g_RenderShaderMgr.SetInputLayout(SHADER_Sprite, m_pLayout);
	g_RenderShaderMgr.SetShaderVS(SHADER_Sprite, m_pVertexShader);
	g_RenderShaderMgr.SetShaderGS(SHADER_Sprite, nullptr);
	g_RenderShaderMgr.SetShaderPS(SHADER_Sprite, m_pPixelShader);

	if (!Validate(0))
	{
		AddDebugMessage(0, g_szValidationError, m_szName);
		return;
	}

	g_D3DDevice.GetDeviceContext()->DrawIndexed(dwIndexCount, 0, 0);
}


CRenderShader_SpriteBatch::~CRenderShader_SpriteBatch()
{
	RELEASE_INTERFACE(m_pVPSPerObjectBuffer, g_szFreeError_VPS_PO_CB, m_szName);
}

bool CRenderShader_SpriteBatch::Init(ID3D11VertexShader* pVertexShader, ID3D11PixelShader* pPixelShader, ID3D10Blob* pVSBlob)
{
	return CRenderShader_Base::Init(pVertexShader, pPixelShader, pVSBlob) &&
		CreateInputLayout(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize()) &&
		CreateVPSConstantBuffer();
}

bool CRenderShader_SpriteBatch::SetPerObjectParams(ID3D11ShaderResourceView** ppTextures, uint32 dwTextureCount, 
	XMFloat4x4Trinity* pTransforms, XMFloat4x4Trinity* pTransformsRC, VPSPerObjectParams* pParams, uint32 dwSpriteCount)
{
	auto lambdaUpdateVS = [pTransforms, pTransformsRC, pParams, dwSpriteCount](D3D11_MAPPED_SUBRESOURCE* pSubResource)
		{
			VPSInputPerObject* pData = (VPSInputPerObject*)pSubResource->pData;
			pData->Init(pTransforms, pTransformsRC, pParams, dwSpriteCount);
		};

	if (!g_GlobalMgr.UpdateGenericBufferEx(m_pVPSPerObjectBuffer, D3D11_MAP_WRITE_DISCARD, lambdaUpdateVS))
		return false;

	g_RenderShaderMgr.SetConstantBuffersVS(CBS_3D_VS_Sprite, 1, CBI_3D_VS_Sprite, &m_pVPSPerObjectBuffer);
	g_RenderShaderMgr.SetConstantBuffersPS(CBS_3D_PS_Sprite, 1, CBI_3D_PS_Sprite, &m_pVPSPerObjectBuffer);

	for (uint32 i = 0; i < dwTextureCount; i++)
		g_RenderShaderMgr.SetShaderResourcePS(SRS_PS_Other + i, ppTextures[i]); 

	return true;
}

bool CRenderShader_SpriteBatch::CreateInputLayout(void* pVSCode, uint32 dwCodeSize)
{
	HRESULT hResult = g_D3DDevice.GetDevice()->CreateInputLayout(VertexTypes::PositionTextureIndex::c_aInputElements,
		VertexTypes::PositionTextureIndex::c_dwInputElements, pVSCode, dwCodeSize, &m_pLayout);

	if (FAILED(hResult))
		return false;

	return true;
}

bool CRenderShader_SpriteBatch::CreateVPSConstantBuffer()
{
	m_pVPSPerObjectBuffer = g_GlobalMgr.CreateConstantBuffer(sizeof(VPSInputPerObject), D3D11_USAGE_DYNAMIC,
		D3D11_CPU_ACCESS_WRITE);
	return (m_pVPSPerObjectBuffer != nullptr);
}

bool CRenderShader_SpriteBatch::Validate(uint32 dwFlags)
{
	return g_RenderShaderMgr.ValidateCurrentConstBufferSlotVS(CBS_3D_VS_PerFrame, CBI_3D_VS_PerFrame) &&
		g_RenderShaderMgr.ValidateCurrentConstBufferSlotPS(CBS_3D_PS_PerFrame, CBI_3D_PS_PerFrame) &&
		g_RenderShaderMgr.ValidateCurrentConstBufferSlotVS(CBS_3D_VS_Sprite, CBI_3D_VS_Sprite) &&
		g_RenderShaderMgr.ValidateCurrentConstBufferSlotPS(CBS_3D_PS_Sprite, CBI_3D_PS_Sprite);
}

void CRenderShader_SpriteBatch::Render(uint32 dwIndexCount, uint32 dwStartIndex)
{
	g_RenderShaderMgr.SetInputLayout(SHADER_Sprite, m_pLayout);
	g_RenderShaderMgr.SetShaderVS(SHADER_Sprite, m_pVertexShader);
	g_RenderShaderMgr.SetShaderGS(SHADER_Sprite, nullptr);
	g_RenderShaderMgr.SetShaderPS(SHADER_Sprite, m_pPixelShader);

	if (!Validate(0))
	{
		AddDebugMessage(0, g_szValidationError, m_szName);
		return;
	}

	g_D3DDevice.GetDeviceContext()->DrawIndexed(dwIndexCount, dwStartIndex, 0);
}