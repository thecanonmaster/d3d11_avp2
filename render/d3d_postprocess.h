#ifndef __D3D_POSTPROCESS_H__
#define __D3D_POSTPROCESS_H__

#ifndef __D3D11_H__
#include <d3d11_1.h>
#define __D3D11_H__
#endif

#ifndef __DIRECT_X_MATH_H__
#include <DirectXMath.h>
#define __DIRECT_X_MATH_H__
#endif

bool d3d_PostProcess_SolidDrawing(ID3D11ShaderResourceView* pSRV);
bool d3d_PostProcess_ToneMap(ID3D11ShaderResourceView* pSRV);
bool d3d_PostProcess_ScreenFX(ID3D11ShaderResourceView* pSRV);
bool d3d_PostProcess_ClearScreen(LTRect* pRect, DirectX::XMFLOAT3* pColor);
bool d3d_PostProcess_PassThrough(ID3D11ShaderResourceView* pSRV);
bool d3d_PostProcess_BloomExtract(ID3D11ShaderResourceView* pSRV);
bool d3d_PostProcess_BloomBlur(ID3D11ShaderResourceView* pSRV, bool bHorizontal, uint32 dwWidth, uint32 dwHeight);
bool d3d_PostProcess_BloomCombine(ID3D11ShaderResourceView* pSRV, ID3D11ShaderResourceView* pBloomSRV);

#endif