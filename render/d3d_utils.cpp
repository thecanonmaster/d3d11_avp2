#include "pch.h"

#include "d3d_utils.h"
#include "d3d_device.h"
#include "common_stuff.h"
#include "rendererconsolevars.h"

void d3d_GetColorMasks(uint32 dwFormat, uint32& dwBitCount, uint32& dwAlphaMask, uint32& dwRedMask, uint32& dwGreenMask, uint32& dwBlueMask)
{
	if (dwFormat == g_D3DDevice.GetModeInfo()->m_dwFormat)
	{
		dwBitCount = 32;
		dwAlphaMask = 0x00000000; // TODO - are you sure?
		dwRedMask = 0x00FF0000;
		dwGreenMask = 0x0000FF00;
		dwBlueMask = 0x000000FF;
	}
}

void d3d_D3DFormatToPFormat(uint32 dwFormat, PFormat* pFormat)
{
	uint32 dwBitCount = 0, dwAlphaMask = 0, dwRedMask = 0, dwGreenMask = 0, dwBlueMask = 0;
	switch (dwFormat)
	{
		case DXGI_FORMAT_BC1_UNORM_SRGB:
		case DXGI_FORMAT_BC1_UNORM: pFormat->Init(BPP_S3TC_DXT1, dwAlphaMask, dwRedMask, dwGreenMask, dwBlueMask); return;

		case DXGI_FORMAT_BC2_UNORM_SRGB:
		case DXGI_FORMAT_BC2_UNORM: pFormat->Init(BPP_S3TC_DXT3, dwAlphaMask, dwRedMask, dwGreenMask, dwBlueMask); return;

		case DXGI_FORMAT_BC3_UNORM_SRGB:
		case DXGI_FORMAT_BC3_UNORM: pFormat->Init(BPP_S3TC_DXT5, dwAlphaMask, dwRedMask, dwGreenMask, dwBlueMask); return;
	}

	d3d_GetColorMasks(dwFormat, dwBitCount, dwAlphaMask, dwRedMask, dwGreenMask, dwBlueMask);
	if (dwBitCount == 32)
	{
		pFormat->Init(BPP_32, dwAlphaMask, dwRedMask, dwGreenMask, dwBlueMask); 
		return;
	}

	AddDebugMessage(0, "Unsupported format %d", dwFormat);
}

bool IsUsingSRGB()
{
	return (g_D3DDevice.GetModeInfo()->m_dwFormat == DXGI_FORMAT_B8G8R8A8_UNORM_SRGB);
}

bool IsS3TCFormat(uint32 dwBPP)
{
	return (dwBPP == BPP_S3TC_DXT1 || dwBPP == BPP_S3TC_DXT3 || dwBPP == BPP_S3TC_DXT5);
}

uint32 S3TCFormatConv_BPP(uint32 dwBPP)
{
	switch (dwBPP)
	{
		case BPP_S3TC_DXT1: return !IsUsingSRGB() ? DXGI_FORMAT_BC1_UNORM : DXGI_FORMAT_BC1_UNORM_SRGB;
		case BPP_S3TC_DXT3: return !IsUsingSRGB() ? DXGI_FORMAT_BC2_UNORM : DXGI_FORMAT_BC2_UNORM_SRGB;
		case BPP_S3TC_DXT5: return !IsUsingSRGB() ? DXGI_FORMAT_BC3_UNORM : DXGI_FORMAT_BC3_UNORM_SRGB;
	}

	return 0;
}

uint32 S3TCFormatConv_Format(uint32 dwFormat)
{
	switch (dwFormat)
	{
		case DXGI_FORMAT_BC1_UNORM_SRGB:
		case DXGI_FORMAT_BC1_UNORM: return BPP_S3TC_DXT1;

		case DXGI_FORMAT_BC2_UNORM_SRGB:
		case DXGI_FORMAT_BC2_UNORM: return BPP_S3TC_DXT3;

		case DXGI_FORMAT_BC3_UNORM_SRGB:
		case DXGI_FORMAT_BC3_UNORM: return BPP_S3TC_DXT5;
	}

	return 0;
}

uint32 GetTexturePitch(uint32 dwFormat, uint32 dwWidth)
{
	switch (dwFormat)
	{
		case DXGI_FORMAT_BC1_UNORM_SRGB:
		case DXGI_FORMAT_BC1_UNORM: return (dwWidth << 1);

		case DXGI_FORMAT_BC2_UNORM_SRGB:
		case DXGI_FORMAT_BC2_UNORM:
		case DXGI_FORMAT_BC3_UNORM_SRGB:
		case DXGI_FORMAT_BC3_UNORM: 
		default:					return (dwWidth << 2);
	}
}