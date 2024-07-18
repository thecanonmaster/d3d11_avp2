#include "pch.h"

#include "d3d_shader_bloom.h"
#include "d3d_device.h"
#include "common_stuff.h"
#include "rendershadermgr.h"
#include "d3d_vertextypes.h"
#include "globalmgr.h"

CRenderShader_BloomExtract::~CRenderShader_BloomExtract()
{
	RELEASE_INTERFACE(m_pPSPerFrameBuffer, g_szFreeError_PS_PO_CB, m_szName);
}

bool CRenderShader_BloomExtract::CreatePSConstantBuffer()
{
	m_pPSPerFrameBuffer = g_GlobalMgr.CreateConstantBuffer(sizeof(PSInputPerFrame), D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
	return (m_pPSPerFrameBuffer != nullptr);
}

bool CRenderShader_BloomExtract::Init(ID3D11VertexShader* pVertexShader, ID3D11PixelShader* pPixelShader, ID3D10Blob* pVSBlob)
{
	return CRenderShader_Base::Init(pVertexShader, pPixelShader, pVSBlob) && CreatePSConstantBuffer();
}

bool CRenderShader_BloomExtract::SetPerFrameParams(float fBloomThreshold, float fBaseSaturation, float fBloomSaturation,
	float fBaseIntensity, float fBloomIntensity)
{
	auto lambdaUpdatePS = [fBloomThreshold, fBaseSaturation, fBloomSaturation, fBaseIntensity, 
		fBloomIntensity](D3D11_MAPPED_SUBRESOURCE* pSubResource)
		{
			PSInputPerFrame* pData = (PSInputPerFrame*)pSubResource->pData;
			pData->Init(fBloomThreshold, fBaseSaturation, fBloomSaturation, fBaseIntensity, fBloomIntensity);
		};

	if (!g_GlobalMgr.UpdateGenericBufferEx(m_pPSPerFrameBuffer, D3D11_MAP_WRITE_DISCARD, lambdaUpdatePS))
		return false;

	g_RenderShaderMgr.SetConstantBuffersPS(CBS_3D_PS_Bloom, 1, CBI_3D_PS_Bloom, &m_pPSPerFrameBuffer);

	return true;
}

bool CRenderShader_BloomExtract::SetPerObjectParams(ID3D11ShaderResourceView* pMainTexture)
{
	g_RenderShaderMgr.SetShaderResourcePS(SRS_PS_Primary, pMainTexture);
	return true;
}

bool CRenderShader_BloomBase::Validate(uint32 dwFlags)
{
	return g_RenderShaderMgr.ValidateCurrentConstBufferSlotPS(CBS_3D_PS_Bloom, CBI_3D_PS_Bloom);
}

void CRenderShader_BloomExtract::Render()
{
	g_RenderShaderMgr.SetInputLayout(SHADER_BloomExtract, nullptr);
	g_RenderShaderMgr.SetShaderVS(SHADER_BloomExtract, m_pVertexShader);
	g_RenderShaderMgr.SetShaderGS(SHADER_BloomExtract, nullptr);
	g_RenderShaderMgr.SetShaderPS(SHADER_BloomExtract, m_pPixelShader);

	if (!Validate(0))
	{
		AddDebugMessage(0, g_szValidationError, m_szName);
		return;
	}

	g_D3DDevice.GetDeviceContext()->Draw(BLOOM_VERTICES, 0);
}

CRenderShader_BloomBlur::~CRenderShader_BloomBlur()
{
	RELEASE_INTERFACE(m_pPSPerObjectBuffer, g_szFreeError_PS_PO_CB, m_szName);
}

bool CRenderShader_BloomBlur::CreatePSConstantBuffer()
{
	m_pPSPerObjectBuffer = g_GlobalMgr.CreateConstantBuffer(sizeof(PSInputPerObject), D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
	return (m_pPSPerObjectBuffer != nullptr);
}

bool CRenderShader_BloomBlur::Init(ID3D11VertexShader* pVertexShader, ID3D11PixelShader* pPixelShader, ID3D10Blob* pVSBlob)
{
	return CRenderShader_Base::Init(pVertexShader, pPixelShader, pVSBlob) && CreatePSConstantBuffer();
}

bool CRenderShader_BloomBlur::SetPerObjectParams(ID3D11ShaderResourceView* pMainTexture, DirectX::XMFLOAT4* pSampleWeights, 
	DirectX::XMFLOAT4* pSampleOffsets)
{
	auto lambdaUpdatePS = [ pSampleWeights, pSampleOffsets](D3D11_MAPPED_SUBRESOURCE* pSubResource)
		{
			PSInputPerObject* pData = (PSInputPerObject*)pSubResource->pData;
			pData->Init(pSampleWeights, pSampleOffsets);
		};

	if (!g_GlobalMgr.UpdateGenericBufferEx(m_pPSPerObjectBuffer, D3D11_MAP_WRITE_DISCARD, lambdaUpdatePS))
		return false;

	g_RenderShaderMgr.SetConstantBuffersPS(CBS_3D_PS_Bloom, 1, CBI_3D_PS_Bloom, &m_pPSPerObjectBuffer);
	g_RenderShaderMgr.SetShaderResourcePS(SRS_PS_Primary, pMainTexture);

	return true;
}

void CRenderShader_BloomBlur::Render()
{
	g_RenderShaderMgr.SetInputLayout(SHADER_BloomBlur, nullptr);
	g_RenderShaderMgr.SetShaderVS(SHADER_BloomBlur, m_pVertexShader);
	g_RenderShaderMgr.SetShaderGS(SHADER_BloomBlur, nullptr);
	g_RenderShaderMgr.SetShaderPS(SHADER_BloomBlur, m_pPixelShader);

	if (!Validate(0))
	{
		AddDebugMessage(0, g_szValidationError, m_szName);
		return;
	}

	g_D3DDevice.GetDeviceContext()->Draw(BLOOM_VERTICES, 0);
}

bool CRenderShader_BloomCombine::Init(ID3D11VertexShader* pVertexShader, ID3D11PixelShader* pPixelShader, ID3D10Blob* pVSBlob)
{
	return CRenderShader_Base::Init(pVertexShader, pPixelShader, pVSBlob);
}

bool CRenderShader_BloomCombine::SetPerObjectParams(ID3D11ShaderResourceView* pMainTexture, ID3D11ShaderResourceView* pBloomTexture, 
	ID3D11Buffer* pPerFrameBuffer)
{
	g_RenderShaderMgr.SetConstantBuffersPS(CBS_3D_PS_Bloom, 1, CBI_3D_PS_Bloom, &pPerFrameBuffer);
	g_RenderShaderMgr.SetShaderResourcePS(SRS_PS_Primary, pMainTexture);
	g_RenderShaderMgr.SetShaderResourcePS(SRS_PS_Other, pBloomTexture);

	return true;
}

void CRenderShader_BloomCombine::Render()
{
	g_RenderShaderMgr.SetInputLayout(SHADER_BloomCombine, nullptr);
	g_RenderShaderMgr.SetShaderVS(SHADER_BloomCombine, m_pVertexShader);
	g_RenderShaderMgr.SetShaderGS(SHADER_BloomCombine, nullptr);
	g_RenderShaderMgr.SetShaderPS(SHADER_BloomCombine, m_pPixelShader);

	if (!Validate(0))
	{
		AddDebugMessage(0, g_szValidationError, m_szName);
		return;
	}

	g_D3DDevice.GetDeviceContext()->Draw(BLOOM_VERTICES, 0);
}
