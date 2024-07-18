#ifndef __D3D_SHADER_OPTIMIZED_SURFACE_H__
#define __D3D_SHADER_OPTIMIZED_SURFACE_H__

#ifndef __D3D11_H__
#include <d3d11_1.h>
#define __D3D11_H__
#endif

#ifndef __DIRECT_X_MATH_H__
#include <DirectXMath.h>
#define __DIRECT_X_MATH_H__
#endif

#include "d3d_shader_base.h"
#include "rendershadermgr.h"
#include "d3d_utils.h"

#define OPTIMIZED_SURFACE_BATCH_SIZE	128

class CRenderShader_OptimizedSurface : public CRenderShader_Base
{

public:

	XM_ALIGNED_STRUCT(16) struct PSInputPerObject
	{	
		void Init(DirectX::XMFLOAT3* pTransparentColor, float fSetAlpha, DirectX::XMFLOAT3* pBaseColor)
		{
			m_vTransparentColorAndAlpha =
			{
				pTransparentColor->x,
				pTransparentColor->y,
				pTransparentColor->z,
				fSetAlpha
			};

			m_vBaseColor =
			{
				pBaseColor->x, pBaseColor->y, pBaseColor->z
			};
		}

		DirectX::XMFLOAT4	m_vTransparentColorAndAlpha;
		DirectX::XMFLOAT3A	m_vBaseColor;
	};

	CRenderShader_OptimizedSurface() : CRenderShader_Base() 
	{ 
		m_szName = "OptimizedSurface";
		
		m_pPSPerObjectBuffer = nullptr;
	}

	virtual ~CRenderShader_OptimizedSurface();

	ID3D11Buffer*	GetPSPerObjectBuffer() { return m_pPSPerObjectBuffer; }

	virtual bool	Init(ID3D11VertexShader* pVertexShader, ID3D11PixelShader* pPixelShader, ID3D10Blob* pVSBlob);

	bool	SetPerObjectParams(ID3D11ShaderResourceView* pMainTexture, DirectX::XMFLOAT3* pTransparentColor, float fSetAlpha,
		DirectX::XMFLOAT3* pBaseColor);

	virtual bool	Validate(uint32 dwFlags);

	void	Render();

	static const RenderShader	m_eRenderShader = SHADER_OptimizedSurface;

private:

	bool	CreateInputLayout(void* pVSCode, uint32 dwCodeSize);
	bool	CreatePSConstantBuffer();

	ID3D11Buffer* m_pPSPerObjectBuffer;
};

class CRenderShader_OptimizedSurfaceBatch : public CRenderShader_Base
{

public:

	struct PSPerObjectParams
	{
		PSPerObjectParams()
		{
			m_vTransparentColor = { };
			m_fAlpha = 0.0f;
			m_vBaseColor = { };
			m_dwTextureIndex = 0;
		}

		DirectX::XMFLOAT3	m_vTransparentColor;
		float				m_fAlpha;
		DirectX::XMFLOAT3	m_vBaseColor;

		uint32	m_dwTextureIndex;
	};

	XM_ALIGNED_STRUCT(16) struct OptimizedSurface_PS
	{
		void Init(PSPerObjectParams& params)
		{
			m_vTransparentColorAndAlpha =
			{
				params.m_vTransparentColor.x,
				params.m_vTransparentColor.y,
				params.m_vTransparentColor.z,
				params.m_fAlpha
			};

			m_vBaseColorAndTextureIndex.x = params.m_vBaseColor.x;
			m_vBaseColorAndTextureIndex.y = params.m_vBaseColor.y;
			m_vBaseColorAndTextureIndex.z = params.m_vBaseColor.z;
			*(uint32*)&m_vBaseColorAndTextureIndex.w = params.m_dwTextureIndex;
		}

		DirectX::XMFLOAT4	m_vTransparentColorAndAlpha;
		DirectX::XMFLOAT4A	m_vBaseColorAndTextureIndex;
	};

	XM_ALIGNED_STRUCT(16) struct PSInputPerObject
	{
		void InitAll(PSPerObjectParams* pParams, uint32 dwCount)
		{
			for (uint32 i = 0; i < dwCount; i++)
				m_aOptimizedSurface[i].Init(pParams[i]);
		}
		
		void InitSingle(uint32 dwIndex, PSPerObjectParams* pParams)
		{
			m_aOptimizedSurface[dwIndex].Init(pParams[dwIndex]);
		}

		OptimizedSurface_PS	m_aOptimizedSurface[OPTIMIZED_SURFACE_BATCH_SIZE];
	};

	CRenderShader_OptimizedSurfaceBatch() : CRenderShader_Base()
	{
		m_szName = "OptimizedSurfaceBatch";

		m_pPSPerObjectBuffer = nullptr;
	}

	virtual ~CRenderShader_OptimizedSurfaceBatch();

	ID3D11Buffer* GetPSPerObjectBuffer() { return m_pPSPerObjectBuffer; }

	virtual bool	Init(ID3D11VertexShader* pVertexShader, ID3D11PixelShader* pPixelShader, ID3D10Blob* pVSBlob);

	bool	SetPerObjectParams(ID3D11ShaderResourceView** ppTextures, uint32 dwTextureCount, 
		PSPerObjectParams* pParams, uint32 dwSurfaceCount);

	virtual bool	Validate(uint32 dwFlags);

	void	Render(uint32 dwIndexCount, uint32 dwStartIndex);

	static const RenderShader	m_eRenderShader = SHADER_OptimizedSurfaceBatch;

private:

	bool	CreateInputLayout(void* pVSCode, uint32 dwCodeSize);
	bool	CreatePSConstantBuffer();

	ID3D11Buffer* m_pPSPerObjectBuffer;
};

#endif