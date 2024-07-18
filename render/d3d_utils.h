#ifndef __D3D_UTILS_H__
#define __D3D_UTILS_H__

#ifndef __D3D11_H__
#include <d3d11_1.h>
#define __D3D11_H__
#endif

#ifndef __DIRECT_X_MATH_H__
#include <DirectXMath.h>
#define __DIRECT_X_MATH_H__
#endif

#define RGBA_MAKE(r, g, b, a)	((uint32)(((a) << 24) | ((r) << 16) | ((g) << 8) | (b)))
#define RGB_MAKE(r, g, b)		((uint32) (((r) << 16) | ((g) << 8) | (b)))
#define RGBA_GETA(color)		((uint8)( color               >> 24))
#define RGBA_GETR(color)		((uint8)((color & 0x00FF0000) >> 16))
#define RGBA_GETG(color)		((uint8)((color & 0x0000FF00) >> 8 ))
#define RGBA_GETB(color)		((uint8)((color & 0x000000FF)      ))
#define RGBA_GETFA(color)		((float)( color               >> 24))
#define RGBA_GETFR(color)		((float)((color & 0x00FF0000) >> 16))
#define RGBA_GETFG(color)		((float)((color & 0x0000FF00) >> 8 ))
#define RGBA_GETFB(color)		((float)((color & 0x000000FF)      ))

inline uint32 MakeRandomRGB()
{
	return RGB_MAKE(rand() % 256, rand() % 256, rand() % 256);
}

struct XMFloat4x4Trinity
{
	DirectX::XMFLOAT4X4	m_mWorld;
	DirectX::XMFLOAT4X4	m_mWorldView;
	DirectX::XMFLOAT4X4	m_mWorldViewProj;
};

inline void XMFloat4FromRGBA(DirectX::XMFLOAT4* pColor, uint32 dwColor)
{
	pColor->x = RGBA_GETFR(dwColor) * MATH_ONE_OVER_255;
	pColor->y = RGBA_GETFG(dwColor) * MATH_ONE_OVER_255;
	pColor->z = RGBA_GETFB(dwColor) * MATH_ONE_OVER_255;
	pColor->w = RGBA_GETFA(dwColor) * MATH_ONE_OVER_255;
}

inline void XMFloat3FromRGBA(DirectX::XMFLOAT3* pColor, uint32 dwColor)
{
	pColor->x = RGBA_GETFR(dwColor) * MATH_ONE_OVER_255;
	pColor->y = RGBA_GETFG(dwColor) * MATH_ONE_OVER_255;
	pColor->z = RGBA_GETFB(dwColor) * MATH_ONE_OVER_255;
}

void d3d_GetColorMasks(uint32 dwFormat, uint32& dwBitCount, uint32& dwAlphaMask, uint32& dwRedMask, uint32& dwGreenMask, uint32& dwBlueMask);
void d3d_D3DFormatToPFormat(uint32 dwFormat, PFormat* pFormat);

bool IsUsingSRGB();
bool IsS3TCFormat(uint32 dwBPP);
uint32 S3TCFormatConv_BPP(uint32 dwBPP);
uint32 S3TCFormatConv_Format(uint32 dwFormat);
uint32 GetTexturePitch(uint32 dwFormat, uint32 dwWidth);

constexpr ID3D11RenderTargetView* g_pNull_RenderTargetView = nullptr;
constexpr ID3D11ShaderResourceView* g_pNull_ShaderResourceView = nullptr;

#endif