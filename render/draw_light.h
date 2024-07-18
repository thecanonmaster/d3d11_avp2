#ifndef __DRAW_LIGHT_H__
#define __DRAW_LIGHT_H__

#ifndef __DIRECT_X_MATH_H__
#include <DirectXMath.h>
#define __DIRECT_X_MATH_H__
#endif

#define MODEL_SHADOW_LIGHTS_RESERVE	16

struct ModelShadowLight
{
	ModelShadowLight(ModelInstance* pInstance, DirectX::XMFLOAT4* pPositionAndRadius)
	{
		m_pInstance = pInstance;
		m_vPositionAndRadius = *pPositionAndRadius;
	}

	ModelShadowLight(ModelInstance* pInstance, DirectX::XMFLOAT3* pPosition, float fRadius)
	{
		m_pInstance = pInstance;
		m_vPositionAndRadius = 
		{
			pPosition->x, pPosition->y, pPosition->z, fRadius
		};
	}

	ModelInstance*		m_pInstance;
	DirectX::XMFLOAT4	m_vPositionAndRadius;
};

typedef std::vector<ModelShadowLight> Array_ModelShadowLight;

void d3d_LightLookup_Internal(LightTable* pTable, DirectX::XMFLOAT3* pPos, DirectX::XMFLOAT3* pColor);
void d3d_LightLookup_Original(LightTable* pTable, DirectX::XMFLOAT3* pPos, DirectX::XMFLOAT3* pColor);
void d3d_CalcLightAdd(DirectX::XMFLOAT3* pPos, DirectX::XMFLOAT3* pLightAdd, uint32 dwMaxLights);
void d3d_ProcessLight(LTObject* pObj);
void d3d_ProcessModelShadowLight(ModelInstance* pInstance);

extern Array_ModelShadowLight g_aModelShadowLight;

#endif