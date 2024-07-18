#ifndef __D3D_SHADER_POLY_GRID_H__
#define __D3D_SHADER_POLY_GRID_H__

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

class CRenderShader_PolyGrid : public CRenderShader_Base
{

public:

	XM_ALIGNED_STRUCT(16) struct VPSInputPerObject
	{
		void Init(uint32 dwMode, DirectX::XMFLOAT3* pModeColor, XMFloat4x4Trinity* pTransforms, 
			DirectX::XMFLOAT4* pColorScale)
		{
			m_mWorld = pTransforms->m_mWorld;
			m_mWorldView = pTransforms->m_mWorldView;
			m_mWorldViewProj = pTransforms->m_mWorldViewProj;
			m_vColorScale = *pColorScale;

			m_vModeColorAndMode.x = pModeColor->x;
			m_vModeColorAndMode.y = pModeColor->y;
			m_vModeColorAndMode.z = pModeColor->z;
			*(uint32*)&m_vModeColorAndMode.w = dwMode;
		}

		DirectX::XMFLOAT4X4	m_mWorld;
		DirectX::XMFLOAT4X4	m_mWorldView;
		DirectX::XMFLOAT4X4	m_mWorldViewProj;
		DirectX::XMFLOAT4	m_vColorScale;
		DirectX::XMFLOAT4	m_vModeColorAndMode;
	};

	CRenderShader_PolyGrid() : CRenderShader_Base()
	{
		m_szName = "PolyGrid";

		m_pVPSPerObjectBuffer = nullptr;
	}

	virtual ~CRenderShader_PolyGrid();

	ID3D11Buffer*	GetVPSPerObjectBuffer() { return m_pVPSPerObjectBuffer; }

	virtual bool	Init(ID3D11VertexShader* pVertexShader, ID3D11PixelShader* pPixelShader, ID3D10Blob* pVSBlob);

	bool	SetPerObjectParams(ID3D11ShaderResourceView* pMainTexture, ID3D11ShaderResourceView* pEnvTexture, uint32 dwMode,
		DirectX::XMFLOAT3* pModeColor, DirectX::XMFLOAT4* pColorScale, XMFloat4x4Trinity* pTransforms);

	virtual bool	Validate(uint32 dwFlags);

	void	Render(UINT dwIndexCount);

	static const RenderShader	m_eRenderShader = SHADER_PolyGrid;

private:

	bool	CreateInputLayout(void* pVSCode, uint32 dwCodeSize);
	bool	CreateVPSConstantBuffer();

	ID3D11Buffer*	m_pVPSPerObjectBuffer;
};

#endif