#include "pch.h"

#include "draw_worldmodel.h"
#include "rendererconsolevars.h"
#include "tagnodes.h"
#include "common_stuff.h"
#include "globalmgr.h"
#include "d3d_draw.h"
#include "rendermodemgr.h"
#include "common_draw.h"
#include "d3d_shader_worldmodel.h"
#include "rendertargetmgr.h"
#include "rendertarget.h"
#include "draw_light.h"

static Array_InternalWorldModelVertex g_aVertexData;
static Array_UInt32 g_aIndexData;
static Array_LMVertexData g_aLightMapData;
static Array_InternalSkyPortalVertex g_aSkyPortalVertexData;
static Array_UInt16 g_aSkyPortalIndexData;

uint16 g_aSKyPortalBBoxIndexData[SKY_PORTAL_BBOX_INDICES] = 
{ 
	0, 2, 1, 1, 2, 3,
	4, 5, 6, 6, 5, 7,
	0, 4, 6, 6, 2, 0,
	1, 3, 5, 5, 3, 7,
	0, 1, 5, 5, 4, 0,
	2, 6, 3, 3, 6, 7
};

bool IsCameraInsideBsp(ViewParams* pParams, WorldBsp* pBsp)
{
	DirectX::XMVECTOR vMinBox = DirectX::XMLoadFloat3(PLTVECTOR_TO_PXMFLOAT3(&pBsp->m_vMinBox));
	DirectX::XMVECTOR vMaxBox = DirectX::XMLoadFloat3(PLTVECTOR_TO_PXMFLOAT3(&pBsp->m_vMaxBox));
	DirectX::XMVECTOR vPos = DirectX::XMLoadFloat3(PLTVECTOR_TO_PXMFLOAT3(&pParams->m_vPos));

	return (DirectX::XMVector3Greater(vPos, vMinBox) && DirectX::XMVector3Less(vPos, vMaxBox));
}

float DistanceToBspEst(ViewParams* pParams, WorldBsp* pBsp)
{
	DirectX::XMVECTOR vMinBox = DirectX::XMLoadFloat3(PLTVECTOR_TO_PXMFLOAT3(&pBsp->m_vMinBox));
	DirectX::XMVECTOR vMaxBox = DirectX::XMLoadFloat3(PLTVECTOR_TO_PXMFLOAT3(&pBsp->m_vMaxBox));
	DirectX::XMVECTOR vPos = DirectX::XMLoadFloat3(PLTVECTOR_TO_PXMFLOAT3(&pParams->m_vPos));
	DirectX::XMVECTOR vCenter = (vMaxBox + vMinBox) / 2.0f;

	if (DirectX::XMVector3Greater(vPos, vMinBox) && DirectX::XMVector3Less(vPos, vMaxBox))
		return 0.0f;

	return DirectX::XMVectorGetX(DirectX::XMVector3Length(vPos - vCenter) -
		DirectX::XMVector3Length(vCenter - vMaxBox));
}

static SurfaceEffect* FindSurfaceEffect(Surface* pSurface)
{
	SurfaceEffect* pEffect = g_pSceneDesc->m_pRenderContext->m_pMainWorld->m_pSurfaceEffects;

	if (pEffect == nullptr)
		return pEffect;

	while (pEffect != nullptr)
	{
		if (pSurface == pEffect->m_pSurface)
			return pEffect;

		pEffect = pEffect->m_pPrev;
	}

	return nullptr;
}

static bool IsMainSurfaceIdentical(CWorldModelData::TextureInfo* pTextureInfo, Surface* pSurface, 
	SurfaceEffect* pEffect)
{
	if (pTextureInfo->m_pSurface == pSurface)
		return true;

	if (pSurface->m_dwFlags & SURF_SPRITEANIMATE)
		return false;
	
	uint32 dwExtraModeFlags = (pSurface->m_dwFlags & SURF_PANNINGSKY) ? MODE_SKY_PAN : 0;
	if (pEffect != nullptr)
		dwExtraModeFlags |= MODE_SURFACE_FX;

	return (pTextureInfo->m_pSurface->m_pTexture == pSurface->m_pTexture && 
		pTextureInfo->m_dwExtraModeFlags == dwExtraModeFlags);
}

static bool IsExtraSurfaceIdentical(CWorldModelData::TextureInfo* pTextureInfo, Surface* pSurface)
{
	if (pTextureInfo->m_pSurface == pSurface)
		return true;

	if (pSurface->m_dwFlags & SURF_SPRITEANIMATE)
		return false;
	
	return (pTextureInfo->m_pSurface->m_pTexture->m_pLinkedTexture == pSurface->m_pTexture->m_pLinkedTexture);
}

static bool StoreTextureInfo(CWorldModelData::Array_TextureInfo& aBuffer, CWorldModelData::PolyTextures* pTextures,
	CWorldModelData::PolyTextureIndices* pIndices, SurfaceEffect* pEffect)
{
	uint32 dwNewItems = 2;
	bool bMainFlag = true;
	bool bExtraFlag = true;

	if (pTextures->m_pSurface->m_pTexture == nullptr)
	{
		dwNewItems--;
		bMainFlag = false;
	}

	if (!bMainFlag || pTextures->m_pSurface->m_pTexture->m_pLinkedTexture == nullptr)
	{
		dwNewItems--;
		bExtraFlag = false;
	}

	for (uint32 i = 0; i < aBuffer.size(); i++)
	{
		if (bMainFlag && aBuffer[i].m_eType == CWorldModelData::TT_MAIN && 
			IsMainSurfaceIdentical(&aBuffer[i], pTextures->m_pSurface, pEffect))
		{
			bMainFlag = false;
			dwNewItems--;
			pIndices->m_dwMainIndex = i;
		}

		if (bExtraFlag && aBuffer[i].m_eType != CWorldModelData::TT_MAIN &&
			IsExtraSurfaceIdentical(&aBuffer[i], pTextures->m_pSurface))
		{
			bExtraFlag = false;
			dwNewItems--;
			pIndices->m_dwExtraIndex = i;
		}
	}

	uint32 dwBufferSize = aBuffer.size();
	if (dwBufferSize + dwNewItems > WORLD_MODEL_TEXTURES_PER_DRAW)
		return false;

	if (bMainFlag)
	{
		pIndices->m_dwMainIndex = dwBufferSize;

		uint32 dwExtraModeFlags = 0;
		if (pEffect != nullptr)
			dwExtraModeFlags |= MODE_SURFACE_FX;

		if (pTextures->m_pSurface->m_dwFlags & SURF_PANNINGSKY)
			dwExtraModeFlags |= MODE_SKY_PAN;

		aBuffer.emplace_back(pTextures->m_pSurface, CWorldModelData::TT_MAIN, dwExtraModeFlags);
		dwBufferSize++;
	}

	if (bExtraFlag)
	{
		pIndices->m_dwExtraIndex = dwBufferSize;
		aBuffer.emplace_back(pTextures->m_pSurface, pTextures->m_pSurface->m_pTexture->m_eTexType);
		dwBufferSize++;
	}

	return true;
}

static inline CWorldModelData::RenderBlock* AddRenderBlock(CWorldModelData::Array_RenderBlock& aRenderBlock)
{
	aRenderBlock.emplace_back();
	CWorldModelData::RenderBlock* pResult = &aRenderBlock.back();
	pResult->m_aTextureInfo.reserve(WORLD_MODEL_TEXTURES_PER_DRAW);

	return pResult;
}

static inline bool IsVisibleSurface(Surface* pSurface)
{
	// TODO - more flags?
	return !(pSurface->m_dwFlags & SURF_INVISIBLE);
}

static inline bool IsSkyPortal(Surface* pSurface)
{
	return (pSurface->m_dwFlags & SURF_SKY);
}

static inline bool IsLightMappedSurface(Surface* pSurface)
{
	// TODO - directional light?
	return (pSurface->m_dwFlags & SURF_LIGHTMAP);
}

static inline bool IsTransparentSurface(Surface* pSurface)
{
	return (pSurface->m_dwFlags & SURF_TRANSPARENT);
}

static inline bool IsTexturedSurface(Surface* pSurface)
{
	return (pSurface->m_pTexture != nullptr);
}

static DirectX::XMFLOAT4X4* SetupExtraTextureTransform(SharedTexture* pTexture, DirectX::XMFLOAT4X4* pTransform)
{
	if (pTexture != nullptr && pTexture->m_pLinkedTexture != nullptr && 
		pTexture->m_eTexType == eSharedTexType_Detail)
	{
		RTexture* pRenderTexture = (RTexture*)pTexture->m_pRenderData;
		float fScale = pRenderTexture->m_fDetailTextureScale * g_CV_DetailTextureScale.m_Val;

		pTransform->m[0][0] = fScale * pRenderTexture->m_fDetailTextureAngleC;
		pTransform->m[0][1] = fScale * -pRenderTexture->m_fDetailTextureAngleS;
		pTransform->m[1][0] = fScale * pRenderTexture->m_fDetailTextureAngleS;
		pTransform->m[1][1] = fScale * pRenderTexture->m_fDetailTextureAngleC;

		return pTransform;
	}

	return nullptr;
}

static PolyAnimRef* FindPolyAnimRef(WorldPoly* pPoly, uint32 dwAnimIndex)
{
	for (uint32 dwRef = 0; dwRef < pPoly->m_dwPolyAnimRefs; dwRef++)
	{
		if (pPoly->m_pPolyAnimRefs[dwRef].m_wRefs[0] == dwAnimIndex)
			return &pPoly->m_pPolyAnimRefs[dwRef];
	}

	return nullptr;
}

static void PushLMVertex(Array_LMVertexData& aLightmapData, WorldPoly* pPoly, uint32 dwVert,
	CWorldModelData::PolyTextureIndices* pPolyTextureIndices)
{
	pPolyTextureIndices->m_dwLMDataStart = aLightmapData.size();
	LMPagedPoly* pPagedPoly = (LMPagedPoly*)pPoly->m_pLMData;

	uint32 dwAllFrame = 0;

	for (uint32 dwAnim = 0; dwAnim < g_pSceneDesc->m_pRenderContext->m_pMainWorld->GetLMAnimCount(); dwAnim++)
	{
		LMAnim* pAnim = g_pSceneDesc->m_pRenderContext->m_pMainWorld->GetLMAnim(dwAnim);
		PolyAnimRef* pRef = FindPolyAnimRef(pPoly, dwAnim);

		if (pRef != nullptr)
		{
			for (uint32 dwFrame = 0; dwFrame < pAnim->m_nFrames; dwFrame++)
			{
				LMFramePolyData& polyData = pAnim->m_pFrames[dwFrame].m_pPolyDataList[pRef->m_wRefs[1]];		
				LMPagedPoly::FrameInfo& frameInfo = pPagedPoly->m_aFrameInfo[dwAllFrame];

				uint32 dwFlags;
				if (IsLightMappedSurface(pPoly->m_pSurface) && frameInfo.m_dwPage != UINT32_MAX)
					dwFlags = LMVD_FLAG_USE_TEXTURE;
				else
					dwFlags = LMVD_FLAG_USE_COLORS;

				aLightmapData.emplace_back(
					(float)polyData.m_pReds[dwVert] * MATH_ONE_OVER_255,
					(float)polyData.m_pGreens[dwVert] * MATH_ONE_OVER_255,
					(float)polyData.m_pBlues[dwVert] * MATH_ONE_OVER_255,
					frameInfo.m_dwPage,
					dwFlags,
					frameInfo.m_fOffsetU,
					frameInfo.m_fOffsetV
				);

				dwAllFrame++;
			}
		}
		else
		{
			aLightmapData.insert(aLightmapData.end(), pAnim->m_nFrames, { });
			dwAllFrame += pAnim->m_nFrames;
		}
	}
}

static void GetBaseLightMapCoords(DirectX::XMFLOAT2* pCoords, WorldPoly* pPoly, uint32 dwVert,
	DirectX::XMFLOAT3* pLMPlaneVectorU, DirectX::XMFLOAT3* pLMPlaneVectorV)
{
	LMPagedPoly* pPagedPoly = (LMPagedPoly*)pPoly->m_pLMData;

	if (pPagedPoly != nullptr)
	{
		*pCoords = g_GlobalMgr.GetLightMapMgr()->SetupBaseLMVertexTexCoords(pPagedPoly, pPoly, dwVert,
			pLMPlaneVectorU, pLMPlaneVectorV);
	}
	else
	{
		*pCoords = { };
	}
}

static void InitWorldModelVertex(InternalWorldModelVertex* pVertex, WorldPoly* pPoly, SharedTexture* pTexture, 
	uint32 dwVert, CWorldModelData::PolyTextureIndices* pPolyTextureIndices, DirectX::XMFLOAT4X4* pExtraTexTransform,
	DirectX::XMFLOAT3* pLMPlaneVectorU, DirectX::XMFLOAT3* pLMPlaneVectorV)
{
	SPolyVertex& initVertex = pPoly->m_Vertices[dwVert];

	pVertex->vPosition = *PLTVECTOR_TO_PXMFLOAT3(&initVertex.m_pPoints->m_vVec);
	pVertex->vNormal = *PLTVECTOR_TO_PXMFLOAT3(&pPoly->m_pPlane->m_vNormal);

	pVertex->vDiffuseColor =
	{
		(float)initVertex.m_nColorR * MATH_ONE_OVER_255,
		(float)initVertex.m_nColorG * MATH_ONE_OVER_255,
		(float)initVertex.m_nColorB * MATH_ONE_OVER_255,
		(float)initVertex.m_nColorA * MATH_ONE_OVER_255
	};

	// TODO - looks fine without extra shading?
	/*if (pExtraShade != nullptr)
	{
		pVertex->vDiffuseColor =
		{
			((float)initVertex.m_nColorR * MATH_ONE_OVER_255) + pExtraShade->x,
			((float)initVertex.m_nColorG /* MATH_ONE_OVER_255) + pExtraShade->y,
			((float)initVertex.m_nColorB * MATH_ONE_OVER_255) + pExtraShade->z,
			(float)initVertex.m_nColorA * MATH_ONE_OVER_255
		};

		if (pVertex->vDiffuseColor.x > 1.0f)
			pVertex->vDiffuseColor.x = 1.0f;

		if (pVertex->vDiffuseColor.y > 1.0f)
			pVertex->vDiffuseColor.y = 1.0f;

		if (pVertex->vDiffuseColor.z > 1.0f)
			pVertex->vDiffuseColor.z = 1.0f;
	}
	else
	{
		pVertex->vDiffuseColor = { };
	}*/

	if (pTexture != nullptr)
	{
		RTexture* pRenderTexture = (RTexture*)pTexture->m_pRenderData;

		pVertex->vTexCoords.x = initVertex.m_fScaledU / (float)pRenderTexture->m_dwScaledWidth;
		pVertex->vTexCoords.y = initVertex.m_fScaledV / (float)pRenderTexture->m_dwScaledHeight;

		if (pExtraTexTransform != nullptr)
		{	
			RTexture* pRenderDetailTexture = (RTexture*)pTexture->m_pLinkedTexture->m_pRenderData;

			float fDetailU = initVertex.m_fScaledU / (float)pRenderDetailTexture->m_dwScaledWidth;
			float fDetaulV = initVertex.m_fScaledV / (float)pRenderDetailTexture->m_dwScaledHeight;

			pVertex->vExtraCoords.x = fDetailU * pExtraTexTransform->m[0][0] +
				fDetaulV * pExtraTexTransform->m[0][1];
			pVertex->vExtraCoords.y = fDetailU * pExtraTexTransform->m[1][0] +
				fDetaulV * pExtraTexTransform->m[1][1];
		}
		else
		{
			pVertex->vExtraCoords = pVertex->vTexCoords;
		}
	}
	else
	{
		pVertex->vTexCoords = { 0.0f, 0.0f };
		pVertex->vExtraCoords = { 0.0f, 0.0f };
	}

	GetBaseLightMapCoords(&pVertex->vLightMapCoords, pPoly, dwVert, pLMPlaneVectorU, pLMPlaneVectorV);

	pVertex->adwTextureIndices[WMTI_Main] = pPolyTextureIndices->m_dwMainIndex;
	pVertex->adwTextureIndices[WMTI_Extra] = pPolyTextureIndices->m_dwExtraIndex;
	pVertex->adwTextureIndices[WMTI_LMDataStart] = pPolyTextureIndices->m_dwLMDataStart;
	pVertex->adwTextureIndices[WMTI_VertexFXStart] = UINT32_MAX;
}

static void InitPerSubObjectShaderParams(CRenderShader_WorldModel::VPSPerSubObjectParams* pParams, 
	const CWorldModelData::TextureInfo* pTextureInfo, uint32 dwRenderMode)
{
	if (pTextureInfo->m_eType != CWorldModelData::TT_MAIN)
	{
		pParams->m_pTexture = 
			((RTexture*)pTextureInfo->m_pSurface->m_pTexture->m_pLinkedTexture->m_pRenderData)->m_pResourceView;
		return;
	}

	if (pTextureInfo->m_pSurface->m_pTexture == nullptr)
	{
		pParams->m_dwMode |= MODE_NO_TEXTURE;
		return;
	}

	SharedTexture* pTexture = pTextureInfo->m_pSurface->m_pTexture;
	RTexture* pRenderTexture = (RTexture*)pTexture->m_pRenderData;

	pParams->m_pTexture = pRenderTexture->m_pResourceView;
	pParams->m_dwMode = dwRenderMode;

	pParams->m_dwMode |= pTextureInfo->m_dwExtraModeFlags;

	if (g_pStruct->m_GlobalPanInfo->m_pTexture == nullptr)
		pParams->m_dwMode &= ~MODE_SKY_PAN;

	if (pRenderTexture->m_dwFlags & RT_FULLBRITE)
		pParams->m_dwMode |= MODE_FULL_BRITE;

	if ((pTextureInfo->m_pSurface->m_dwFlags & SURF_BRIGHT) || g_CV_LMFullBright.m_Val)
		pParams->m_dwMode |= MODE_NO_LIGHT;

	if (pTexture->m_pStateChange != nullptr)
	{
		BlendState eBlendStateOverride = BLEND_STATE_Invalid;
		StencilState eStencilStateOverride = STENCIL_STATE_Invalid;

		g_RenderModeMgr.ApplyStateChange(pTexture->m_pStateChange, eBlendStateOverride, eStencilStateOverride,
			pParams->m_dwMode, &pParams->m_vModeColor);

		if (eBlendStateOverride != BLEND_STATE_Invalid)
			g_RenderStateMgr.SetBlendState(eBlendStateOverride);

		if (eStencilStateOverride != STENCIL_STATE_Invalid)
			g_RenderStateMgr.SetStencilState(eStencilStateOverride);
	}
	
	if (pTexture->m_pLinkedTexture != nullptr)
	{
		switch (pTexture->m_eTexType)
		{
			case eSharedTexType_Detail:
			{
				pParams->m_dwMode |= MODE_DETAIL_TEXTURE; 
				break;
			}
			case eSharedTexType_EnvMap: 
			{
				if (g_CV_EnvMapWorld.m_Val)
					pParams->m_dwMode |= MODE_ENV_MAP; 

				break;
			}
			case eSharedTexType_EnvMapAlpha:
			{
				if (g_CV_EnvMapWorld.m_Val)
					pParams->m_dwMode |= (MODE_ENV_MAP | MODE_ENV_MAP_ALPHA); 

				break;
			}
		}
	}

	constexpr float c_fDefaultAlphaRef = 0.5f + (0.5f / 255.0f);
	float fAlphaRef = pRenderTexture->m_fAlphaRef;
	if ((dwRenderMode & MODE_CHROMAKEY) && fAlphaRef == 0.0f)
		fAlphaRef = c_fDefaultAlphaRef;

	pParams->m_fAlphaRef = fAlphaRef;
}

static uint32 InitDynamicLightIndicesEx(uint32* pIndices, CWorldModelData::Array_BBox& aBBox)
{
	uint32 dwCounter = 0;
	AllocSet* pSet = d3d_GetVisibleSet()->GetLights();
	
	for (CWorldModelData::BBox& bbox : aBBox)
	{
		for (uint32 i = 0; i < pSet->GetObjectCount(); i++)
		{
			DynamicLight* pLight = pSet->GetObjectByIndex(i)->ToDynamicLight();

			if (!g_CV_BlackDynamicLight.m_Val && pLight->IsBlackLight())
				continue;

			DirectX::XMFLOAT3 vLightMinBBox;
			DirectX::XMFLOAT3 vLightMaxBBox;

			DirectX::XMVECTOR vRadius = DirectX::XMVectorReplicate(pLight->m_fLightRadius);
			DirectX::XMVECTOR vPos = DirectX::XMLoadFloat3(PLTVECTOR_TO_PXMFLOAT3(&pLight->m_Base.m_vPos));

			DirectX::XMStoreFloat3(&vLightMinBBox, vPos - vRadius);
			DirectX::XMStoreFloat3(&vLightMaxBBox, vPos + vRadius);

			if (!(pLight->m_Base.m_dwFlags & FLAG_ONLYLIGHTOBJECTS) &&
				Vector_DoBoxesTouch(&vLightMinBBox, &vLightMaxBBox, &bbox.m_vMin, &bbox.m_vMax))
			{
				pIndices[dwCounter] = i;
				dwCounter++;

				if (dwCounter == MAX_LIGHTS_PER_SUB_WORLD_MODEL)
					return dwCounter;
			}
		}
	}

	return dwCounter;
}

static uint32 InitModelShadowLightIndicesEx(uint32* pIndices, CWorldModelData::Array_BBox& aBBox)
{
	uint32 dwCounter = 0;

	for (CWorldModelData::BBox& bbox : aBBox)
	{
		for (uint32 i = 0; i < g_aModelShadowLight.size(); i++)
		{
			ModelShadowLight& light = g_aModelShadowLight[i];

			DirectX::XMFLOAT3 vLightMinBBox;
			DirectX::XMFLOAT3 vLightMaxBBox;

			DirectX::XMVECTOR vRadius = DirectX::XMVectorReplicate(light.m_vPositionAndRadius.w);
			DirectX::XMVECTOR vPos = DirectX::XMLoadFloat4(&light.m_vPositionAndRadius);

			DirectX::XMStoreFloat3(&vLightMinBBox, vPos - vRadius);
			DirectX::XMStoreFloat3(&vLightMaxBBox, vPos + vRadius);

			if (Vector_DoBoxesTouch(&vLightMinBBox, &vLightMaxBBox, &bbox.m_vMin, &bbox.m_vMax))
			{
				pIndices[dwCounter] = i;
				dwCounter++;

				if (dwCounter == MAX_MS_LIGHTS_PER_SUB_WORLD_MODEL)
					return dwCounter;
			}
		}
	}

	return dwCounter;
}

static uint32 InitDynamicLightIndices(uint32* pIndices, DirectX::XMFLOAT3* pRBMinBBox, DirectX::XMFLOAT3* pRBMaxBBox)
{
	uint32 dwCounter = 0;
	AllocSet* pSet = d3d_GetVisibleSet()->GetLights();
	
	for (uint32 i = 0; i < pSet->GetObjectCount(); i++)
	{
		DynamicLight* pLight = pSet->GetObjectByIndex(i)->ToDynamicLight();

		if (!g_CV_BlackDynamicLight.m_Val && pLight->IsBlackLight())
			continue;

		DirectX::XMFLOAT3 vLightMinBBox;
		DirectX::XMFLOAT3 vLightMaxBBox;

		DirectX::XMVECTOR vRadius = DirectX::XMVectorReplicate(pLight->m_fLightRadius);
		DirectX::XMVECTOR vPos = DirectX::XMLoadFloat3(PLTVECTOR_TO_PXMFLOAT3(&pLight->m_Base.m_vPos));

		DirectX::XMStoreFloat3(&vLightMinBBox, vPos - vRadius);
		DirectX::XMStoreFloat3(&vLightMaxBBox, vPos + vRadius);

		if (!(pLight->m_Base.m_dwFlags & FLAG_ONLYLIGHTOBJECTS) &&
			Vector_DoBoxesTouch(&vLightMinBBox, &vLightMaxBBox, pRBMinBBox, pRBMaxBBox))
		{
			pIndices[dwCounter] = i;
			dwCounter++;

			if (dwCounter == MAX_LIGHTS_PER_SUB_WORLD_MODEL)
				return dwCounter;
		}
	}

	return dwCounter;
}

static uint32 InitModelShadowLightIndices(uint32* pIndices, DirectX::XMFLOAT3* pRBMinBBox, DirectX::XMFLOAT3* pRBMaxBBox)
{
	uint32 dwCounter = 0;

	for (uint32 i = 0; i < g_aModelShadowLight.size(); i++)
	{
		ModelShadowLight& light = g_aModelShadowLight[i];

		DirectX::XMFLOAT3 vLightMinBBox;
		DirectX::XMFLOAT3 vLightMaxBBox;

		DirectX::XMVECTOR vRadius = DirectX::XMVectorReplicate(light.m_vPositionAndRadius.w);
		DirectX::XMVECTOR vPos = DirectX::XMLoadFloat4(&light.m_vPositionAndRadius);

		DirectX::XMStoreFloat3(&vLightMinBBox, vPos - vRadius);
		DirectX::XMStoreFloat3(&vLightMaxBBox, vPos + vRadius);

		if (Vector_DoBoxesTouch(&vLightMinBBox, &vLightMaxBBox, pRBMinBBox, pRBMaxBBox))
		{
			pIndices[dwCounter] = i;
			dwCounter++;

			if (dwCounter == MAX_MS_LIGHTS_PER_SUB_WORLD_MODEL)
				return dwCounter;
		}
	}

	return dwCounter;
}

static inline void XM_CALLCONV TransformBBox(DirectX::XMFLOAT3* pPos, DirectX::XMMATRIX mTransform, 
	CWorldModelData::RenderBlock& renderBlock, DirectX::XMFLOAT3* pMinBBox, DirectX::XMFLOAT3* pMaxBBox)
{
	DirectX::XMVECTOR vPos = DirectX::XMVectorSet(pPos->x, pPos->y, pPos->z, 1.0f);
	DirectX::XMVECTOR vDiffPos = vPos - DirectX::XMVector4Transform(vPos, mTransform);

	DirectX::XMStoreFloat3(pMinBBox, DirectX::XMLoadFloat3(&renderBlock.m_BBox.m_vMin) + vDiffPos);
	DirectX::XMStoreFloat3(pMaxBBox, DirectX::XMLoadFloat3(&renderBlock.m_BBox.m_vMax) + vDiffPos);
}

static void GenerateNormals(Array_InternalWorldModelVertex& aVertexData, Array_UInt16 aIndexData)
{
	for (uint32 i = 0; i < aIndexData.size(); i += 3)
	{
		DirectX::XMVECTOR v0 = DirectX::XMLoadFloat3(&aVertexData[aIndexData[i]].vPosition);
		DirectX::XMVECTOR v1 = DirectX::XMLoadFloat3(&aVertexData[aIndexData[i + 1]].vPosition) - v0;
		DirectX::XMVECTOR v2 = DirectX::XMLoadFloat3(&aVertexData[aIndexData[i + 2]].vPosition) - v0;

		DirectX::XMVECTOR vFaceNormal = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(v1, v2));
		
		DirectX::XMFLOAT3* pNormal0 = &aVertexData[aIndexData[i]].vNormal;
		DirectX::XMFLOAT3* pNormal1 = &aVertexData[aIndexData[i + 1]].vNormal;
		DirectX::XMFLOAT3* pNormal2 = &aVertexData[aIndexData[i + 2]].vNormal;

		DirectX::XMVECTOR vNormal0 = DirectX::XMLoadFloat3(pNormal0);
		DirectX::XMVECTOR vNormal1 = DirectX::XMLoadFloat3(pNormal1);
		DirectX::XMVECTOR vNormal2 = DirectX::XMLoadFloat3(pNormal2);

		DirectX::XMStoreFloat3(pNormal0, vNormal0 + vFaceNormal);
		DirectX::XMStoreFloat3(pNormal1, vNormal1 + vFaceNormal);
		DirectX::XMStoreFloat3(pNormal2, vNormal2 + vFaceNormal);
	}

	for (uint32 i = 0; i < aVertexData.size(); i++)
	{
		DirectX::XMFLOAT3* pNormal = &aVertexData[i].vNormal;
		DirectX::XMVECTOR vNormal = DirectX::XMLoadFloat3(pNormal);

		DirectX::XMStoreFloat3(pNormal, DirectX::XMVector3Normalize(vNormal));
	}
}

CWorldModelData::~CWorldModelData()
{
	RELEASE_INTERFACE(m_pVertexBuffer, g_szFreeError_VB);
	RELEASE_INTERFACE(m_pIndexBuffer, g_szFreeError_IB);
	RELEASE_INTERFACE(m_pLMVertexData, g_szFreeError_SRV);

	for (RenderBlock& block : m_aRenderBlock)
	{
		RELEASE_INTERFACE(block.m_pVertexFXView, g_szFreeError_SRV);
		RELEASE_INTERFACE(block.m_pVertexFXBuffer, g_szFreeError_VB);
	}

	m_aRenderBlock.clear();
}

static inline void XM_CALLCONV ResetBBoxVectors(XMVECTOR* pMin, XMVECTOR* pMax)
{
	*pMin = { FLT_MAX, FLT_MAX, FLT_MAX };
	*pMax = { -FLT_MAX, -FLT_MAX, -FLT_MAX };
}

bool CWorldModelData::RenderBlock::CreateVertexFXBuffer()
{	
	D3D11_BUFFER_DESC desc = { };

	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.ByteWidth = m_dwVertexCount * sizeof(InternalWorldModelVertexFX);
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	desc.StructureByteStride = sizeof(InternalWorldModelVertexFX);

	ID3D11Buffer* pBuffer;
	HRESULT hResult = g_D3DDevice.GetDevice()->CreateBuffer(&desc, nullptr, &pBuffer);
	if (FAILED(hResult))
		return false;

	D3D11_SHADER_RESOURCE_VIEW_DESC resourceDesc = { };
	resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	resourceDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	resourceDesc.Buffer.FirstElement = 0;
	resourceDesc.Buffer.NumElements = m_dwVertexCount;

	ID3D11ShaderResourceView* pResourceView;
	hResult = g_D3DDevice.GetDevice()->CreateShaderResourceView(pBuffer, &resourceDesc, &pResourceView);
	if (FAILED(hResult))
	{
		pBuffer->Release();
		return false;
	}

	m_pVertexFXBuffer = pBuffer;
	m_pVertexFXView = pResourceView;

	return true;
}

bool CWorldModelData::RenderBlock::UpdateVertexFXBuffer()
{
	auto lambdaUpdateVS = [this](D3D11_MAPPED_SUBRESOURCE* pSubResource)
		{
			InternalWorldModelVertexFX* pData = (InternalWorldModelVertexFX*)pSubResource->pData;

			for (uint32 dwVert = 0; dwVert < m_aVertexFXData.size() ; dwVert++)
			{
				VertexFX& vertexFX = m_aVertexFXData[dwVert];
				
				if (vertexFX.m_pTexture == nullptr)
				{
					pData[dwVert].vTexCoords.x = 0.0f;
					pData[dwVert].vTexCoords.y = 0.0f;

					continue;
				}
			
				float fScaledWidth = (float)((RTexture*)(vertexFX.m_pTexture->m_pRenderData))->m_dwScaledWidth;
				float fScaledHeight = (float)((RTexture*)(vertexFX.m_pTexture->m_pRenderData))->m_dwScaledHeight;

				pData[dwVert].vTexCoords.x = vertexFX.m_pVertex->m_fScaledU / fScaledWidth;
				pData[dwVert].vTexCoords.y = vertexFX.m_pVertex->m_fScaledV / fScaledHeight;
			}
		};

	if (!g_GlobalMgr.UpdateGenericBufferEx(m_pVertexFXBuffer, D3D11_MAP_WRITE_DISCARD, lambdaUpdateVS))
		return false;

	return true;
}

bool CWorldModelData::Init(WorldBsp* pBsp)
{
#if defined(DEBUG) || defined(_DEBUG)
	strcpy_s(m_szName, pBsp->m_szWorldName);
#endif

	g_aVertexData.clear();
	g_aIndexData.clear();
	g_aLightMapData.clear();
	
	bool bFullyInvisible = true;
	bool bFullyUntextured = true;
	uint32 dwLMVerts = 0;

	for (uint32 dwPoly = 0; dwPoly < pBsp->m_dwPolies; dwPoly++)
	{
		WorldPoly* pPoly = pBsp->m_pPolies[dwPoly];
		uint32 dwVerts = g_CV_FixTJunc.m_Val ? pPoly->m_wNumFixVerts : pPoly->m_wNumVerts;

		m_dwVerts += dwVerts;
		m_dwIndices += (dwVerts - 2) * 3;

		if (pPoly->m_dwPolyAnimRefs)
			dwLMVerts += dwVerts;

		// TODO - check surfaces instead?
		bFullyInvisible = bFullyInvisible &&
			(!IsVisibleSurface(pPoly->m_pSurface) || IsSkyPortal(pPoly->m_pSurface));

		bFullyUntextured = bFullyUntextured && (!IsVisibleSurface(pPoly->m_pSurface) || !IsTexturedSurface(pPoly->m_pSurface));
	}

	if (bFullyInvisible)
		return true;

	if (bFullyUntextured)
		m_fAlphaMod = g_CV_UntexturedTWMAlphaMod.m_Val;

	g_aVertexData.reserve(m_dwVerts);
	g_aIndexData.reserve(m_dwIndices);
	g_aLightMapData.reserve(dwLMVerts * g_GlobalMgr.GetLightMapMgr()->GetAllFrameCount());

	// TODO - adequate reserve
	m_aRenderBlock.reserve((pBsp->m_dwTextures << 1) / WORLD_MODEL_TEXTURES_PER_DRAW + 1);
	RenderBlock* pCurrRB = AddRenderBlock(m_aRenderBlock);

	uint32 dwBatchIndexPos = 0;
	uint32 dwBatchIndexCount = 0;
	uint32 dwBatchVertexCount = 0;

	SurfaceEffect* pPrevSurfaceFX = nullptr;

	DirectX::XMVECTOR vMinBBox;
	DirectX::XMVECTOR vMaxBBox;
	ResetBBoxVectors(&vMinBBox, &vMaxBBox);

	for (uint32 dwPoly = 0; dwPoly < pBsp->m_dwPolies; dwPoly++)
	{
		WorldPoly* pPoly = pBsp->m_pPolies[dwPoly];

		uint32 dwNumVerts, dwStartVert;
		if (g_CV_FixTJunc.m_Val && pPoly->m_wNumVerts != pPoly->m_wNumFixVerts)
		{
			dwStartVert = pPoly->m_wNumVerts;
			dwNumVerts = pPoly->m_wNumFixVerts;
		}
		else
		{
			dwStartVert = 0;
			dwNumVerts = pPoly->m_wNumVerts;
		}
	
		Surface* pSurface = pPoly->m_pSurface;

		// TODO - decrease amount of vertices/indices?
		if (!IsVisibleSurface(pSurface) || IsSkyPortal(pSurface))
			continue;

		PolyTextures sPolyTextures = { pSurface };
		PolyTextureIndices sPolyTextureIndices;

		DirectX::XMFLOAT4X4 mExtraTexTransform;
		DirectX::XMFLOAT4X4* pExtraTexTransform = SetupExtraTextureTransform(pSurface->m_pTexture, &mExtraTexTransform);

		SurfaceEffect* pSurfaceFX = FindSurfaceEffect(pSurface);

		if (!StoreTextureInfo(pCurrRB->m_aTextureInfo, &sPolyTextures, &sPolyTextureIndices, pSurfaceFX))
		{
			pCurrRB->SetBBox(vMinBBox, vMaxBBox);
			pCurrRB->OptimizeExtraBBoxes();
			pCurrRB->SetVertexData(dwBatchIndexPos, dwBatchIndexCount, dwBatchVertexCount);

			ResetBBoxVectors(&vMinBBox, &vMaxBBox);

			dwBatchIndexPos += dwBatchIndexCount;
			dwBatchIndexCount = 0;
			dwBatchVertexCount = 0;

			pCurrRB = AddRenderBlock(m_aRenderBlock);

			sPolyTextureIndices.Clear();
			StoreTextureInfo(pCurrRB->m_aTextureInfo, &sPolyTextures, &sPolyTextureIndices, pSurfaceFX);
		}

		DirectX::XMFLOAT3 vLMPlaneVectorU;
		DirectX::XMFLOAT3 vLMPlaneVectorV;

		if (IsValidLMPlane(pPoly) && IsLightMappedSurface(pSurface))
		{
			uint32 dwLMPlaneIndex = GetLMPlaneIndex(pPoly);		

			SetupLMPlaneVectors(dwLMPlaneIndex, PLTVECTOR_TO_PXMFLOAT3(&pPoly->m_pPlane->m_vNormal),
				&vLMPlaneVectorU, &vLMPlaneVectorV);
		}

		// TODO - looks fine without extra shading?
		/*DirectX::XMFLOAT3 vExtraShade;
		DirectX::XMFLOAT3* pExtraShade = nullptr;
		if (IsTransparentSurface(pSurface))
		{
			pExtraShade = &vExtraShade;

			DoLightLookup_Internal(&g_pSceneDesc->m_pRenderContext->m_pMainWorld->m_LightTable, 
				PLTVECTOR_TO_PXMFLOAT3(&pPoly->m_vData_24), pExtraShade);
		}*/

		uint32 dwIndexStart = g_aVertexData.size();

		DirectX::XMVECTOR vExtraMinBBox;
		DirectX::XMVECTOR vExtraMaxBBox;
		ResetBBoxVectors(&vExtraMinBBox, &vExtraMaxBBox);

		for (uint32 dwVert = 0; dwVert < dwNumVerts; dwVert++)
		{
			InternalWorldModelVertex currVert;

			if (pPoly->m_dwPolyAnimRefs)
				PushLMVertex(g_aLightMapData, pPoly, dwStartVert + dwVert, &sPolyTextureIndices);

			InitWorldModelVertex(&currVert, pPoly, pSurface->m_pTexture, dwStartVert + dwVert, &sPolyTextureIndices,
				pExtraTexTransform, &vLMPlaneVectorU, &vLMPlaneVectorV);

			DirectX::XMVECTOR vVertPos = DirectX::XMLoadFloat3(&currVert.vPosition);
			vMinBBox = DirectX::XMVectorMin(vVertPos, vMinBBox);
			vMaxBBox = DirectX::XMVectorMax(vVertPos, vMaxBBox);
			vExtraMinBBox = DirectX::XMVectorMin(vVertPos, vExtraMinBBox);
			vExtraMaxBBox = DirectX::XMVectorMax(vVertPos, vExtraMaxBBox);

			if (pSurfaceFX != nullptr)
			{
				currVert.adwTextureIndices[WMTI_VertexFXStart] = pCurrRB->m_aVertexFXData.size();
				pCurrRB->m_aVertexFXData.emplace_back(pSurface->m_pTexture, &pPoly->m_Vertices[dwStartVert + dwVert]);
			}

			g_aVertexData.push_back(currVert);

			if (dwVert > 2)
			{
				g_aIndexData.push_back(dwIndexStart);
				g_aIndexData.push_back(dwIndexStart + dwVert - 1);

				dwBatchIndexCount += 2;
			}

			g_aIndexData.push_back(dwIndexStart + dwVert);

			dwBatchIndexCount++;
		}

		dwBatchVertexCount += dwNumVerts;

		pCurrRB->AppendExtraBBox(vExtraMinBBox, vExtraMaxBBox);
	}

	pCurrRB->SetBBox(vMinBBox, vMaxBBox);
	pCurrRB->OptimizeExtraBBoxes();	
	pCurrRB->SetVertexData(dwBatchIndexPos, dwBatchIndexCount, dwBatchVertexCount);

	//GenerateNormals(aVertexData, aIndexData);

	// TEST
	/*if (pBsp->m_dwWorldInfoFlags & WIF_PHYSICSBSP)
	{
		for (uint32 i = 0; i < m_aRenderBlock.size(); i++)
		{
			CWorldModelData::RenderBlock& renderBlock = m_aRenderBlock[i];
			AddDebugMessage(0, "[%d] %d", i, renderBlock.m_aExtraBBox.size());
		}
	}*/

	return CreateVertexBuffer(g_aVertexData.data(), m_dwVerts) &&
		CreateIndexBuffer(g_aIndexData.data(), m_dwIndices) &&
		CreateLightmapDataBuffer(g_aLightMapData.data(), g_aLightMapData.size());
}

void XM_CALLCONV CWorldModelData::RenderBlock::AppendExtraBBox(DirectX::FXMVECTOR vMin, DirectX::FXMVECTOR vMax)
{
	if (!m_aExtraBBox.empty())
	{		
		float fMargin = g_CV_RBExtraBBoxMargin.m_Val;

		for (BBox& bbox : m_aExtraBBox)
		{
			DirectX::XMFLOAT3 vMinTemp;
			DirectX::XMFLOAT3 vMaxTemp;

			DirectX::XMStoreFloat3(&vMinTemp, vMin - DirectX::XMVectorReplicate(fMargin));
			DirectX::XMStoreFloat3(&vMaxTemp, vMax + DirectX::XMVectorReplicate(fMargin));

			if (Vector_DoBoxesTouch(&vMinTemp, &vMaxTemp, &bbox.m_vMin, &bbox.m_vMax))
			{
				DirectX::XMStoreFloat3(&bbox.m_vMin,
					DirectX::XMVectorMin(DirectX::XMLoadFloat3(&bbox.m_vMin), vMin));
				DirectX::XMStoreFloat3(&bbox.m_vMax,
					DirectX::XMVectorMax(DirectX::XMLoadFloat3(&bbox.m_vMax), vMax));

				return;
			}
		}
	}

	AddExtraBBox(vMin, vMax);
}

void CWorldModelData::RenderBlock::OptimizeExtraBBoxes()
{
	if (m_aExtraBBox.size() == 1)
		return;

	float fMargin = g_CV_RBExtraBBoxMargin.m_Val;

LABEL_OptimizeExtraBBoxes_start:

	uint32 i = 0;
	while (i < m_aExtraBBox.size())
	{
		BBox& bbox1 = m_aExtraBBox[i];

		uint32 j = 0;
		while (j < m_aExtraBBox.size())
		{
			if (i == j)
			{
				j++;
				continue;
			}

			BBox& bbox2 = m_aExtraBBox[j];

			DirectX::XMFLOAT3 vMinTemp;
			DirectX::XMFLOAT3 vMaxTemp;

			DirectX::XMStoreFloat3(&vMinTemp, DirectX::XMLoadFloat3(&bbox1.m_vMin) -
				DirectX::XMVectorReplicate(fMargin));
			DirectX::XMStoreFloat3(&vMaxTemp, DirectX::XMLoadFloat3(&bbox1.m_vMax) +
				DirectX::XMVectorReplicate(fMargin));

			if (Vector_DoBoxesTouch(&vMinTemp, &vMaxTemp, &bbox2.m_vMin, &bbox2.m_vMax))
			{
				DirectX::XMStoreFloat3(&bbox1.m_vMin, DirectX::XMVectorMin(DirectX::XMLoadFloat3(&bbox1.m_vMin),
					DirectX::XMLoadFloat3(&bbox2.m_vMin)));
				DirectX::XMStoreFloat3(&bbox1.m_vMax, DirectX::XMVectorMax(DirectX::XMLoadFloat3(&bbox1.m_vMax),
					DirectX::XMLoadFloat3(&bbox2.m_vMax)));

				m_aExtraBBox[j] = m_aExtraBBox.back();
				m_aExtraBBox.pop_back();
				
				goto LABEL_OptimizeExtraBBoxes_start;
			}

			j++;
		}

		i++;
	}
}

bool CWorldModelData::RenderBlock::IsVisibleInFrustum(ViewParams* pParams)
{
	for (BBox& bbox : m_aExtraBBox)
	{
		if (g_RenderWorld.IsAABBVisible(pParams, &bbox.m_vMin, &bbox.m_vMax))
			return true;
	}

	return false;
}

bool CWorldModelData::CreateVertexBuffer(InternalWorldModelVertex* pData, uint32 dwSize)
{
	m_pVertexBuffer = g_GlobalMgr.CreateVertexBuffer(pData, sizeof(InternalWorldModelVertex) * dwSize,
		D3D11_USAGE_IMMUTABLE, 0);

	DXGI_FORMAT_R16_UINT;

	return (m_pVertexBuffer != nullptr);
}

bool CWorldModelData::CreateIndexBuffer(uint32* pData, uint32 dwSize)
{
	m_pIndexBuffer = g_GlobalMgr.CreateIndexBuffer(pData, sizeof(uint32) * dwSize, D3D11_USAGE_IMMUTABLE, 0);
	return (m_pIndexBuffer != nullptr);
}

bool CWorldModelData::CreateLightmapDataBuffer(LMVertexData* pData, uint32 dwSize)
{
	m_pLMVertexData = g_GlobalMgr.GetLightMapMgr()->CreateLMVertexDataBuffer(pData, dwSize);
	return (m_pLMVertexData != nullptr);
}

void CWorldModelManager::FreeAllData()
{
	RELEASE_INTERFACE(m_pSkyPortal_VB, g_szFreeError_VB);
	RELEASE_INTERFACE(m_pSkyPortal_IB, g_szFreeError_IB);
	RELEASE_INTERFACE(m_pSkyPortalBBox_VB, g_szFreeError_VB);
	RELEASE_INTERFACE(m_pSkyPortalBBox_IB, g_szFreeError_IB);

	CExpirableDataManager2::FreeAllData();
}

CWorldModelData* CWorldModelManager::GetWorldModelData(WorldBsp* pBsp)
{
	CWorldModelData* pData = m_Data[pBsp->m_wIndex];

	if (pData != nullptr)
	{
		pData->SetLastUpdate(g_fLastClientTime);

		return pData;
	}

	CWorldModelData* pNewData = new CWorldModelData();

	if (!pNewData->Init(pBsp))
	{
		delete pNewData;
		return nullptr;
	}

	pNewData->SetLastUpdate(g_fLastClientTime);
	m_Data[pBsp->m_wIndex] = pNewData;

	return pNewData;
}

void CWorldModelManager::ReserveData()
{
	m_Data.resize(g_pSceneDesc->m_pRenderContext->m_pMainWorld->GetWorldModelCount());

	g_aModelShadowLight.reserve(MODEL_SHADOW_LIGHTS_RESERVE);

	InitSkyPortalBuffers();
}

bool CWorldModelManager::InitSkyPortalBuffers()
{
	g_aSkyPortalVertexData.clear();
	g_aSkyPortalIndexData.clear();
	m_dwSkyPortalVerts = 0;
	m_dwSkyPortalIndices = 0;

	// TODO - can all world models contain sky portals or PhysicsBSP only?
	WorldBsp* pBsp = g_pSceneDesc->m_pRenderContext->m_pMainWorld->GetSpecificBsp(WIF_PHYSICSBSP);

	DirectX::XMVECTOR vMinBBox;
	DirectX::XMVECTOR vMaxBBox;
	ResetBBoxVectors(&vMinBBox, &vMaxBBox);

	for (uint32 dwPoly = 0; dwPoly < pBsp->m_dwPolies; dwPoly++)
	{
		WorldPoly* pPoly = pBsp->m_pPolies[dwPoly];

		if (IsSkyPortal(pPoly->m_pSurface))
		{
			uint32 dwNumVerts = g_CV_FixTJunc.m_Val ? pPoly->m_wNumFixVerts : pPoly->m_wNumVerts;

			m_dwSkyPortalVerts += dwNumVerts;
			m_dwSkyPortalIndices += (dwNumVerts - 2) * 3;

			uint32 dwIndexStart = g_aSkyPortalVertexData.size();

			for (uint32 dwVert = 0; dwVert < dwNumVerts; dwVert++)
			{
				InternalSkyPortalVertex currVert;
				SPolyVertex& initVertex = pPoly->m_Vertices[dwVert];

				currVert.vPosition = *PLTVECTOR_TO_PXMFLOAT3(&initVertex.m_pPoints->m_vVec);

				DirectX::XMVECTOR vVertPos = DirectX::XMLoadFloat3(&currVert.vPosition);
				vMinBBox = DirectX::XMVectorMin(vVertPos, vMinBBox);
				vMaxBBox = DirectX::XMVectorMax(vVertPos, vMaxBBox);

				g_aSkyPortalVertexData.push_back(currVert);

				if (dwVert > 2)
				{
					g_aSkyPortalIndexData.push_back((uint16)(dwIndexStart));
					g_aSkyPortalIndexData.push_back((uint16)(dwIndexStart + dwVert - 1));
				}

				g_aSkyPortalIndexData.push_back((uint16)(dwIndexStart + dwVert));
			}
		}
	}

	if (!m_dwSkyPortalVerts)
		return true;

	SetSkyPortalBBox(vMinBBox, vMaxBBox);

	InternalSkyPortalVertex aBBoxVertexData[SKY_PORTAL_BBOX_VERTICES];

	aBBoxVertexData[0].vPosition = { m_SkyPortalBBox.m_vMin.x, m_SkyPortalBBox.m_vMax.y, m_SkyPortalBBox.m_vMin.z };
	aBBoxVertexData[1].vPosition = { m_SkyPortalBBox.m_vMax.x, m_SkyPortalBBox.m_vMax.y, m_SkyPortalBBox.m_vMin.z };
	aBBoxVertexData[2].vPosition = m_SkyPortalBBox.m_vMin;
	aBBoxVertexData[3].vPosition = { m_SkyPortalBBox.m_vMax.x, m_SkyPortalBBox.m_vMin.y, m_SkyPortalBBox.m_vMin.z };
	aBBoxVertexData[4].vPosition = { m_SkyPortalBBox.m_vMin.x, m_SkyPortalBBox.m_vMax.y, m_SkyPortalBBox.m_vMax.z };
	aBBoxVertexData[5].vPosition = m_SkyPortalBBox.m_vMax;
	aBBoxVertexData[6].vPosition = { m_SkyPortalBBox.m_vMin.x, m_SkyPortalBBox.m_vMin.y, m_SkyPortalBBox.m_vMax.z };
	aBBoxVertexData[7].vPosition = { m_SkyPortalBBox.m_vMax.x, m_SkyPortalBBox.m_vMin.y, m_SkyPortalBBox.m_vMax.z };

	return CreateSkyPortalVertexBuffer(g_aSkyPortalVertexData.data(), m_dwSkyPortalVerts) &&
		CreateSkyPortalIndexBuffer(g_aSkyPortalIndexData.data(), m_dwSkyPortalIndices) &&
		CreateSkyPortalBBoxVertexBuffer(aBBoxVertexData, SKY_PORTAL_BBOX_VERTICES) &&
		CreateSkyPortalBBoxIndexBuffer(g_aSKyPortalBBoxIndexData, SKY_PORTAL_BBOX_INDICES);
}

void CWorldModelManager::SetSkyPortalBuffersAndTopology()
{
	g_RenderShaderMgr.SetVertexResource(VRS_Main, m_pSkyPortal_VB, sizeof(InternalSkyPortalVertex), 0);
	g_RenderShaderMgr.SetIndexBuffer16(m_pSkyPortal_IB, 0);
	g_RenderShaderMgr.SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

bool CWorldModelManager::CreateSkyPortalVertexBuffer(InternalSkyPortalVertex* pData, uint32 dwSize)
{
	m_pSkyPortal_VB = g_GlobalMgr.CreateVertexBuffer(pData, sizeof(InternalSkyPortalVertex) * dwSize,
		D3D11_USAGE_IMMUTABLE, 0);

	return (m_pSkyPortal_VB != nullptr);
}

bool CWorldModelManager::CreateSkyPortalIndexBuffer(uint16* pData, uint32 dwSize)
{
	m_pSkyPortal_IB = g_GlobalMgr.CreateIndexBuffer(pData, sizeof(uint16) * dwSize, D3D11_USAGE_IMMUTABLE, 0);
	return (m_pSkyPortal_IB != nullptr);
}

bool CWorldModelManager::CreateSkyPortalBBoxVertexBuffer(InternalSkyPortalVertex* pData, uint32 dwSize)
{
	m_pSkyPortalBBox_VB = g_GlobalMgr.CreateVertexBuffer(pData, sizeof(InternalSkyPortalVertex) * dwSize,
		D3D11_USAGE_IMMUTABLE, 0);

	return (m_pSkyPortalBBox_VB != nullptr);
}

bool CWorldModelManager::CreateSkyPortalBBoxIndexBuffer(uint16* pData, uint32 dwSize)
{
	m_pSkyPortalBBox_IB = g_GlobalMgr.CreateIndexBuffer(pData, sizeof(uint16) * dwSize, D3D11_USAGE_IMMUTABLE, 0);
	return (m_pSkyPortalBBox_IB != nullptr);
}

bool d3d_CacheWorldModel(WorldBsp* pBsp)
{
	return (g_GlobalMgr.GetWorldModelMgr()->GetWorldModelData(pBsp) != nullptr);
}

bool d3d_CacheWorldModel(LTObject* pObject)
{
	return (g_GlobalMgr.GetWorldModelMgr()->GetWorldModelData(pObject->ToWorldModel()->m_pOriginalBsp) 
		!= nullptr);
}

bool d3d_IsTranslucentWorldModel(LTObject* pObject)
{
	WorldBsp* pBsp = pObject->ToWorldModel()->m_pOriginalBsp;

	// TODO - refine the condition?
	//return (pObject->m_nColorA < 255) ||
	//	(pBsp->m_dwSurfaces && pBsp->m_pSurfaces[0].m_dwFlags & SURF_TRANSPARENT) ||
	//	(pObject->m_dwFlags2 & FLAG2_ADDITIVE);

	if (pObject->m_dwFlags2 & FLAG2_ADDITIVE)
		return true;

	bool bFullyTransparent = true;
	bool bFullyUntextured = true;
	for (uint32 i = 0; i < pBsp->m_dwSurfaces; i++)
	{
		bFullyTransparent = bFullyTransparent && IsTransparentSurface(&pBsp->m_pSurfaces[i]);
		bFullyUntextured = bFullyUntextured && !IsTexturedSurface(&pBsp->m_pSurfaces[i]);
	}

	return bFullyTransparent || bFullyUntextured;
}

void d3d_ProcessWorldModel(LTObject* pObject)
{
	if (!g_CV_DrawWorldModels.m_Val)
		return;

	WorldModelInstance* pInstance = pObject->ToWorldModel();
	VisibleSet* pVisibleSet = d3d_GetVisibleSet();

	if (d3d_IsTranslucentWorldModel(pObject))
	{
		pVisibleSet->GetTranslucentWorldModels()->Add(pObject);
	}
	else
	{
		if (pObject->m_dwFlags2 & FLAG2_CHROMAKEY)
			pVisibleSet->GetChromakeyWorldModels()->Add(pObject);
		else
			pVisibleSet->GetWorldModels()->Add(pObject);
	}
}

static void d3d_GetFinalWorldModelTransform(ViewParams* pParams, DirectX::XMFLOAT4X4* pTransform,
	DirectX::XMFLOAT4X4* pTransformNoProj, DirectX::XMFLOAT4X4* pWorldModelTransform)
{
	DirectX::XMMATRIX mTransform;

	mTransform = DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(pWorldModelTransform)) *
		DirectX::XMLoadFloat4x4(&pParams->m_mViewTransposed);

	DirectX::XMStoreFloat4x4(pTransformNoProj, mTransform);

	mTransform *= DirectX::XMLoadFloat4x4(&pParams->m_mProjectionTransposed);

	DirectX::XMStoreFloat4x4(pTransform, mTransform);
}

static void d3d_GetFinalSkyPortalTransform(ViewParams* pParams, DirectX::XMFLOAT4X4* pTransform,
	DirectX::XMFLOAT4X4* pWorldModelTransform)
{
	DirectX::XMMATRIX mTransform;

	mTransform = DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(pWorldModelTransform)) *
		DirectX::XMLoadFloat4x4(&pParams->m_mViewTransposed) *
		DirectX::XMLoadFloat4x4(&pParams->m_mProjectionTransposed);
		//DirectX::XMLoadFloat4x4(&pParams->m_mSkyFarZProjectionTransposed);

	DirectX::XMStoreFloat4x4(pTransform, mTransform);
}

static void d3d_SetBuffersAndTopology(CWorldModelData* pData)
{
	g_RenderShaderMgr.SetVertexResource(VRS_Main, pData->m_pVertexBuffer, sizeof(InternalWorldModelVertex), 0);
	g_RenderShaderMgr.SetIndexBuffer32(pData->m_pIndexBuffer, 0);
	g_RenderShaderMgr.SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void d3d_DrawSkyPortals(ViewParams* pParams, DirectX::XMFLOAT4X4* pTransform)
{
	if (!g_GlobalMgr.GetWorldModelMgr()->GetSkyPortalVertexCount())
		return;

	g_RenderStateMgr.SavePrimaryStates();

	g_RenderStateMgr.SetBlendState(BLEND_STATE_Default);
	g_RenderStateMgr.SetRasterState(g_CV_Wireframe.m_Val ? RASTER_STATE_CullbackWireframe : RASTER_STATE_Cullback);

	g_GlobalMgr.GetWorldModelMgr()->SetSkyPortalBuffersAndTopology();

	XMFloat4x4Trinity sTransforms;
	sTransforms.m_mWorld = *pTransform;

	d3d_GetFinalSkyPortalTransform(pParams, &sTransforms.m_mWorldViewProj, &sTransforms.m_mWorld);

	CRenderShader_SkyPortal* pRenderShader = g_RenderShaderMgr.GetRenderShader<CRenderShader_SkyPortal>();

	if (!pRenderShader->SetPerObjectParams(
		g_RenderTargetMgr.GetRenderTarget(RENDER_TARGET_Bloom1)->GetShaderResourceView(),
		&sTransforms, g_D3DDevice.GetModeInfo()->m_dwWidth, g_D3DDevice.GetModeInfo()->m_dwHeight))
	{
		return;
	}

	pRenderShader->Render(g_GlobalMgr.GetWorldModelMgr()->GetSkyPortalIndexCount());
	g_RenderShaderMgr.ClearShaderResourcesPS(SRS_PS_Other, 1);

	g_RenderStateMgr.RestorePrimaryStates();
}

static void d3d_DrawWorldModel_Solid(ViewParams* pParams, LTObject* pObject)
{
	d3d_DrawWorldModel(pParams, pObject, BLEND_STATE_Default, 0);
}

static void d3d_DrawWorldModel_Alpha(ViewParams* pParams, LTObject* pObject)
{
	d3d_DrawWorldModel(pParams, pObject, BLEND_STATE_Alpha, 0);
}

static void d3d_DrawWorldModelEx_Alpha(ViewParams* pParams, LTObject* pObject, LTObject* pPrevObject)
{
	g_GlobalMgr.GetSpriteBatchMgr()->RenderBatch();
	g_GlobalMgr.GetCanvasBatchMgr()->RenderBatch();

	d3d_DrawWorldModel(pParams, pObject, BLEND_STATE_Alpha, 0);
}

static void d3d_DrawWorldModel_Chromakey(ViewParams* pParams, LTObject* pObject)
{
	d3d_DrawWorldModel(pParams, pObject, BLEND_STATE_Default, MODE_CHROMAKEY);
}

void d3d_DrawWorldModel(ViewParams* pParams, WorldBsp* pBsp, DirectX::XMFLOAT4X4* pTransform, uint32 dwFlags,
	uint32 dwFlags2, DirectX::XMFLOAT3* pPos, DirectX::XMFLOAT4* pDiffuseColor,
	BlendState eDefaultBlendState, uint32 dwExtraModeFlags)
{	
#ifdef WORLD_TWEAKS_ENABLED
	uint32 dwAllWorldTweakFlags = g_GlobalMgr.GetWorldTweakMgr()->m_dwAllFlags;
	
	if ((dwAllWorldTweakFlags & WORLD_TWEAK_HIDE_TERRAIN) && (pBsp->m_dwWorldInfoFlags & WIF_TERRAIN))
		return;
	
	bool bOuterRBsTest = (dwAllWorldTweakFlags & WORLD_TWEAK_RB_DIST_TEST) &&
		(pBsp->m_dwWorldInfoFlags & WIF_PHYSICSBSP);
#endif

	bool bFrustumTest = g_CV_RBExtraBBoxFrustumTest && !(dwExtraModeFlags & MODE_SKY_OBJECT) &&
		((pBsp->m_dwWorldInfoFlags & WIF_PHYSICSBSP) || (pBsp->m_dwWorldInfoFlags & WIF_TERRAIN));

#if defined(DEBUG) || defined(_DEBUG)
	bool bVisBoxTest = g_CV_VisBoxTest.m_Val && !(dwExtraModeFlags & MODE_SKY_OBJECT) &&
		((pBsp->m_dwWorldInfoFlags & WIF_PHYSICSBSP) || (pBsp->m_dwWorldInfoFlags & WIF_TERRAIN));
#endif
	
	g_RenderStateMgr.SavePrimaryStates();

	uint32 dwRenderMode = 0;
	BlendState eBlendState;

	dwRenderMode |= dwExtraModeFlags;

	if (!(dwRenderMode & MODE_SKY_OBJECT))
		g_RenderModeMgr.SetupRenderMode_WorldModel(dwFlags, dwFlags2, eBlendState, dwRenderMode, eDefaultBlendState);
	else
		g_RenderModeMgr.SetupRenderMode_SkyWorldModel(dwFlags, dwFlags2, eBlendState, dwRenderMode, eDefaultBlendState);

	g_RenderStateMgr.SetBlendState(eBlendState);

	if (eBlendState != BLEND_STATE_Alpha)
	{
		g_RenderStateMgr.SetRasterState(g_CV_Wireframe.m_Val ? RASTER_STATE_CullbackWireframe : RASTER_STATE_Cullback);
	}
	else
	{
		if (!g_CV_TwoPassSkyAlpha.m_Val && (dwRenderMode & MODE_SKY_OBJECT))
			g_RenderStateMgr.SetRasterState(g_CV_Wireframe.m_Val ? RASTER_STATE_Wireframe : RASTER_STATE_Default);
		else
			g_RenderStateMgr.SetRasterState(g_CV_Wireframe.m_Val ? RASTER_STATE_Wireframe : RASTER_STATE_CullbackCCW);
	}

	CWorldModelData* pData = g_GlobalMgr.GetWorldModelMgr()->GetWorldModelData(pBsp);

	if (pData == nullptr || pData->m_aRenderBlock.empty())
		return;

	d3d_SetBuffersAndTopology(pData);

	XMFloat4x4Trinity sTransforms;
	sTransforms.m_mWorld = *pTransform;

	d3d_GetFinalWorldModelTransform(pParams, &sTransforms.m_mWorldViewProj, &sTransforms.m_mWorldView,
		&sTransforms.m_mWorld);

	CRenderShader_WorldModel* pRenderShader;
	if (!(dwRenderMode & MODE_SKY_OBJECT))
		pRenderShader = g_RenderShaderMgr.GetRenderShader<CRenderShader_WorldModel>();
	else
		pRenderShader = g_RenderShaderMgr.GetRenderShader<CRenderShader_SkyWorldModel>();

	pDiffuseColor->w *= pData->GetAlphaMod();

	if (!pRenderShader->SetPerObjectParams(&sTransforms, pDiffuseColor, pData->m_pLMVertexData))
		return;

	CRenderShader_WorldModel::VPSPerSubObjectParams aShaderParams[WORLD_MODEL_TEXTURES_PER_DRAW] = { };
	uint32 aDynamicLightIndex[MAX_LIGHTS_PER_SUB_WORLD_MODEL] = { };
	uint32 aMSLightIndex[MAX_MS_LIGHTS_PER_SUB_WORLD_MODEL] = { };

	DirectX::XMMATRIX mWorldTransform = DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(pTransform));
	bool bIsWorldTransformIdentity = DirectX::XMMatrixIsIdentity(mWorldTransform);

#ifdef WORLD_TWEAKS_ENABLED
	for (uint32 i = 0; i < pData->m_aRenderBlock.size(); i++)
	{	
		CWorldModelData::RenderBlock& renderBlock = pData->m_aRenderBlock[i];
		
		if (bOuterRBsTest && !Vector_DoBoxesTouch(&renderBlock.m_vMinBBox, &renderBlock.m_vMaxBBox,
			&g_GlobalMgr.GetWorldTweakMgr()->m_vRB_BBoxMin, &g_GlobalMgr.GetWorldTweakMgr()->m_vRB_BBoxMax))
		{
			continue;
		}
#else
	for (CWorldModelData::RenderBlock& renderBlock : pData->m_aRenderBlock)
	{
#endif		
#if defined(DEBUG) || defined(_DEBUG)
		if (bVisBoxTest && !Vector_DoBoxesTouch(&renderBlock.m_BBox.m_vMin, &renderBlock.m_BBox.m_vMax,
			&g_vCameraPosVisBoxMin, &g_vCameraPosVisBoxMax))
		{
			continue;
		}
#endif
		if (bFrustumTest && !renderBlock.IsVisibleInFrustum(pParams))
			continue;
		
		for (uint32 dwTextureIndex = 0; dwTextureIndex < renderBlock.m_aTextureInfo.size(); dwTextureIndex++)
		{
			InitPerSubObjectShaderParams(&aShaderParams[dwTextureIndex], &renderBlock.m_aTextureInfo[dwTextureIndex],
				dwRenderMode);
		}

		if (!renderBlock.m_aVertexFXData.empty())
		{
			g_RenderShaderMgr.SetShaderResourceVS(SRS_VS_VertexFXData, renderBlock.m_pVertexFXView);
			renderBlock.UpdateVertexFXBuffer();
		}

		uint32 dwLightCount = 0;
		uint32 dwMSLightCount = 0;
		if (!(dwRenderMode & MODE_SKY_OBJECT))
		{
			if (!bIsWorldTransformIdentity)
			{
				DirectX::XMFLOAT3 vMinBBox, vMaxBBox;
				TransformBBox(pPos, mWorldTransform, renderBlock, &vMinBBox, &vMaxBBox);

				dwLightCount = InitDynamicLightIndices(aDynamicLightIndex, &vMinBBox, &vMaxBBox);
				dwMSLightCount = InitModelShadowLightIndices(aMSLightIndex, &vMinBBox, &vMaxBBox);
			}
			else
			{
				dwLightCount = InitDynamicLightIndicesEx(aDynamicLightIndex, renderBlock.m_aExtraBBox);
				dwMSLightCount = InitModelShadowLightIndicesEx(aMSLightIndex, renderBlock.m_aExtraBBox);
			}			
		}

		pRenderShader->SetPerSubObjectParams(aShaderParams, dwLightCount, aDynamicLightIndex, dwMSLightCount, aMSLightIndex);

#ifdef WORLD_TWEAKS_ENABLED
		if ((dwAllWorldTweakFlags & WORLD_TWEAK_RB_CUT) && i == g_GlobalMgr.GetWorldTweakMgr()->m_pRBCutData->m_dwIndex)
		{
			uint32 dwIndexCount1, dwIndexPos1, dwIndexCount2, dwIndexPos2;
			g_GlobalMgr.GetWorldTweakMgr()->CalcRBCutData(renderBlock.m_dwIndexCount, renderBlock.m_dwIndexPos, 
				dwIndexCount1, dwIndexPos1, dwIndexCount2, dwIndexPos2);

			pRenderShader->Render(dwIndexCount1, dwIndexPos1);

			if (dwIndexCount2)
				pRenderShader->Render(dwIndexCount2, dwIndexPos2);

			if (g_RenderStateMgr.GetRasterState() == RASTER_STATE_CullbackCCW)
			{
				g_RenderStateMgr.SetRasterState(RASTER_STATE_Cullback);

				pRenderShader->Render(dwIndexCount1, dwIndexPos1);

				if (dwIndexCount2)
					pRenderShader->Render(dwIndexCount2, dwIndexPos2);
			}
		}
		else
#endif
		{
			pRenderShader->Render(renderBlock.m_dwIndexCount, renderBlock.m_dwIndexPos);

			if (g_RenderStateMgr.GetRasterState() == RASTER_STATE_CullbackCCW)
			{
				g_RenderStateMgr.SetRasterState(RASTER_STATE_Cullback);
				pRenderShader->Render(renderBlock.m_dwIndexCount, renderBlock.m_dwIndexPos);
			}
		}	
	}

	g_RenderStateMgr.RestorePrimaryStates();
}

void d3d_DrawWorldModel(ViewParams* pParams, LTObject* pObject, BlendState eDefaultBlendState, uint32 dwExtraModeFlags)
{
	WorldModelInstance* pInstance = pObject->ToWorldModel();

	DirectX::XMFLOAT4 vDiffuseColor
	{
		(float)pObject->m_nColorR * MATH_ONE_OVER_255,
		(float)pObject->m_nColorG * MATH_ONE_OVER_255,
		(float)pObject->m_nColorB * MATH_ONE_OVER_255,
		(float)pObject->m_nColorA * MATH_ONE_OVER_255,
	};

	d3d_DrawWorldModel(pParams, pInstance->m_pOriginalBsp, PLTMATRIX_TO_PXMFLOAT4X4(&pInstance->m_mTransform),
		pInstance->m_Base.m_dwFlags, pInstance->m_Base.m_dwFlags2, PLTVECTOR_TO_PXMFLOAT3(&pInstance->m_Base.m_vPos),
		&vDiffuseColor, eDefaultBlendState, dwExtraModeFlags);
}

void d3d_DrawSolidWorldModels(ViewParams* pParams)
{
	AllocSet* pSet = d3d_GetVisibleSet()->GetWorldModels();
	if (!pSet->GetObjectCount())
		return;

	pSet->Draw(pParams, d3d_DrawWorldModel_Solid);
}

void d3d_DrawChromakeyWorldModels(ViewParams* pParams)
{
	AllocSet* pSet = d3d_GetVisibleSet()->GetChromakeyWorldModels();
	if (!pSet->GetObjectCount())
		return;

	pSet->Draw(pParams, d3d_DrawWorldModel_Chromakey);
}

void d3d_QueueTranslucentWorldModels(ViewParams* pParams, ObjectDrawList* pDrawList)
{
	d3d_GetVisibleSet()->GetTranslucentWorldModels()->Queue(pDrawList, pParams, d3d_DrawWorldModel_Alpha, 
		d3d_DrawWorldModelEx_Alpha);
}