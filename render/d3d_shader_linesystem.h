#ifndef __D3D_SHADER_LINE_SYSTEM_H__
#define __D3D_SHADER_LINE_SYSTEM_H__

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

class CRenderShader_LineSystem : public CRenderShader_Base
{

public:

	XM_ALIGNED_STRUCT(16) struct VPSInputPerObject
	{
		void Init(uint32 dwMode, XMFloat4x4Trinity* pTransforms, float fObjectAlpha)
		{
			m_mWorld = pTransforms->m_mWorld;
			m_mWorldView = pTransforms->m_mWorldView;
			m_mWorldViewProj = pTransforms->m_mWorldViewProj;

			m_fAlphaScaleAndMode.x = fObjectAlpha;
			*(uint32*)&m_fAlphaScaleAndMode.y = dwMode;
		}

		DirectX::XMFLOAT4X4	m_mWorld;
		DirectX::XMFLOAT4X4	m_mWorldView;
		DirectX::XMFLOAT4X4	m_mWorldViewProj;
		DirectX::XMFLOAT2A	m_fAlphaScaleAndMode;
	};

	CRenderShader_LineSystem() : CRenderShader_Base()
	{
		m_szName = "LineSystem";

		m_pVPSPerObjectBuffer = nullptr;
	}

	virtual ~CRenderShader_LineSystem();

	ID3D11Buffer*	GetVPSPerObjectBuffer() { return m_pVPSPerObjectBuffer; }

	virtual bool	Init(ID3D11VertexShader* pVertexShader, ID3D11PixelShader* pPixelShader, ID3D10Blob* pVSBlob);

	bool	SetPerObjectParams(uint32 dwRenderMode, XMFloat4x4Trinity* pTransforms, float fObjectAlpha);

	virtual bool	Validate(uint32 dwFlags);

	void	Render(UINT dwVertexCount);

	static const RenderShader	m_eRenderShader = SHADER_LineSystem;

private:

	bool	CreateInputLayout(void* pVSCode, uint32 dwCodeSize);
	bool	CreateVPSConstantBuffer();

	ID3D11Buffer*	m_pVPSPerObjectBuffer;
};

#endif