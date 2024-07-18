#ifndef __D3D_SHADER_TONE_MAP_H__
#define __D3D_SHADER_TONE_MAP_H__

#ifndef __D3D11_H__
#include <d3d11_1.h>
#define __D3D11_H__
#endif

#include "d3d_shader_base.h"
#include "rendershadermgr.h"

#define TONE_MAP_VERTICES	3

class CRenderShader_ToneMap : public CRenderShader_Base
{

public:

	CRenderShader_ToneMap() : CRenderShader_Base()
	{ 
		m_szName = "ToneMap";
	}

	virtual ~CRenderShader_ToneMap() { };

	virtual bool	Init(ID3D11VertexShader* pVertexShader, ID3D11PixelShader* pPixelShader, ID3D10Blob* pVSBlob);

	void	SetPerObjectParams(ID3D11ShaderResourceView* pMainTexture);

	virtual bool	Validate(uint32 dwFlags);

	void	Render();

	static const RenderShader	m_eRenderShader = SHADER_ToneMap;
};

#endif