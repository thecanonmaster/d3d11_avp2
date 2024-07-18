#include "pch.h"

#include "d3d_shader_base.h"
#include "common_stuff.h"

CRenderShader_Base::CRenderShader_Base()
{
	m_szName = nullptr;
	
	m_pVertexShader = nullptr;
	m_pPixelShader = nullptr;
	m_pLayout = nullptr;
}

CRenderShader_Base::~CRenderShader_Base()
{
	RELEASE_INTERFACE(m_pLayout, g_szFreeError_IL, m_szName);
	RELEASE_INTERFACE(m_pVertexShader, g_szFreeError_VS, m_szName);
	RELEASE_INTERFACE(m_pPixelShader, g_szFreeError_PS, m_szName);
}

bool CRenderShader_Base::Init(ID3D11VertexShader* pVertexShader, ID3D11PixelShader* pPixelShader, ID3D10Blob* pVSBlob)
{
	m_pVertexShader = pVertexShader;
	m_pPixelShader = pPixelShader;

	return true;
}
