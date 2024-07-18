#ifndef __D3D_SHADER_SOLID_DRAWING_H__
#define __D3D_SHADER_SOLID_DRAWING_H__

#ifndef __D3D11_H__
#include <d3d11_1.h>
#define __D3D11_H__
#endif

#define SOLID_DRAWING_VERTICES	3

#include "d3d_shader_base.h"
#include "rendershadermgr.h"

class CRenderShader_SolidDrawing : public CRenderShader_Base
{

public:

	CRenderShader_SolidDrawing() : CRenderShader_Base()
	{ 
		m_szName = "SolidDrawing";
	}

	virtual ~CRenderShader_SolidDrawing() { };

	virtual bool	Init(ID3D11VertexShader* pVertexShader, ID3D11PixelShader* pPixelShader, ID3D10Blob* pVSBlob);

	void	SetPerObjectParams(ID3D11ShaderResourceView* pMainTexture);

	virtual bool	Validate(uint32 dwFlags);

	void	Render();

	static const RenderShader	m_eRenderShader = SHADER_SolidDrawing;
};

#endif