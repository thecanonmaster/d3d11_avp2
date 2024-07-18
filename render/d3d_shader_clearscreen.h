#ifndef __D3D_CLEAR_SCREEN_MAP_H__
#define __D3D_CLEAR_SCREEN_MAP_H__

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

#define CLEAR_SCREEN_VERTICES	3

class CRenderShader_ClearScreen : public CRenderShader_Base
{

public:

	XM_ALIGNED_STRUCT(16) struct PSInputPerObject
	{
		void Init(LTRect* pRect, DirectX::XMFLOAT3* pColor)
		{
			vFillArea = 
			{
				(float)pRect->m_nLeft,
				(float)pRect->m_nTop,
				(float)pRect->m_nRight,
				(float)pRect->m_nBottom
			};

			vFillColor =
			{
				pColor->x, pColor->y, pColor->z
			};
		}

		DirectX::XMFLOAT4	vFillArea;
		DirectX::XMFLOAT3A	vFillColor;
	};

	CRenderShader_ClearScreen() : CRenderShader_Base()
	{ 
		m_szName = "ClearScreen";

		m_pPSPerObjectBuffer = nullptr;
	}

	virtual ~CRenderShader_ClearScreen();

	virtual bool	Init(ID3D11VertexShader* pVertexShader, ID3D11PixelShader* pPixelShader, ID3D10Blob* pVSBlob);

	bool	SetPerObjectParams(LTRect* pRect, DirectX::XMFLOAT3* pColor);

	virtual bool	Validate(uint32 dwFlags);

	void	Render();

	static const RenderShader	m_eRenderShader = SHADER_ClearScreen;

private:

	bool	CreatePSConstantBuffer();

	ID3D11Buffer* m_pPSPerObjectBuffer;
};

#endif