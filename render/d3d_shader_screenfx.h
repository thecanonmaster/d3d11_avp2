#ifndef __D3D_SHADER_SCREEN_FX_H__
#define __D3D_SHADER_SCREEN_FX_H__

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

#define SCREEN_FX_VERTICES	3

class CRenderShader_ScreenFX : public CRenderShader_Base
{

public:

	CRenderShader_ScreenFX() : CRenderShader_Base()
	{ 
		m_szName = "ScreenFX";
	}

	virtual ~CRenderShader_ScreenFX() { };

	virtual bool	Init(ID3D11VertexShader* pVertexShader, ID3D11PixelShader* pPixelShader, ID3D10Blob* pVSBlob);

	void	SetPerObjectParams(ID3D11ShaderResourceView* pMainTexture);

	virtual bool	Validate(uint32 dwFlags);

	void	Render();

	static const RenderShader	m_eRenderShader = SHADER_ScreenFX;
};

#endif