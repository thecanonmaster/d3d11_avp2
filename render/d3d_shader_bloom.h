#ifndef __D3D_SHADER_BLOOM_H__
#define __D3D_SHADER_BLOOM_H__

#ifndef __D3D11_H__
#include <d3d11_1.h>
#define __D3D11_H__
#endif

#include "d3d_shader_base.h"
#include "globalmgr.h"

#define BLOOM_VERTICES				3
#define MAX_BLOOM_SAMPLES			15

class CRenderShader_BloomBase : public CRenderShader_Base
{

public:

	XM_ALIGNED_STRUCT(16) struct PSInputPerFrame
	{
		void Init(float fThreshold, float fBaseSaturation, float fBloomSaturation, float fBaseIntensity, 
			float fBloomIntensity)
		{
			m_vThreshold =
			{
				fThreshold, fThreshold, fThreshold, fThreshold
			};

			m_vBaseAndBloomSaturation = { fBaseSaturation, fBloomSaturation };

			m_vBaseIntensity =
			{
				fBaseIntensity, fBaseIntensity, fBaseIntensity, fBaseIntensity
			};

			m_vBloomIntensity =
			{
				fBloomIntensity, fBloomIntensity, fBloomIntensity, fBloomIntensity
			};
		}

		DirectX::XMFLOAT4	m_vThreshold;
		DirectX::XMFLOAT2A	m_vBaseAndBloomSaturation;
		DirectX::XMFLOAT4	m_vBaseIntensity;
		DirectX::XMFLOAT4	m_vBloomIntensity;
	};

	CRenderShader_BloomBase() : CRenderShader_Base() { };

	virtual bool	Validate(uint32 dwFlags);
};

class CRenderShader_BloomExtract : public CRenderShader_BloomBase
{

public:

	CRenderShader_BloomExtract() : CRenderShader_BloomBase()
	{ 
		m_szName = "BloomExtract";
		m_pPSPerFrameBuffer = nullptr;
	}

	virtual ~CRenderShader_BloomExtract();

	ID3D11Buffer* GetPSPerFrameBuffer() { return m_pPSPerFrameBuffer; }

	virtual bool	Init(ID3D11VertexShader* pVertexShader, ID3D11PixelShader* pPixelShader, ID3D10Blob* pVSBlob);

	bool	SetPerFrameParams(float fBloomThreshold, float fBaseSaturation, float fBloomSaturation,
		float fBaseIntensity, float fBloomIntensity);

	bool	SetPerObjectParams(ID3D11ShaderResourceView* pMainTexture);

	void	Render();

	static const RenderShader	m_eRenderShader = SHADER_BloomExtract;

private:

	bool	CreatePSConstantBuffer();

	ID3D11Buffer* m_pPSPerFrameBuffer;
};

class CRenderShader_BloomBlur : public CRenderShader_BloomBase
{

public:

	XM_ALIGNED_STRUCT(16) struct PSInputPerObject
	{
		void Init(DirectX::XMFLOAT4* pSampleWeights, DirectX::XMFLOAT4* pSampleOffsets)
		{
			for (uint32 i = 0; i < MAX_BLOOM_SAMPLES; i++)
			{
				m_avSampleWeight[i] = pSampleWeights[i];
				m_avSampleOffset[i] = pSampleOffsets[i];
			}
		}

		DirectX::XMFLOAT4	m_avSampleWeight[MAX_BLOOM_SAMPLES];
		DirectX::XMFLOAT4	m_avSampleOffset[MAX_BLOOM_SAMPLES];
	};

	CRenderShader_BloomBlur() : CRenderShader_BloomBase()
	{
		m_szName = "BloomBlur";
		m_pPSPerObjectBuffer = nullptr;
	}

	virtual ~CRenderShader_BloomBlur();

	virtual bool	Init(ID3D11VertexShader* pVertexShader, ID3D11PixelShader* pPixelShader, ID3D10Blob* pVSBlob);

	bool	SetPerObjectParams(ID3D11ShaderResourceView* pMainTexture, DirectX::XMFLOAT4* pSampleWeights, 
		DirectX::XMFLOAT4* pSampleOffsets);

	void	Render();

	static const RenderShader	m_eRenderShader = SHADER_BloomBlur;

private:

	bool	CreatePSConstantBuffer();

	ID3D11Buffer*	m_pPSPerObjectBuffer;
};

class CRenderShader_BloomCombine : public CRenderShader_BloomBase
{

public:

	CRenderShader_BloomCombine() : CRenderShader_BloomBase()
	{
		m_szName = "BloomCombine";
	}

	virtual bool	Init(ID3D11VertexShader* pVertexShader, ID3D11PixelShader* pPixelShader, ID3D10Blob* pVSBlob);

	bool	SetPerObjectParams(ID3D11ShaderResourceView* pMainTexture, ID3D11ShaderResourceView* pBloomTexture, 
		ID3D11Buffer* pPerFrameBuffer);

	void	Render();

	static const RenderShader	m_eRenderShader = SHADER_BloomCombine;
};

#endif