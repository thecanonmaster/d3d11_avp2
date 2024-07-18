#ifndef __D3D_SHADER_BASE_H__
#define __D3D_SHADER_BASE_H__

#ifndef __D3D11_H__
#include <d3d11_1.h>
#define __D3D11_H__
#endif

#ifndef __DIRECT_X_MATH_H__
#include <DirectXMath.h>
#define __DIRECT_X_MATH_H__
#endif

#include "d3d_utils.h"
#include "common_stuff.h"
#include "d3d_mathhelpers.h"
#include "draw_light.h"

struct LMPage;

static const char* g_szFreeError_IL = "Failed to free D3D input layout (RS = %s, count = %d)";
static const char* g_szFreeError_VS = "Failed to free D3D vertex shader (RS = %s, count = %d)";
static const char* g_szFreeError_GS = "Failed to free D3D geometry shader (RS = %s, count = %d)";
static const char* g_szFreeError_PS = "Failed to free D3D pixel shader (RS = %s, count = %d)";
static const char* g_szFreeError_VS_PF_CB = "Failed to free D3D VS per-frame constant buffer (RS = %s, count = %d)";
static const char* g_szFreeError_PS_PF_CB = "Failed to free D3D VS per-frame constant buffer (RS = %s, count = %d)";
static const char* g_szFreeError_VS_PO_CB = "Failed to free D3D VS per-object constant buffer (RS = %s, count = %d)";
static const char* g_szFreeError_GS_PO_CB = "Failed to free D3D GS per-object constant buffer (RS = %s, count = %d)";
static const char* g_szFreeError_PS_PO_CB = "Failed to free D3D PS per-object constant buffer (RS = %s, count = %d)";
static const char* g_szFreeError_VPS_PO_CB = "Failed to free D3D VS/PS per-object constant buffer (RS = %s, count = %d)";
static const char* g_szFreeError_VS_PSO_CB = "Failed to free D3D VS per-sub-object constant buffer (RS = %s, count = %d)";
static const char* g_szFreeError_PS_PSO_CB = "Failed to free D3D PS per-sub-object constant buffer (RS = %s, count = %d)";
static const char* g_szFreeError_VPS_DL_CB = "Failed to free D3D VS/PS dynamic constant buffer (RS = %s, count = %d)";


static const char* g_szValidationError = "Validation failed for shader \"%s\"";

#define MAX_LIGHT_ANIMS_PER_BUFF	32
#define MAX_LIGHTS_PER_BUFF			192
#define MAX_MS_LIGHTS_PER_BUFF		32

class CRenderShader_Base
{

public:

	struct VSPerFrameParams_3D
	{
		VSPerFrameParams_3D()
		{
			memset(this, 0, sizeof(VSPerFrameParams_3D));
		}

		uint32	m_dwScreenWidth;
		uint32	m_dwScreenHeight;

		float	m_fFogStart;
		float	m_fFogEnd;

		DirectX::XMFLOAT4X4* m_pView;
		DirectX::XMFLOAT4X4* m_pInvView;

		DirectX::XMFLOAT3* m_pCameraPos;

		float	m_fEnvScale;
		float	m_fEnvPanSpeed;

		bool	m_bVFogEnabled;
		float	m_fVFogDensity;
		float	m_fVFogMinY;
		float	m_fVFogMaxY;
		float	m_fVFogMinYVal;
		float	m_fVFogMaxYVal;
		float	m_fVFogMax;
		float	m_fSkyFogNear;
		float	m_fSkyFogFar;

		float	m_fCurSkyXOffset;
		float	m_fCurSkyZOffset;
		float	m_fPanSkyScaleX;
		float	m_fPanSkyScaleZ;

#ifndef MODEL_PIXEL_LIGHTING
		DirectX::XMFLOAT3*	m_pDirLightDir;
#endif
		LMAnim* m_pLMAnims;
		uint32	m_dwLMAnims;
	};

	struct PSPerFrameParams_3D
	{
		PSPerFrameParams_3D()
		{
			memset(this, 0, sizeof(PSPerFrameParams_3D));
		}

		DirectX::XMFLOAT3*	m_pFogColor;
		DirectX::XMFLOAT3*	m_pFogColorMP;

		DirectX::XMFLOAT3*	m_pLightAdd;
		DirectX::XMFLOAT3*	m_pLightScale;
		bool				m_bInvertHack;

		ID3D11ShaderResourceView*	m_pGlobalEnvMapSRV;
		ID3D11ShaderResourceView*	m_pGlobalPanSRV;

		DirectX::XMFLOAT3*	m_pGlobalVertexTint;

#ifdef MODEL_PIXEL_LIGHTING
		DirectX::XMFLOAT3*	m_pDirLightDir;
#endif
		LMAnim* m_pLMAnims;
		uint32	m_dwLMAnims;

		ID3D11ShaderResourceView*	m_pLMPages;
	};

	XM_ALIGNED_STRUCT(16) struct VSInputPerFrame_Opt2D
	{
		void Init(uint32 dwScreenWidth, uint32 dwScreenHeight)
		{
			m_vScreenDims = 
			{
				1.0f / (float)dwScreenWidth, 1.0f / (float)dwScreenHeight, 0.0f, 0.0f
			};
		}

		DirectX::XMFLOAT4	m_vScreenDims;
	};

	XM_ALIGNED_STRUCT(16) struct PSInputPerFrame_Opt2D
	{
		void Init(float fExposure)
		{
			m_fExposure = fExposure;
		}

		float m_fExposure;
	};

	XM_ALIGNED_STRUCT(16) struct LMBlendData_VS
	{
		void Init()
		{
			m_vFramesDataAndShadowMap =
			{
				0, LIGHTANIMFRAME_NONE, LIGHTANIMFRAME_NONE, 0
			};

			m_fBetweenAndBlendPercent = { };
		}

		void Init(LMAnim* pAnim)
		{
			m_vFramesDataAndShadowMap =
			{
				pAnim->m_nFrames, pAnim->m_dwBetweenFrames[0], pAnim->m_dwBetweenFrames[1], pAnim->m_bShadowMap
			};

			m_fBetweenAndBlendPercent = 
			{
				pAnim->m_fPercentBetween, pAnim->m_fBlendPercent, 0.0f, 0.0f
			};
		}

		DirectX::XMUINT4	m_vFramesDataAndShadowMap;
		DirectX::XMFLOAT4	m_fBetweenAndBlendPercent;
	};

	XM_ALIGNED_STRUCT(16) struct LMBlendData_PS
	{
		void Init()
		{
			m_vFrameBetweenFramesAndShadowMap =
			{
				0, LIGHTANIMFRAME_NONE, LIGHTANIMFRAME_NONE, 0
			};

			m_fBetweenAndBlendPercent = { };
		}

		void Init(LMAnim* pAnim)
		{
			m_vFrameBetweenFramesAndShadowMap =
			{
				pAnim->m_nFrames, pAnim->m_dwBetweenFrames[0], pAnim->m_dwBetweenFrames[1], pAnim->m_bShadowMap
			};

			m_fBetweenAndBlendPercent = 
			{
				pAnim->m_fPercentBetween, pAnim->m_fBlendPercent, 0.0f, 0.0f
			};

			m_vLightPos = { pAnim->m_vLightPos.x, pAnim->m_vLightPos.y, pAnim->m_vLightPos.z };
			
			m_vLightColorAndRadius = 
			{
				pAnim->m_vLightColor.x * MATH_ONE_OVER_255,
				pAnim->m_vLightColor.y * MATH_ONE_OVER_255,
				pAnim->m_vLightColor.z * MATH_ONE_OVER_255,
				pAnim->m_fLightRadius
			};
		}

		DirectX::XMUINT4	m_vFrameBetweenFramesAndShadowMap;
		DirectX::XMFLOAT4	m_fBetweenAndBlendPercent;
		DirectX::XMFLOAT3A	m_vLightPos;
		DirectX::XMFLOAT4	m_vLightColorAndRadius;
	};

	XM_ALIGNED_STRUCT(16) struct VSInputPerFrame_3D
	{
		void Init(VSPerFrameParams_3D* pInitStruct)
		{
			m_vScreenDimsAndFogStartEnd =
			{
				1.0f / (float)pInitStruct->m_dwScreenWidth,
				1.0f / (float)pInitStruct->m_dwScreenHeight,
				pInitStruct->m_fFogStart, pInitStruct->m_fFogEnd
			};

			m_mView = *pInitStruct->m_pView;
			m_mInvView = *pInitStruct->m_pInvView;

			m_vCameraPos = 
			{
				pInitStruct->m_pCameraPos->x,
				pInitStruct->m_pCameraPos->y,
				pInitStruct->m_pCameraPos->z,
			};

			m_vEnvScalePanSpeedAndVFogEnabledDensity =
			{
				pInitStruct->m_fEnvScale, pInitStruct->m_fEnvPanSpeed,
				(float)pInitStruct->m_bVFogEnabled, pInitStruct->m_fVFogDensity
			};

			m_vVFogMinYMaxAndVals =
			{
				pInitStruct->m_fVFogMinY, pInitStruct->m_fVFogMaxY,
				pInitStruct->m_fVFogMinYVal, pInitStruct->m_fVFogMaxYVal
			};

			m_vVFogMaxAndSkyFogNearFar = 
			{
				pInitStruct->m_fVFogMax, pInitStruct->m_fSkyFogNear, pInitStruct->m_fSkyFogFar, 0.0f
			};

			m_vSkyPanScaleAndOffset = 
			{
				pInitStruct->m_fPanSkyScaleX, pInitStruct->m_fPanSkyScaleZ,
				pInitStruct->m_fCurSkyXOffset, pInitStruct->m_fCurSkyZOffset
			};

#ifndef MODEL_PIXEL_LIGHTING

			m_vDirLightDirAndLMAnims.x = pInitStruct->m_pDirLightDir->x;
			m_vDirLightDirAndLMAnims.y = pInitStruct->m_pDirLightDir->y;
			m_vDirLightDirAndLMAnims.z = pInitStruct->m_pDirLightDir->z;
			*(uint32*)& m_vDirLightDirAndLMAnims.w = pInitStruct->m_dwLMAnims;

#else
			m_vLMAnimCount = { pInitStruct->m_dwLMAnims, 0, 0, 0 };
#endif

			if (pInitStruct->m_pLMAnims != nullptr)
			{
				for (uint32 i = 0; i < pInitStruct->m_dwLMAnims; i++)
					m_aLMBlendData[i].Init(&pInitStruct->m_pLMAnims[i]);
			}
		}

		DirectX::XMFLOAT4	m_vScreenDimsAndFogStartEnd;
		DirectX::XMFLOAT4X4	m_mView;
		DirectX::XMFLOAT4X4	m_mInvView;
		DirectX::XMFLOAT3A	m_vCameraPos;
		DirectX::XMFLOAT4	m_vEnvScalePanSpeedAndVFogEnabledDensity;
		DirectX::XMFLOAT4	m_vVFogMinYMaxAndVals;
		DirectX::XMFLOAT4	m_vVFogMaxAndSkyFogNearFar;
		DirectX::XMFLOAT4	m_vSkyPanScaleAndOffset;

#ifndef MODEL_PIXEL_LIGHTING
		DirectX::XMFLOAT4	m_vDirLightDirAndLMAnims;
#else
		DirectX::XMUINT4	m_vLMAnimCount;
#endif

		LMBlendData_VS		m_aLMBlendData[MAX_LIGHT_ANIMS_PER_BUFF];
	};

	XM_ALIGNED_STRUCT(16) struct PSInputPerFrame_3D
	{
		void Init(PSPerFrameParams_3D* pInitStruct)
		{
			m_vFogColor = 
			{ 
				pInitStruct->m_pFogColor->x, 
				pInitStruct->m_pFogColor->y, 
				pInitStruct->m_pFogColor->z 
			};

			m_vLightAddAndInvertHack =
			{
				pInitStruct->m_pLightAdd->x,
				pInitStruct->m_pLightAdd->y,
				pInitStruct->m_pLightAdd->z,
				(float)pInitStruct->m_bInvertHack
			};

			m_vLightScale =
			{
				pInitStruct->m_pLightScale->x,
				pInitStruct->m_pLightScale->y,
				pInitStruct->m_pLightScale->z
			};

			if (pInitStruct->m_pGlobalVertexTint->x != 0.0f ||
				pInitStruct->m_pGlobalVertexTint->y != 0.0f ||
				pInitStruct->m_pGlobalVertexTint->z != 0.0f)
			{
				m_vGlobalVertexTint =
				{
					pInitStruct->m_pGlobalVertexTint->x,
					pInitStruct->m_pGlobalVertexTint->y,
					pInitStruct->m_pGlobalVertexTint->z,
				};
			}
			else
			{
				m_vGlobalVertexTint = { 1.0f, 1.0f, 1.0f };
			}

#ifndef MODEL_PIXEL_LIGHTING

			m_vFogColorMPAndLMAnimCount.x = pInitStruct->m_pFogColorMP->x;
			m_vFogColorMPAndLMAnimCount.y = pInitStruct->m_pFogColorMP->y;
			m_vFogColorMPAndLMAnimCount.z = pInitStruct->m_pFogColorMP->z;
			*(uint32*)&m_vFogColorMPAndLMAnimCount.w = pInitStruct->m_dwLMAnims;

#else

			m_vFogColorMP =
			{
				pInitStruct->m_pFogColorMP->x,
				pInitStruct->m_pFogColorMP->y,
				pInitStruct->m_pFogColorMP->z
			};


			m_vDirLightDirAndLMAnimCount.x = pInitStruct->m_pDirLightDir->x;
			m_vDirLightDirAndLMAnimCount.y = pInitStruct->m_pDirLightDir->y;
			m_vDirLightDirAndLMAnimCount.z = pInitStruct->m_pDirLightDir->z;
			*(uint32*)&m_vDirLightDirAndLMAnimCount.w = pInitStruct->m_dwLMAnims;
#endif
			
			if (pInitStruct->m_pLMAnims != nullptr)
			{
				for (uint32 i = 0; i < pInitStruct->m_dwLMAnims; i++)
					m_aLMBlendData[i].Init(&pInitStruct->m_pLMAnims[i]);
			}
		}

		DirectX::XMFLOAT3A	m_vFogColor;
		DirectX::XMFLOAT4	m_vLightAddAndInvertHack;
		DirectX::XMFLOAT3A	m_vLightScale;

		DirectX::XMFLOAT3A	m_vGlobalVertexTint;

#ifndef MODEL_PIXEL_LIGHTING
		DirectX::XMFLOAT4	m_vFogColorMPAndLMAnimCount;
#else
		DirectX::XMFLOAT3A	m_vFogColorMP;
		DirectX::XMFLOAT4	m_vDirLightDirAndLMAnimCount;
#endif

		LMBlendData_PS		m_aLMBlendData[MAX_LIGHT_ANIMS_PER_BUFF];
	};

	XM_ALIGNED_STRUCT(16) struct DynamicLight_VPS
	{
		void Init(DynamicLight* pLight, bool bBrightenBlackLights)
		{
			m_vPosition =
			{
				pLight->m_Base.m_vPos.x, pLight->m_Base.m_vPos.y, pLight->m_Base.m_vPos.z
			};

			if (bBrightenBlackLights)
			{
				m_vColorAndRadius =
				{
					(pLight->m_Base.m_nColorR > 127) ? 
					(float)pLight->m_Base.m_nColorR * MATH_ONE_OVER_255 : 
					(float)(127 - ((127 - pLight->m_Base.m_nColorR) >> 1)) * MATH_ONE_OVER_255,

					(pLight->m_Base.m_nColorG > 127) ? 
					(float)pLight->m_Base.m_nColorG * MATH_ONE_OVER_255 :
					(float)(127 - ((127 - pLight->m_Base.m_nColorG) >> 1)) * MATH_ONE_OVER_255,

					(pLight->m_Base.m_nColorB > 127) ?
					(float)pLight->m_Base.m_nColorB * MATH_ONE_OVER_255 :
					(float)(127 - ((127 - pLight->m_Base.m_nColorB) >> 1)) * MATH_ONE_OVER_255,

					pLight->m_fLightRadius
				};
			}
			else
			{
				m_vColorAndRadius =
				{
					(float)pLight->m_Base.m_nColorR * MATH_ONE_OVER_255,
					(float)pLight->m_Base.m_nColorG * MATH_ONE_OVER_255,
					(float)pLight->m_Base.m_nColorB * MATH_ONE_OVER_255,
					pLight->m_fLightRadius
				};
			}
		}
		
		DirectX::XMFLOAT3A	m_vPosition;
		DirectX::XMFLOAT4	m_vColorAndRadius;
	};

	XM_ALIGNED_STRUCT(16) struct ModelShadowLight_VPS
	{
		void Init(const ModelShadowLight* pModelShadowLight)
		{
			m_vPositionAndRadius = pModelShadowLight->m_vPositionAndRadius;
		}

		DirectX::XMFLOAT4	m_vPositionAndRadius;
	};

	XM_ALIGNED_STRUCT(16) struct VPSInputDynamicLights_3D
	{
		void Init(const Array_PLTObject& aLight, const Array_ModelShadowLight& aModelShadowLight, bool bBrightenBlackLights)
		{
			for (uint32 i = 0; i < aLight.size(); i++)
				m_aLight[i].Init(aLight[i]->ToDynamicLight(), bBrightenBlackLights);

			for (uint32 i = 0; i < aModelShadowLight.size(); i++)
				m_aModelShadowLight[i].Init(&aModelShadowLight[i]);
		}
		
		DynamicLight_VPS		m_aLight[MAX_LIGHTS_PER_BUFF];
		ModelShadowLight_VPS	m_aModelShadowLight[MAX_MS_LIGHTS_PER_BUFF];
	};

	CRenderShader_Base();
	virtual ~CRenderShader_Base();

	ID3D11VertexShader* GetVertexShader() { return m_pVertexShader; }
	ID3D11PixelShader*	GetPixelShader() { return m_pPixelShader; }
	
	ID3D11InputLayout*	GetLayout() { return m_pLayout; }

	virtual bool	Init(ID3D11VertexShader* pVertexShader, ID3D11PixelShader* pPixelShader, ID3D10Blob* pVSBlob);

	virtual bool	ExtraInit(ID3D11GeometryShader* pGeometryShader) { return true; };
	
	virtual bool	Validate(uint32 dwFlags) = 0;

protected:

	ID3D11VertexShader*	m_pVertexShader;
	ID3D11PixelShader*	m_pPixelShader;

	ID3D11InputLayout*	m_pLayout;

	const char* m_szName;
};

#endif