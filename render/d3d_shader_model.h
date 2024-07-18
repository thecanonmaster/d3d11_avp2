#ifndef __D3D_SHADER_MODEL_H__
#define __D3D_SHADER_MODEL_H__

#ifndef __D3D11_H__
#include <d3d11_1.h>
#define __D3D11_H__
#endif

#ifndef __DIRECT_X_MATH_H__
#include <DirectXMath.h>
#define __DIRECT_X_MATH_H__
#endif

#include "d3d_shader_base.h"
#include "draw_model.h"
#include "rendershadermgr.h"

using namespace DirectX;

//#define MODEL_PIXEL_LIGHTING

#define MAX_MODEL_TRANSFORMS			96
#define MAX_DYNAMIC_LIGHTS_PER_MODEL	16
#define MAX_STATIC_LIGHTS_PER_MODEL		16

class CRenderShader_Model : public CRenderShader_Base
{

public:

	struct VGPSPerObjectParams
	{
		DirectX::XMFLOAT4X4*	m_pNodeTransforms;
		uint32					m_dwNodeCount; 

		XMFloat4x4Trinity*	m_pTransforms;

		DirectX::XMFLOAT4*	m_pDiffuseColor;

		DirectX::XMFLOAT4X4*	m_pNormalRef;

		DirectX::XMFLOAT3*	m_pDirLightColor;
		uint32				m_dwDynamicLightCount;
		uint32				m_dwStaticLightCount;
		uint32*				m_pDynamicLightIndices;
		StaticLight**		m_ppStaticLights;

		ID3D11ShaderResourceView*	m_apTextures[MAX_MODEL_TEXTURES];

		uint32				m_adwMode[MAX_MODEL_TEXTURES];
		DirectX::XMFLOAT3	m_avModeColor[MAX_MODEL_TEXTURES];
		float				m_afAlphaRef[MAX_MODEL_TEXTURES];
	};

	XM_ALIGNED_STRUCT(16) struct StaticLight_VPS
	{
		void Init(StaticLight* pLight)
		{
			m_vPosition =
			{
				pLight->m_vPos.x, pLight->m_vPos.y, pLight->m_vPos.z
			};

			m_vInnerColorAndRadius =
			{
				pLight->m_vInnerColor.x * MATH_ONE_OVER_255,
				pLight->m_vInnerColor.y * MATH_ONE_OVER_255,
				pLight->m_vInnerColor.z * MATH_ONE_OVER_255,
				pLight->m_fRadius
			};

			m_vOuterColorAndFOV =
			{
				pLight->m_vOuterColor.x * MATH_ONE_OVER_255,
				pLight->m_vOuterColor.y * MATH_ONE_OVER_255,
				pLight->m_vOuterColor.z * MATH_ONE_OVER_255,
				pLight->m_fFOV
			};

			m_vDirection =
			{
				pLight->m_vDir.x,
				pLight->m_vDir.y,
				pLight->m_vDir.z,
				(float)(pLight->m_fFOV <= -1.0f || pLight->m_fFOV >= 1.0f || pLight->m_fFOV == 0.0f)
			};
	}

		DirectX::XMFLOAT3A	m_vPosition;
		DirectX::XMFLOAT4	m_vInnerColorAndRadius;
		DirectX::XMFLOAT4	m_vOuterColorAndFOV;
		DirectX::XMFLOAT4	m_vDirection;
};

	XM_ALIGNED_STRUCT(16) struct VSInputPerObject
	{
		void Init(VGPSPerObjectParams* pParams)
		{
			m_mWorld = pParams->m_pTransforms->m_mWorld;
			m_mWorldView = pParams->m_pTransforms->m_mWorldView;
			m_mWorldViewProj = pParams->m_pTransforms->m_mWorldViewProj;
			m_vDiffuseColor = *pParams->m_pDiffuseColor;
			m_mNormalRef = *pParams->m_pNormalRef;

			uint32 dwNodeCount = pParams->m_dwNodeCount;
			if (dwNodeCount > MAX_MODEL_TRANSFORMS)
				dwNodeCount = MAX_MODEL_TRANSFORMS;

			m_vModes =
			{
				pParams->m_adwMode[0], pParams->m_adwMode[1], pParams->m_adwMode[2], pParams->m_adwMode[3]
			};

			memcpy(m_amNodeTransforms, pParams->m_pNodeTransforms, sizeof(DirectX::XMFLOAT4X4) * dwNodeCount);

#ifndef MODEL_PIXEL_LIGHTING
			m_vDirLightColor = 
			{ 
				pParams->m_pDirLightColor->x, pParams->m_pDirLightColor->y, pParams->m_pDirLightColor->z
			};

			m_vDynamicAndStaticLightCount = 
			{ 
				pParams->m_dwDynamicLightCount, pParams->m_dwStaticLightCount, 0, 0
			};

			for (uint32 i = 0; i < pParams->m_dwDynamicLightCount; i++)
				m_avDynamicLightIndex[i] = { pParams->m_pDynamicLightIndices[i], 0, 0, 0 };

			for (uint32 i = 0; i < pParams->m_dwStaticLightCount; i++)
				m_aStaticLight[i].Init(pParams->m_ppStaticLights[i]);
#endif
		}

		DirectX::XMFLOAT4X4		m_mWorld;
		DirectX::XMFLOAT4X4		m_mWorldView;
		DirectX::XMFLOAT4X4		m_mWorldViewProj;
		DirectX::XMFLOAT4		m_vDiffuseColor;
		DirectX::XMFLOAT4X4		m_mNormalRef;
		DirectX::XMUINT4		m_vModes;
		DirectX::XMFLOAT4X4		m_amNodeTransforms[MAX_MODEL_TRANSFORMS];

#ifndef MODEL_PIXEL_LIGHTING
		DirectX::XMFLOAT3A	m_vDirLightColor;
		DirectX::XMUINT4	m_vDynamicAndStaticLightCount;
		DirectX::XMUINT4	m_avDynamicLightIndex[MAX_DYNAMIC_LIGHTS_PER_MODEL];
		StaticLight_VPS		m_aStaticLight[MAX_STATIC_LIGHTS_PER_MODEL];
#endif
	};

	XM_ALIGNED_STRUCT(16) struct GSInputPerObject
	{
		void Init(uint32 dwHiddenPartsCount, uint32* pHiddenPrimitiveIDs)
		{
			m_vHiddenPieceCount = { dwHiddenPartsCount, 0, 0, 0 };
			
			for (uint32 i = 0; i < dwHiddenPartsCount; i++)
			{
				m_vHiddenPrimitiveID[i] = 
				{ 
					pHiddenPrimitiveIDs[i * 2], pHiddenPrimitiveIDs[i * 2 + 1], 0, 0
				};
			}
		}

		DirectX::XMUINT4	m_vHiddenPieceCount;
		DirectX::XMUINT4	m_vHiddenPrimitiveID[MAX_PIECES_PER_MODEL];
	};

	XM_ALIGNED_STRUCT(16) struct PSInputPerObject
	{
		void Init(VGPSPerObjectParams* pParams)
		{
			for (uint32 i = 0; i < MAX_MODEL_TEXTURES; i++)
			{
				m_avModeColorAndMode[i].x = pParams->m_avModeColor[i].x;
				m_avModeColorAndMode[i].y = pParams->m_avModeColor[i].y;
				m_avModeColorAndMode[i].z = pParams->m_avModeColor[i].z;
				*(uint32*)&m_avModeColorAndMode[i].w = pParams->m_adwMode[i];
			}

			m_vAlphaRefs =
			{
				pParams->m_afAlphaRef[0], pParams->m_afAlphaRef[1], pParams->m_afAlphaRef[2], pParams->m_afAlphaRef[3]
			};

#ifdef MODEL_PIXEL_LIGHTING
			m_vDirLightColor = 
			{ 
				pParams->m_pDirLightColor->x, pParams->m_pDirLightColor->y, pParams->m_pDirLightColor->z
			};
			
			m_vDynamicAndStaticLightCount = 
			{ 
				pParams->m_dwDynamicLightCount, pParams->m_dwStaticLightCount, 0, 0
			};

			for (uint32 i = 0; i < pParams->m_dwDynamicLightCount; i++)
				m_avDynamicLightIndex[i] = { pParams->m_pDynamicLightIndices[i], 0, 0, 0 };

			for (uint32 i = 0; i < pParams->m_dwStaticLightCount; i++)
				m_aStaticLight[i].Init(pParams->m_ppStaticLights[i]);
#endif
		}

		DirectX::XMFLOAT4	m_avModeColorAndMode[MAX_MODEL_TEXTURES];
		DirectX::XMFLOAT4	m_vAlphaRefs;

#ifdef MODEL_PIXEL_LIGHTING
		DirectX::XMFLOAT3A	m_vDirLightColor;
		DirectX::XMUINT4	m_vDynamicAndStaticLightCount;
		DirectX::XMUINT4	m_avDynamicLightIndex[MAX_DYNAMIC_LIGHTS_PER_MODEL];
		StaticLight_VPS		m_aStaticLight[MAX_STATIC_LIGHTS_PER_MODEL];
#endif
	};

	CRenderShader_Model() : CRenderShader_Base()
	{
		m_szName = "Model";

		m_pGeometryShader = nullptr;

		m_pVSPerObjectBuffer = nullptr;
		m_pGSPerObjectBuffer = nullptr;
		m_pPSPerObjectBuffer = nullptr;
	}

	virtual ~CRenderShader_Model();

	virtual bool	ExtraInit(ID3D11GeometryShader* pGeometryShader);

	ID3D11Buffer*	GetVSPerObjectBuffer() { return m_pVSPerObjectBuffer; }
	ID3D11Buffer*	GetGSPerObjectBuffer() { return m_pGSPerObjectBuffer; }
	ID3D11Buffer*	GetPSPerObjectBuffer() { return m_pPSPerObjectBuffer; }

	virtual bool	Init(ID3D11VertexShader* pVertexShader, ID3D11PixelShader* pPixelShader, ID3D10Blob* pVSBlob);

	bool	SetPerObjectParamsVPS(VGPSPerObjectParams* pParams);
	bool	SetPerObjectParamsGS(uint32 dwHiddenPartsCount, uint32* pHiddenPrimitiveIDs);

	virtual bool	Validate(uint32 dwFlags);

	void	Render(uint32 dwIndexCount, uint32 dwStartIndex);

	static const RenderShader	m_eRenderShader = SHADER_Model;

private:

	bool	CreateInputLayout(void* pVSCode, uint32 dwCodeSize);
	bool	CreateVSConstantBuffer();
	bool	CreateGSConstantBuffer();
	bool	CreatePSConstantBuffer();

	ID3D11GeometryShader*	m_pGeometryShader;

	ID3D11Buffer*	m_pVSPerObjectBuffer;
	ID3D11Buffer*	m_pGSPerObjectBuffer;
	ID3D11Buffer*	m_pPSPerObjectBuffer;
};

#endif