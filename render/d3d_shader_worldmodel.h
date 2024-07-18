#ifndef __D3D_SHADER_WORLD_MODEL_H__
#define __D3D_SHADER_WORLD_MODEL_H__

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
#include "draw_worldmodel.h"
#include "rendermodemgr.h"
#include "rendershadermgr.h"

using namespace DirectX;

#define MAX_LIGHTS_PER_SUB_WORLD_MODEL		32
#define MAX_MS_LIGHTS_PER_SUB_WORLD_MODEL	16

class CRenderShader_WorldModel : public CRenderShader_Base
{

public:

	struct VPSPerSubObjectParams
	{
		VPSPerSubObjectParams()
		{
			m_pTexture = nullptr;
			m_vModeColor = { };
			m_dwMode = MODE_NO_TEXTURE;
			m_fAlphaRef = 0.0f;
		}
		
		ID3D11ShaderResourceView*	m_pTexture;

		DirectX::XMFLOAT3	m_vModeColor;
		uint32				m_dwMode;

		float	m_fAlphaRef;
	};

	XM_ALIGNED_STRUCT(16) struct VSInputPerObject
	{
		void Init(XMFloat4x4Trinity* pTransforms, DirectX::XMFLOAT4* pDiffuseColor)
		{
			m_mWorld = pTransforms->m_mWorld;
			m_mWorldView = pTransforms->m_mWorldView;
			m_mWorldViewProj = pTransforms->m_mWorldViewProj;

			m_vDiffuseColor = *pDiffuseColor;
		}

		DirectX::XMFLOAT4X4	m_mWorld;
		DirectX::XMFLOAT4X4	m_mWorldView;
		DirectX::XMFLOAT4X4	m_mWorldViewProj;
		
		DirectX::XMFLOAT4	m_vDiffuseColor;
	};

	XM_ALIGNED_STRUCT(16) struct VSInputPerSubObject
	{
		void InitAll(VPSPerSubObjectParams* pParams)
		{
			for (uint32 i = 0; i < WORLD_MODEL_TEXTURES_PER_DRAW; i++)
			{
				*(uint32*)&m_avMode[i].x = pParams[i].m_dwMode;
				m_avMode[i].y = 0.0f;
			}
		}

		void InitSingle(uint32 dwIndex, VPSPerSubObjectParams* pParams)
		{
			*(uint32*)&m_avMode[dwIndex].x = pParams->m_dwMode;
			m_avMode[dwIndex].y = 0.0f;
		}

		DirectX::XMFLOAT2A	m_avMode[WORLD_MODEL_TEXTURES_PER_DRAW];
	};

	XM_ALIGNED_STRUCT(16) struct PSInputPerSubObject
	{
		void InitAll(VPSPerSubObjectParams* pParams)
		{
			for (uint32 i = 0; i < WORLD_MODEL_TEXTURES_PER_DRAW; i++)
			{
				VPSPerSubObjectParams& currParams = pParams[i];
				
				m_avModeColorAndMode[i].x = currParams.m_vModeColor.x;
				m_avModeColorAndMode[i].y = currParams.m_vModeColor.y;
				m_avModeColorAndMode[i].z = currParams.m_vModeColor.z;
				*(uint32*)&m_avModeColorAndMode[i].w = currParams.m_dwMode;
		
				m_avAlphaRef[i] = { currParams.m_fAlphaRef, 0.0f, 0.0f, 0.0f };
			}
		}

		void InitSingle(uint32 dwIndex, VPSPerSubObjectParams* pParams)
		{
			m_avModeColorAndMode[dwIndex].x = pParams->m_vModeColor.x;
			m_avModeColorAndMode[dwIndex].y = pParams->m_vModeColor.y;
			m_avModeColorAndMode[dwIndex].z = pParams->m_vModeColor.z;
			*(uint32*)&m_avModeColorAndMode[dwIndex].w = pParams->m_dwMode;

			m_avAlphaRef[dwIndex] = { pParams->m_fAlphaRef, 0.0f, 0.0f, 0.0f };
		}

		void InitLightIndices(uint32 dwCount, uint32* pIndices, uint32 dwMSCount, uint32* pMSIndices)
		{
			m_vDynLightAndMSLightCount = { dwCount, dwMSCount, 0, 0 };

			for (uint32 i = 0; i < dwCount; i++)
				m_avDynLightAndMSLightIndex[i] = { pIndices[i], 0, 0, 0 };

			for (uint32 i = 0; i < dwMSCount; i++)
				m_avDynLightAndMSLightIndex[i].y = pMSIndices[i];
		}

		DirectX::XMFLOAT4	m_avModeColorAndMode[WORLD_MODEL_TEXTURES_PER_DRAW];
		DirectX::XMFLOAT4	m_avAlphaRef[WORLD_MODEL_TEXTURES_PER_DRAW];

		DirectX::XMUINT4	m_vDynLightAndMSLightCount;
		DirectX::XMUINT4	m_avDynLightAndMSLightIndex[MAX_LIGHTS_PER_SUB_WORLD_MODEL];
	};

	CRenderShader_WorldModel() : CRenderShader_Base()
	{
		m_szName = "WorldModel";

		m_pVSPerObjectBuffer = nullptr;
		m_pVSPerSubObjectBuffer = nullptr;
		m_pPSPerSubObjectBuffer = nullptr;
	}

	virtual ~CRenderShader_WorldModel();

	ID3D11Buffer*	GetVSPerObjectBuffer() { return m_pVSPerObjectBuffer; }
	ID3D11Buffer*	GetVSPerSubObjectBuffer() { return m_pVSPerSubObjectBuffer; }
	ID3D11Buffer*	GetPSPerSubObjectBuffer() { return m_pPSPerSubObjectBuffer; }

	virtual bool	Init(ID3D11VertexShader* pVertexShader, ID3D11PixelShader* pPixelShader, ID3D10Blob* pVSBlob);

	bool	SetPerObjectParams(XMFloat4x4Trinity* pTransforms, DirectX::XMFLOAT4* pDiffuseColor, 
		ID3D11ShaderResourceView* pLMVertexData);

	bool	SetPerSubObjectParams(VPSPerSubObjectParams* pParams, uint32 dwLightCount, uint32* pLightIndices,
		uint32 dwMSLightCount, uint32* pMSLightIndices);

	virtual bool	Validate(uint32 dwFlags);

	virtual void	Render(uint32 dwIndexCount, uint32 dwStartIndex);

	static const RenderShader	m_eRenderShader = SHADER_WorldModel;

protected:

	bool	CreateInputLayout(void* pVSCode, uint32 dwCodeSize);
	bool	CreateVSConstantBuffers();
	bool	CreatePSConstantBuffer();

	ID3D11Buffer*	m_pVSPerObjectBuffer;
	ID3D11Buffer*	m_pVSPerSubObjectBuffer;
	ID3D11Buffer*	m_pPSPerSubObjectBuffer;
};

class CRenderShader_SkyWorldModel : public CRenderShader_WorldModel
{

public:

	CRenderShader_SkyWorldModel() : CRenderShader_WorldModel()
	{
		m_szName = "SkyWorldModel";

		m_pVSPerObjectBuffer = nullptr;
		m_pVSPerSubObjectBuffer = nullptr;
		m_pPSPerSubObjectBuffer = nullptr;
	}

	virtual void	Render(uint32 dwIndexCount, uint32 dwStartIndex);

	static const RenderShader	m_eRenderShader = SHADER_SkyWorldModel;
};

class CRenderShader_SkyPortal : public CRenderShader_Base
{

public:

	XM_ALIGNED_STRUCT(16) struct VPSInputPerObject
	{
		void Init(XMFloat4x4Trinity* pTransforms, uint32 dwScreenWidth, uint32 dwScreenHeight)
		{
			m_mWorldViewProj = pTransforms->m_mWorldViewProj;
			m_vScreenDims = { (float)dwScreenWidth, (float)dwScreenHeight };
		}

		DirectX::XMFLOAT4X4	m_mWorldViewProj;
		DirectX::XMFLOAT2A	m_vScreenDims;
	};

	CRenderShader_SkyPortal() : CRenderShader_Base()
	{
		m_szName = "SkyPortal";

		m_pVPSPerObjectBuffer = nullptr;
	}

	virtual ~CRenderShader_SkyPortal();

	ID3D11Buffer* GetVPSPerObjectBuffer() { return m_pVPSPerObjectBuffer; }

	virtual bool	Init(ID3D11VertexShader* pVertexShader, ID3D11PixelShader* pPixelShader, ID3D10Blob* pVSBlob);

	bool	SetPerObjectParams(ID3D11ShaderResourceView* pMainTexture, XMFloat4x4Trinity* pTransforms, 
		uint32 dwScreenWidth, uint32 dwScreenHeight);

	virtual bool	Validate(uint32 dwFlags);

	virtual void	Render(uint32 dwIndexCount);

	static const RenderShader	m_eRenderShader = SHADER_SkyPortal;

protected:

	bool	CreateInputLayout(void* pVSCode, uint32 dwCodeSize);
	bool	CreateVPSConstantBuffer();

	ID3D11Buffer*	m_pVPSPerObjectBuffer;
};

#endif