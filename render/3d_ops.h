#ifndef __3D_OPS_H__
#define __3D_OPS_H__

#ifndef __D3D11_H__
#include <d3d11_1.h>
#define __D3D11_H__
#endif

#ifndef __DIRECT_X_MATH_H__
#include <DirectXMath.h>
#define __DIRECT_X_MATH_H__
#endif

#define ROTATION_MAX	100000.0f

struct RGBColor
{
	union
	{
		LTRGB	rgb;
		uint32	color;
	};
};

class CReallyCloseData
{

public:

	D3D11_VIEWPORT	m_Viewport;
};

void d3d_SetupTransformation(const LTVector* pPos, float* pRotation, LTVector* pScale, DirectX::XMFLOAT4X4* pMatrix);

void d3d_SetReallyClose(CReallyCloseData* pData);
void d3d_SetReallyClose(CReallyCloseData* pData, DirectX::XMFLOAT4X4* pProjection, 
	float fFovXOffset, float fFovYOffset, bool bUseTan);
void d3d_CalcReallyCloseMatrix(DirectX::XMFLOAT4X4* pProjection, float fFovXOffset, float fFovYOffset, bool bUseTan);

void d3d_UnsetReallyClose(CReallyCloseData* pData);

#endif