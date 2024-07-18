#ifndef __D3D_SHADER_PARTICLES_H__
#define __D3D_SHADER_PARTICLES_H__

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

class CRenderShader_ParticleSystem : public CRenderShader_Base
{

public:

	XM_ALIGNED_STRUCT(16) struct VPSInputPerObject
	{
		void Init(uint32 dwMode, DirectX::XMFLOAT3* pModeColor, XMFloat4x4Trinity* pTransforms, 
			DirectX::XMFLOAT4* pColorScale,DirectX::XMFLOAT3* pParticleUp, DirectX::XMFLOAT3* pParticleRight)
		{
			m_mWorld = pTransforms->m_mWorld;
			m_mWorldView = pTransforms->m_mWorldView;
			m_mWorldViewProj = pTransforms->m_mWorldViewProj;
			m_vColorScale = *pColorScale;

			m_vParticleUp =
			{
				pParticleUp->x, pParticleUp->y, pParticleUp->z
			};

			m_vParticleRight =
			{
				pParticleRight->x, pParticleRight->y, pParticleRight->z
			};

			m_vModeColorAndMode.x = pModeColor->x;
			m_vModeColorAndMode.y = pModeColor->y;
			m_vModeColorAndMode.z = pModeColor->z;
			*(uint32*)&m_vModeColorAndMode.w = dwMode;
		}

		DirectX::XMFLOAT4X4	m_mWorld;
		DirectX::XMFLOAT4X4	m_mWorldView;
		DirectX::XMFLOAT4X4	m_mWorldViewProj;
		DirectX::XMFLOAT4	m_vColorScale;
		DirectX::XMFLOAT3A	m_vParticleUp;
		DirectX::XMFLOAT3A	m_vParticleRight;
		DirectX::XMFLOAT4	m_vModeColorAndMode;
	};

	CRenderShader_ParticleSystem() : CRenderShader_Base()
	{
		m_szName = "Particles";

		m_pVPSPerObjectBuffer = nullptr;
	}

	virtual ~CRenderShader_ParticleSystem();

	ID3D11Buffer*	GetVPSPerObjectBuffer() { return m_pVPSPerObjectBuffer; }

	virtual bool	Init(ID3D11VertexShader* pVertexShader, ID3D11PixelShader* pPixelShader, ID3D10Blob* pVSBlob);

	bool	SetPerObjectParams(ID3D11ShaderResourceView* pMainTexture, uint32 dwMode, DirectX::XMFLOAT3* pModeColor, DirectX::XMFLOAT4* pColorScale, 
		DirectX::XMFLOAT3* pParticleUp, DirectX::XMFLOAT3* pParticleRight, XMFloat4x4Trinity* pTransforms);

	virtual bool	Validate(uint32 dwFlags);

	void	Render(UINT dwIndexCount, UINT dwInstanceCount);

	static const RenderShader	m_eRenderShader = SHADER_ParticleSystem;

private:

	bool	CreateInputLayout(void* pVSCode, uint32 dwCodeSize);
	bool	CreateVPSConstantBuffer();

	ID3D11Buffer*	m_pVPSPerObjectBuffer;
};

#endif