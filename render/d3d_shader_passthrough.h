#ifndef __D3D_SHADER_PASS_THROUGH_H__
#define __D3D_SHADER_PASS_THROUGH_H__

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

#define PASS_THROUGH_VERTICES	3

class CRenderShader_PassThrough : public CRenderShader_Base
{

public:

	CRenderShader_PassThrough() : CRenderShader_Base()
	{ 
		m_szName = "PassThrough";
	}

	virtual ~CRenderShader_PassThrough() { };

	virtual bool	Init(ID3D11VertexShader* pVertexShader, ID3D11PixelShader* pPixelShader, ID3D10Blob* pVSBlob);

	void	SetPerObjectParams(ID3D11ShaderResourceView* pMainTexture);

	virtual bool	Validate(uint32 dwFlags);

	void	Render();

	static const RenderShader	m_eRenderShader = SHADER_PassThrough;
};

#endif