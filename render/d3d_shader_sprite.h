#ifndef __D3D_SHADER_SPRITE_H__
#define __D3D_SHADER_SPRITE_H__

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

#define SPRITE_BATCH_SIZE	64

class CRenderShader_Sprite : public CRenderShader_Base
{

public:

	XM_ALIGNED_STRUCT(16) struct VPSInputPerObject
	{
		void Init(uint32 dwMode, DirectX::XMFLOAT3* pModeColor, DirectX::XMFLOAT4* pDiffuseColor, 
			XMFloat4x4Trinity* pTransforms)
		{
			m_mWorld = pTransforms->m_mWorld;
			m_mWorldView = pTransforms->m_mWorldView;
			m_mWorldViewProj = pTransforms->m_mWorldViewProj;

			m_vModeColorAndMode.x = pModeColor->x;
			m_vModeColorAndMode.y = pModeColor->y;
			m_vModeColorAndMode.z = pModeColor->z;
			*(uint32*)&m_vModeColorAndMode.w = dwMode;

			m_vDiffuseColor = *pDiffuseColor;
		}

		DirectX::XMFLOAT4X4	m_mWorld;
		DirectX::XMFLOAT4X4	m_mWorldView;
		DirectX::XMFLOAT4X4	m_mWorldViewProj;
		DirectX::XMFLOAT4	m_vModeColorAndMode;
		DirectX::XMFLOAT4	m_vDiffuseColor;
	};

	CRenderShader_Sprite() : CRenderShader_Base()
	{
		m_szName = "Sprite";

		m_pVPSPerObjectBuffer = nullptr;
	}

	virtual ~CRenderShader_Sprite();

	ID3D11Buffer*	GetVPSPerObjectBuffer() { return m_pVPSPerObjectBuffer; }

	virtual bool	Init(ID3D11VertexShader* pVertexShader, ID3D11PixelShader* pPixelShader, ID3D10Blob* pVSBlob);

	bool	SetPerObjectParams(ID3D11ShaderResourceView* pMainTexture, uint32 dwMode, DirectX::XMFLOAT3* pModeColor, 
		DirectX::XMFLOAT4* pDiffuseColor, XMFloat4x4Trinity* pTransforms);

	virtual bool	Validate(uint32 dwFlags);

	void	Render(uint32 dwIndexCount);

	static const RenderShader	m_eRenderShader = SHADER_Sprite;

private:

	bool	CreateInputLayout(void* pVSCode, uint32 dwCodeSize);
	bool	CreateVPSConstantBuffer();

	ID3D11Buffer*	m_pVPSPerObjectBuffer;
};

class CRenderShader_SpriteBatch : public CRenderShader_Base
{

public:

	struct VPSPerObjectParams
	{
		VPSPerObjectParams()
		{
			m_dwMode = 0;
			m_vModeColor = { };
			m_vDiffuseColor = { };
			m_dwTextureIndex = 0;
		}

		uint32				m_dwMode;
		DirectX::XMFLOAT3	m_vModeColor;
		DirectX::XMFLOAT4	m_vDiffuseColor;

		uint32	m_dwTextureIndex;
	};

	XM_ALIGNED_STRUCT(16) struct Sprite_VPS
	{
		void Init(VPSPerObjectParams& params)
		{
			m_vModeColor =
			{
				params.m_vModeColor.x, params.m_vModeColor.y, params.m_vModeColor.x
			};

			m_vDiffuseColor = params.m_vDiffuseColor;
			m_vModeAndTextureIndex = { params.m_dwMode, params.m_dwTextureIndex, 0, 0 };
		}

		DirectX::XMFLOAT3A	m_vModeColor;
		DirectX::XMFLOAT4	m_vDiffuseColor;
		DirectX::XMUINT4	m_vModeAndTextureIndex;
	};

	XM_ALIGNED_STRUCT(16) struct VPSInputPerObject
	{
		void Init(XMFloat4x4Trinity* pTransforms, XMFloat4x4Trinity* pTransformsRC, VPSPerObjectParams* pParams, 
			uint32 dwCount)
		{
			m_mWorldView = pTransforms->m_mWorldView;
			m_mWorldViewProj = pTransforms->m_mWorldViewProj;
			m_mWorldViewRC = pTransformsRC->m_mWorldView;
			m_mWorldViewProjRC = pTransformsRC->m_mWorldViewProj;
			
			for (uint32 i = 0; i < dwCount; i++)
				m_aSprite[i].Init(pParams[i]);
		};

		DirectX::XMFLOAT4X4	m_mWorldView;
		DirectX::XMFLOAT4X4	m_mWorldViewProj;
		DirectX::XMFLOAT4X4	m_mWorldViewRC;
		DirectX::XMFLOAT4X4	m_mWorldViewProjRC;
		Sprite_VPS			m_aSprite[SPRITE_BATCH_SIZE];
	};

	CRenderShader_SpriteBatch() : CRenderShader_Base()
	{
		m_szName = "SpriteBatch";

		m_pVPSPerObjectBuffer = nullptr;
	}

	virtual ~CRenderShader_SpriteBatch();

	ID3D11Buffer* GetVPSPerObjectBuffer() { return m_pVPSPerObjectBuffer; }

	virtual bool	Init(ID3D11VertexShader* pVertexShader, ID3D11PixelShader* pPixelShader, ID3D10Blob* pVSBlob);

	bool	SetPerObjectParams(ID3D11ShaderResourceView** ppTextures, uint32 dwTextureCount, XMFloat4x4Trinity* pTransforms, 
		XMFloat4x4Trinity* pTransformsRC, VPSPerObjectParams* pParams, uint32 dwSpriteCount);

	virtual bool	Validate(uint32 dwFlags);

	void	Render(uint32 dwIndexCount, uint32 dwStartIndex);

	static const RenderShader	m_eRenderShader = SHADER_SpriteBatch;

private:

	bool	CreateInputLayout(void* pVSCode, uint32 dwCodeSize);
	bool	CreateVPSConstantBuffer();

	ID3D11Buffer* m_pVPSPerObjectBuffer;
};

#endif