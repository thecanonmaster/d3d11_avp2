#ifndef __D3D_SHADER_CANVAS_H__
#define __D3D_SHADER_CANVAS_H__

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

#define CANVAS_BATCH_SIZE	96

class CRenderShader_Canvas : public CRenderShader_Base
{

public:

	XM_ALIGNED_STRUCT(16) struct VPSInputPerObject
	{
		void Init(uint32 dwMode, DirectX::XMFLOAT3* pModeColor, XMFloat4x4Trinity* pTransforms)
		{
			m_mWorld = pTransforms->m_mWorld;
			m_mWorldView = pTransforms->m_mWorldView;
			m_mWorldViewProj = pTransforms->m_mWorldViewProj;

			m_vModeColorAndMode.x = pModeColor->x;
			m_vModeColorAndMode.y = pModeColor->y;
			m_vModeColorAndMode.z = pModeColor->z;
			*(uint32*)&m_vModeColorAndMode.w = dwMode;
		}

		DirectX::XMFLOAT4X4	m_mWorld;
		DirectX::XMFLOAT4X4	m_mWorldView;
		DirectX::XMFLOAT4X4	m_mWorldViewProj;
		DirectX::XMFLOAT4	m_vModeColorAndMode;
	};

	CRenderShader_Canvas() : CRenderShader_Base()
	{
		m_szName = "Canvas";

		m_pVPSPerObjectBuffer = nullptr;
	}

	virtual ~CRenderShader_Canvas();

	ID3D11Buffer*	GetVPSPerObjectBuffer() { return m_pVPSPerObjectBuffer; }

	virtual bool	Init(ID3D11VertexShader* pVertexShader, ID3D11PixelShader* pPixelShader, ID3D10Blob* pVSBlob);

	bool	SetPerObjectParams(ID3D11ShaderResourceView* pMainTexture, uint32 dwRenderMode, DirectX::XMFLOAT3* pModeColor, 
		XMFloat4x4Trinity* pTransforms);

	virtual bool	Validate(uint32 dwFlags);

	void	Render(uint32 dwVertexCount);

	static const RenderShader	m_eRenderShader = SHADER_Canvas;

private:

	bool	CreateInputLayout(void* pVSCode, uint32 dwCodeSize);
	bool	CreateVPSConstantBuffer();

	ID3D11Buffer*	m_pVPSPerObjectBuffer;
};

class CRenderShader_CanvasBatch : public CRenderShader_Base
{

public:

	struct VPSPerObjectParams
	{
		VPSPerObjectParams()
		{
			m_dwMode = 0;
			m_vModeColor = { };
			m_dwTextureIndex = 0;
		}

		uint32				m_dwMode;
		DirectX::XMFLOAT3	m_vModeColor;

		uint32	m_dwTextureIndex;
	};

	XM_ALIGNED_STRUCT(16) struct Canvas_VPS
	{
		void Init(VPSPerObjectParams& params)
		{
			m_vModeAndTextureIndex = { params.m_dwMode, params.m_dwTextureIndex, 0, 0 };
			m_vModeColor = { params.m_vModeColor.x, params.m_vModeColor.y, params.m_vModeColor.z };
		}

		DirectX::XMUINT4	m_vModeAndTextureIndex;
		DirectX::XMFLOAT3A	m_vModeColor;
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
				m_aCanvas[i].Init(pParams[i]);
		}

		DirectX::XMFLOAT4X4	m_mWorldView;
		DirectX::XMFLOAT4X4	m_mWorldViewProj;
		DirectX::XMFLOAT4X4	m_mWorldViewRC;
		DirectX::XMFLOAT4X4	m_mWorldViewProjRC;
		Canvas_VPS			m_aCanvas[CANVAS_BATCH_SIZE];
	};

	CRenderShader_CanvasBatch() : CRenderShader_Base()
	{
		m_szName = "CanvasBatch";

		m_pVPSPerObjectBuffer = nullptr;
	}

	virtual ~CRenderShader_CanvasBatch();

	ID3D11Buffer* GetVPSPerObjectBuffer() { return m_pVPSPerObjectBuffer; }

	virtual bool	Init(ID3D11VertexShader* pVertexShader, ID3D11PixelShader* pPixelShader, ID3D10Blob* pVSBlob);

	bool	SetPerObjectParams(ID3D11ShaderResourceView** ppTextures, uint32 dwTextureCount, XMFloat4x4Trinity* pTransforms, 
		XMFloat4x4Trinity* pTransformsRC, VPSPerObjectParams* pParams, uint32 dwCanvasCount);

	virtual bool	Validate(uint32 dwFlags);

	void	Render(uint32 dwVertexCount, uint32 dwStartVertex);

	static const RenderShader	m_eRenderShader = SHADER_CanvasBatch;

private:

	bool	CreateInputLayout(void* pVSCode, uint32 dwCodeSize);
	bool	CreateVPSConstantBuffer();

	ID3D11Buffer* m_pVPSPerObjectBuffer;
};

#endif