#ifndef __D3D_SHADER_VIDEO_H__
#define __D3D_SHADER_VIDEO_H__

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

class CRenderShader_FullScreenVideo : public CRenderShader_Base
{

public:

	CRenderShader_FullScreenVideo() : CRenderShader_Base()
	{ 
		m_szName = "Video";
	}

	virtual ~CRenderShader_FullScreenVideo() { };

	virtual bool	Init(ID3D11VertexShader* pVertexShader, ID3D11PixelShader* pPixelShader, ID3D10Blob* pVSBlob);

	void	SetPerObjectParams(ID3D11ShaderResourceView* pMainTexture);

	virtual bool	Validate(uint32 dwFlags);

	void	Render();

	static const RenderShader	m_eRenderShader = SHADER_FullscreenVideo;

private:

	bool	CreateInputLayout(void* pVSCode, uint32 dwCodeSize);
};

#endif