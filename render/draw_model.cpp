#include "pch.h"

#include "draw_model.h"
#include "rendererconsolevars.h"
#include "draw_objects.h"
#include "setup_model.h"
#include "common_stuff.h"
#include "globalmgr.h"
#include "d3d_draw.h"
#include "renderstatemgr.h"
#include "3d_ops.h"
#include "d3d_shader_model.h"
#include "globalmgr.h"
#include "common_draw.h"
#include "rendermodemgr.h"
#include "draw_light.h"

static Array_InternalModelVertex g_aVertexData;
static Array_UInt16 g_aIndexData;
static Map_Array_IndexToUVPair g_VertexDuplicates;

template<typename T>
void PieceOrLODToInternalFormat(T* pPieceOrLOD, Array_InternalModelVertex& aVertexData, Array_UInt16& aIndexData,
	uint32& dwCurrVertexPos, uint32& dwCurrIndexPos, uint32 dwTextureIndex)
{
	uint32 dwStartVertexPos = dwCurrVertexPos;
	uint32 dwVertCount = pPieceOrLOD->GetVertCount();

	for (uint32 i = 0; i < dwVertCount; i++)
	{
		ModelVert* pModelVert = pPieceOrLOD->GetVert(i);
		InternalModelVertex currVert;

		currVert.vPosition = *PLTVECTOR_TO_PXMFLOAT3(&pModelVert->m_vVec);
		currVert.vNormal = *PLTVECTOR_TO_PXMFLOAT3(&pModelVert->m_vNormal);
		currVert.dwWeightCount =
			pModelVert->m_wWeights > MODEL_MAX_WEIGHTS_PER_VERTEX ? MODEL_MAX_WEIGHTS_PER_VERTEX : pModelVert->m_wWeights;

		currVert.dwTextureIndex = dwTextureIndex;

		for (uint32 j = 0; j < currVert.dwWeightCount; j++)
		{
			*((&currVert.vBlendWeight0) + j) = *PLTROTATION_TO_PXMFLOAT4(&pModelVert->m_pWeights[j].m_afVec);
			currVert.adwBlendIndices[j] = pModelVert->m_pWeights[j].m_dwNode;
		}

		aVertexData.push_back(currVert);
		dwCurrVertexPos++;
	}

	// TODO - switch back to array?
	g_VertexDuplicates.clear();

	for (uint32 i = 0; i < pPieceOrLOD->GetTriCount(); i++)
	{
		ModelTri* pModelTri = pPieceOrLOD->GetTri(i);

		for (uint32 j = 0; j < MODEL_VERTEX_PER_TRI; j++)
		{
			uint16 wIndex = pModelTri->m_awIndices[j];
			Array_IndexToUVPair& uvPairs = g_VertexDuplicates[wIndex];

			if (!uvPairs.size())
			{
				aVertexData[dwStartVertexPos + wIndex].vTexCoords = { pModelTri->m_UVs[j].m_fU, pModelTri->m_UVs[j].m_fV };

				uvPairs.emplace_back(wIndex, &pModelTri->m_UVs[j]);
				aIndexData.push_back((uint16)dwStartVertexPos + wIndex);
			}
			else
			{
				auto iter = std::find(uvPairs.begin(), uvPairs.end(), pModelTri->m_UVs[j]);

				if (iter == uvPairs.end())
				{
					aVertexData.push_back(aVertexData[dwStartVertexPos + wIndex]);
					aVertexData[dwStartVertexPos + dwVertCount].vTexCoords = { pModelTri->m_UVs[j].m_fU, pModelTri->m_UVs[j].m_fV };

					uvPairs.emplace_back(dwVertCount, &pModelTri->m_UVs[j]);
					aIndexData.push_back((uint16)dwStartVertexPos + dwVertCount);

					dwCurrVertexPos++;
					dwVertCount++;
				}
				else
				{
					aIndexData.push_back((uint16)dwStartVertexPos + iter->m_dwIndex);
				}
			}
		}

		dwCurrIndexPos += MODEL_VERTEX_PER_TRI;
	}
}

CModelData::~CModelData()
{
	RELEASE_INTERFACE(m_pVertexBuffer, g_szFreeError_VB);
	RELEASE_INTERFACE(m_pIndexBuffer, g_szFreeError_IB);

	m_aBufferInfo.clear();
}

bool CModelData::Init(Model* pModel)
{
#if defined(DEBUG) || defined(_DEBUG)
	strcpy_s(m_szFilename, pModel->m_szFilename);
#endif

	uint32 dwVertexBufferSize, dwIndexBufferSize;
	PieceToLODArrays_Reserve(pModel, dwVertexBufferSize, dwIndexBufferSize);

	g_aVertexData.clear();
	g_aIndexData.clear();

	g_aVertexData.reserve(dwVertexBufferSize);
	g_aIndexData.reserve(dwIndexBufferSize);

	uint32 dwCurrVertexPos = 0;
	uint32 dwCurrIndexPos = 0;

	for (uint32 dwPieceIndex = 0; dwPieceIndex < pModel->GetPieceCount(); dwPieceIndex++)
	{
		ModelPiece* pPiece = pModel->GetPiece(dwPieceIndex);

		m_aBufferInfo[dwPieceIndex][0].m_dwVertexPos = dwCurrVertexPos;
		m_aBufferInfo[dwPieceIndex][0].m_dwIndexPos = dwCurrIndexPos;

		PieceOrLODToInternalFormat(pPiece, g_aVertexData, g_aIndexData, dwCurrVertexPos, dwCurrIndexPos,
			pPiece->m_wTextureIndex);

#ifdef MODEL_PIECELESS_RENDERING
		if (!g_CV_ModelNoLODs.m_Val)
		{
			for (uint32 dwLODIndex = 0; dwLODIndex < pPiece->GetLODCount(); dwLODIndex++)
			{
				PieceLOD* pLOD = pPiece->GetLOD(dwLODIndex);

				m_aBufferInfo[dwPieceIndex][dwLODIndex + 1].m_dwVertexPos = dwCurrVertexPos;
				m_aBufferInfo[dwPieceIndex][dwLODIndex + 1].m_dwIndexPos = dwCurrIndexPos;

				PieceOrLODToInternalFormat(pLOD, g_aVertexData, g_aIndexData, dwCurrVertexPos, dwCurrIndexPos,
					pPiece->m_wTextureIndex);
			}
		}
#endif
		m_dwTextureIndicesInUse |= (1 << pPiece->m_wTextureIndex);
	}

	m_dwVertexCount = dwCurrVertexPos;
	m_dwIndexCount = dwIndexBufferSize;

	return CreateVertexBuffer(g_aVertexData.data(), dwCurrVertexPos) &&
		CreateIndexBuffer(g_aIndexData.data(), dwIndexBufferSize);
}

void CModelData::PieceToLODArrays_Reserve(Model* pModel, uint32& dwVertexBufferSize, uint32& dwIndexBufferSize)
{
	dwVertexBufferSize = 0;
	dwIndexBufferSize = 0;

	uint32 dwPieceCount = pModel->GetPieceCount();
	m_aBufferInfo.reserve(dwPieceCount);

	for (uint32 dwPieceIndex = 0; dwPieceIndex < dwPieceCount; dwPieceIndex++)
	{
		m_aBufferInfo.emplace_back();

		ModelPiece* pPiece = pModel->GetPiece(dwPieceIndex);
		uint32 dwLODCount = pPiece->GetLODCount() + 1;

		m_aBufferInfo[dwPieceIndex].reserve(dwLODCount);

		dwVertexBufferSize += pPiece->GetVertCount();
		uint32 dwIndexCount = pPiece->GetTriCount() * MODEL_VERTEX_PER_TRI;
		dwIndexBufferSize += dwIndexCount;

		m_aBufferInfo[dwPieceIndex].emplace_back(0, 0, dwIndexCount);

#ifdef MODEL_PIECELESS_RENDERING
		if (!g_CV_ModelNoLODs.m_Val)
		{
			for (uint32 dwLODIndex = 0; dwLODIndex < pPiece->GetLODCount(); dwLODIndex++)
			{
				PieceLOD* pLOD = pPiece->GetLOD(dwLODIndex);

				dwVertexBufferSize += pLOD->GetVertCount();
				dwIndexCount = pLOD->GetTriCount() * MODEL_VERTEX_PER_TRI;
				dwIndexBufferSize += dwIndexCount;

				m_aBufferInfo[dwPieceIndex].emplace_back(0, 0, dwIndexCount);
			}
		}
#endif
	}
}

bool CModelData::CreateVertexBuffer(InternalModelVertex* pData, uint32 dwSize)
{
	m_pVertexBuffer = g_GlobalMgr.CreateVertexBuffer(pData, sizeof(InternalModelVertex) * dwSize, D3D11_USAGE_IMMUTABLE, 0);
	return (m_pVertexBuffer != nullptr);
}

bool CModelData::CreateIndexBuffer(uint16* pData, uint32 dwSize)
{
	m_pIndexBuffer = g_GlobalMgr.CreateIndexBuffer(pData, sizeof(uint16) * dwSize, D3D11_USAGE_IMMUTABLE, 0);
	return (m_pIndexBuffer != nullptr);
}

CModelData* CModelManager::GetModelData(Model* pModel)
{
	auto iter = m_Data.find(pModel);

	if (iter != m_Data.end())
	{
		CModelData* pData = (*iter).second;

#if defined(DEBUG) || defined(_DEBUG)
		// TODO - shouldn't happen within one level?
		if (strcmp(pData->m_szFilename, pModel->m_szFilename))
			AddDebugMessage(0, "Model filename has changed!");
#endif

		pData->SetLastUpdate(g_fLastClientTime);

		return pData;
	}

	CModelData* pNewData = new CModelData();

	if (!pNewData->Init(pModel))
	{
		delete pNewData;
		return nullptr;
	}

	pNewData->SetLastUpdate(g_fLastClientTime);
	m_Data[pModel] = pNewData;

	return pNewData;
}

bool d3d_CacheModel(LTObject* pObject)
{
	return (g_GlobalMgr.GetModelMgr()->GetModelData(pObject->ToModel()->GetModel()) != nullptr);
}

void d3d_GetFinalModelTransform(DirectX::XMFLOAT4X4* pTransform, DirectX::XMFLOAT4X4* pTransformNoProj,
	DirectX::XMFLOAT4X4* pModelTransform, DirectX::XMFLOAT4X4* pReallyCloseProj)
{
	DirectX::XMMATRIX mTransform;

	if (pReallyCloseProj == nullptr)
	{
		mTransform = DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(pModelTransform)) *
			DirectX::XMLoadFloat4x4(&g_ViewParams.m_mViewTransposed);

		DirectX::XMStoreFloat4x4(pTransformNoProj, mTransform);

		mTransform *= DirectX::XMLoadFloat4x4(&g_ViewParams.m_mProjectionTransposed);
	}
	else
	{
		mTransform = DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(pModelTransform));

		DirectX::XMStoreFloat4x4(pTransformNoProj, mTransform);

		mTransform *= DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(pReallyCloseProj));
	}

	DirectX::XMStoreFloat4x4(pTransform, mTransform);
}

static void d3d_SetBuffersAndTopology(CModelData* pData)
{
	g_RenderShaderMgr.SetVertexResource(VRS_Main, pData->m_pVertexBuffer, sizeof(InternalModelVertex), 0);
	g_RenderShaderMgr.SetIndexBuffer16(pData->m_pIndexBuffer, 0);
	g_RenderShaderMgr.SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

static bool d3d_AreBlendStatesIdentical(BlendState* pBlendStates, CModelData* pData)
{
	BlendState eGoodBlendState = BLEND_STATE_Invalid;
	
	for (uint32 i = 0; i < MAX_MODEL_TEXTURES; i++)
	{
		if (!(pData->GetTextureIndicesInUse() & (1 << i)))
			continue;

		if (eGoodBlendState == BLEND_STATE_Invalid)
			eGoodBlendState = pBlendStates[i];
		else if (eGoodBlendState != pBlendStates[i])
			return false;
	}

	return true;
}

static void d3d_SetupTexturesAndRelatedData(ID3D11ShaderResourceView** ppTextures, uint32* pRenderModes,
	DirectX::XMFLOAT3* pModeColors, float* pAlphaRefs, BlendState* pBlendStates, StencilState* pStencilStates,
	ModelInstance* pInstance, uint32 dwBaseRenderMode, BlendState eBaseBlendState)
{	
	constexpr float s_fDefaultAlphaRef = 0.5f + (0.5f / 255.0f);
	
	for (uint32 i = 0; i < MAX_MODEL_TEXTURES; i++)
	{
		pModeColors[i] = { };
		pBlendStates[i] = eBaseBlendState;

		SharedTexture* pTexture = pInstance->m_pSkins[i];
		
		if (pTexture != nullptr)
		{		
			pRenderModes[i] = dwBaseRenderMode;
			
			if (pTexture->m_pStateChange != nullptr)
			{	
				BlendState eBlendStateOverride = BLEND_STATE_Invalid;
				StencilState eStencilStateOverride = STENCIL_STATE_Invalid;

				g_RenderModeMgr.ApplyStateChange(pTexture->m_pStateChange, eBlendStateOverride, eStencilStateOverride,
					pRenderModes[i], &pModeColors[i]);

				if (eBlendStateOverride != BLEND_STATE_Invalid)
					pBlendStates[i] = eBlendStateOverride;

				if (eStencilStateOverride != BLEND_STATE_Invalid)
					pStencilStates[i] = eStencilStateOverride;
			}

			RTexture* pRTexture = (RTexture*)pTexture->m_pRenderData;
			ppTextures[i] = pRTexture->m_pResourceView;

			pAlphaRefs[i] = pRTexture->m_fAlphaRef;
			if ((pRenderModes[i] & MODE_CHROMAKEY) && pAlphaRefs[i] == 0.0f)
				pAlphaRefs[i] = s_fDefaultAlphaRef;
		}
		else
		{
			ppTextures[i] = nullptr;
			pRenderModes[i] = (dwBaseRenderMode & MODE_NO_TEXTURE);
			pAlphaRefs[i] = 0.0f;
		}
	}
}

static void d3d_DrawModelPiece(QueuedPieceInfo* pQueuedInfo, uint32 dwRenderMode, BlendState eBlendStateOverride,
	StencilState eStencilStateOverride)
{
	CModelData* pData = g_GlobalMgr.GetModelMgr()->GetModelData(pQueuedInfo->m_pModel);

	if (pData == nullptr)
		return;

	g_RenderStateMgr.SetBlendState(eBlendStateOverride);

	// TODO - stencil state override support
	//g_RenderStateMgr.SetStencilState(eStencilStateOverride);

	CModelData::BufferInfo* pBufferInfo = &pData->m_aBufferInfo[pQueuedInfo->m_dwPieceIndex][pQueuedInfo->m_dwLODIndex];
	d3d_SetBuffersAndTopology(pData);

	CRenderShader_Model* pRenderShader = g_RenderShaderMgr.GetRenderShader<CRenderShader_Model>();

	pRenderShader->Render(pBufferInfo->m_dwIndexCount, pBufferInfo->m_dwIndexPos);
}

static void d3d_InitHiddenPartsData(uint32& dwHiddenPartsCount, uint32* pHiddenPrimitiveIDs, uint32 dwHiddenPieces, 
	CModelData* pData)
{
	dwHiddenPartsCount = 0;

	if (!dwHiddenPieces)
		return;

	for (uint32 i = 0; i < pData->m_aBufferInfo.size() && i < MAX_PIECES_PER_MODEL; i++)
	{
		if (dwHiddenPieces & (1 << i))
		{
			CModelData::BufferInfo* pInfo = &pData->m_aBufferInfo[i][0];
			
			if (dwHiddenPartsCount && pHiddenPrimitiveIDs[(dwHiddenPartsCount - 1) * 2 + 1] + 1 == pInfo->m_dwIndexPos / 3)
			{
				pHiddenPrimitiveIDs[(dwHiddenPartsCount - 1) * 2 + 1] += pInfo->m_dwIndexCount / 3;
			}
			else
			{
				pHiddenPrimitiveIDs[dwHiddenPartsCount * 2 + 0] = pInfo->m_dwIndexPos / 3;
				pHiddenPrimitiveIDs[dwHiddenPartsCount * 2 + 1] = (pInfo->m_dwIndexPos + pInfo->m_dwIndexCount - 1) / 3;

				dwHiddenPartsCount++;
			}
		}
	}
}

static void d3d_EnrichHiddenPartsData(uint32& dwHiddenPartsCount, uint32* pHiddenPrimitiveIDs, uint32 dwTextureIndex,
	Model* pModel, CModelData* pData)
{
	for (uint32 dwPiece = 0; dwPiece < pModel->GetPieceCount(); dwPiece++)
	{
		if (pModel->GetPiece(dwPiece)->m_wTextureIndex != dwTextureIndex)
		{
			uint32 dwMatchedPart = UINT32_MAX;
			CModelData::BufferInfo* pInfo = &pData->m_aBufferInfo[dwPiece][0];
			
			for (uint32 dwPart = 0; dwPart < dwHiddenPartsCount; dwPart++)
			{
				if (pHiddenPrimitiveIDs[dwPart * 2 + 1] + 1 == pInfo->m_dwIndexPos / 3)
					dwMatchedPart = dwPart;
			}

			if (dwMatchedPart == UINT32_MAX)
			{
				pHiddenPrimitiveIDs[dwHiddenPartsCount * 2 + 0] = pInfo->m_dwIndexPos / 3;
				pHiddenPrimitiveIDs[dwHiddenPartsCount * 2 + 1] = (pInfo->m_dwIndexPos + pInfo->m_dwIndexCount - 1) / 3;

				dwHiddenPartsCount++;
			}
			else
			{
				pHiddenPrimitiveIDs[dwMatchedPart * 2 + 1] += pInfo->m_dwIndexCount / 3;
			}
		}
	}
}

static void d3d_DrawModelPieceless(ModelInstance* pInstance, QueuedModelInfo* pModelInfo, uint32 dwBaseRenderMode, 
	BlendState eBaseBlendState)
{
	Model* pModel = pInstance->GetModel();
	CModelData* pData = g_GlobalMgr.GetModelMgr()->GetModelData(pModel);

	if (pData == nullptr)
		return;

	d3d_SetBuffersAndTopology(pData);
	
	InternalModelHookData* pModelHookData = &pModelInfo->m_ModelHookData;
	
	DirectX::XMFLOAT3 vLightAdd
	{
		pModelHookData->m_vLightAdd.x * 0.5f * MATH_ONE_OVER_255,
		pModelHookData->m_vLightAdd.y * 0.5f * MATH_ONE_OVER_255,
		pModelHookData->m_vLightAdd.z * 0.5f * MATH_ONE_OVER_255,
	};

	DirectX::XMFLOAT4 vDiffuseColor
	{
		(pModelInfo->m_vAmbientLight.x + vLightAdd.x) * (pModelHookData->m_vObjectColor.x * MATH_ONE_OVER_255),
		(pModelInfo->m_vAmbientLight.y + vLightAdd.y) * (pModelHookData->m_vObjectColor.y * MATH_ONE_OVER_255),
		(pModelInfo->m_vAmbientLight.z + vLightAdd.z) * (pModelHookData->m_vObjectColor.z * MATH_ONE_OVER_255),
		(float)pInstance->m_Base.m_nColorA * MATH_ONE_OVER_255,
	};

	DirectX::XMFLOAT4X4 mNormalRef;
	DirectX::XMFLOAT4X4* pNormalRef;

	if (pModel->m_bNormalRef)
	{
		DirectX::XMMATRIX mNormalRefTemp =
			DirectX::XMLoadFloat4x4(PLTMATRIX_TO_PXMFLOAT4X4(pModel->GetTransform(pModel->m_dwNormalRefNode))) *
			DirectX::XMLoadFloat4x4(PLTMATRIX_TO_PXMFLOAT4X4(&pModel->m_mNormalRef));

		DirectX::XMStoreFloat4x4(&mNormalRef, mNormalRefTemp);
		pNormalRef = &mNormalRef;
	}
	else
	{
		pNormalRef = &g_mIdentity;
	}
	
	BlendState aBlendStateOverride[MAX_MODEL_TEXTURES];
	StencilState aStencilStateOverride[MAX_MODEL_TEXTURES];

	uint32 dwHiddenPieceCount = 0;
	uint32 adwHiddenPrimitiveIDs[MAX_PIECES_PER_MODEL << 1];
	CRenderShader_Model::VGPSPerObjectParams params;

	params.m_pNodeTransforms = PLTMATRIX_TO_PXMFLOAT4X4(pInstance->m_CachedTransforms.m_pArray);
	params.m_dwNodeCount = pInstance->GetCachedTransformCount();
	params.m_pTransforms = &pModelInfo->m_sTransforms;
	params.m_pDiffuseColor = &vDiffuseColor;
	params.m_pNormalRef = pNormalRef;
	params.m_pDirLightColor = &pModelInfo->m_vDirLightColor;
	params.m_dwDynamicLightCount = pModelInfo->m_dwDynamicLightCount;
	params.m_dwStaticLightCount = pModelInfo->m_dwStaticLightCount;
	params.m_pDynamicLightIndices = pModelInfo->m_aDynamicLightIndex;
	params.m_ppStaticLights = pModelInfo->m_apStaticLight;

	d3d_InitHiddenPartsData(dwHiddenPieceCount, adwHiddenPrimitiveIDs, pInstance->m_adwHiddenPiece[0], pData);
	
	d3d_SetupTexturesAndRelatedData(params.m_apTextures, params.m_adwMode, params.m_avModeColor,
		params.m_afAlphaRef, aBlendStateOverride, aStencilStateOverride, pInstance, 
		dwBaseRenderMode, eBaseBlendState);

	CRenderShader_Model* pRenderShader = g_RenderShaderMgr.GetRenderShader<CRenderShader_Model>();
	pRenderShader->SetPerObjectParamsVPS(&params);

	// TODO - stencil state override support
	if (d3d_AreBlendStatesIdentical(aBlendStateOverride, pData))
	{
		pRenderShader->SetPerObjectParamsGS(dwHiddenPieceCount, adwHiddenPrimitiveIDs);	
		g_RenderStateMgr.SetBlendState(aBlendStateOverride[0]);

		pRenderShader->Render(pData->GetIndexCount(), 0);
	}
	else
	{
		for (uint32 i = 0; i < MAX_MODEL_TEXTURES; i++)
		{		
			if (!(pData->GetTextureIndicesInUse() & (1 << i)))
				continue;
			
			uint32 dwHiddenPieceCountEx = dwHiddenPieceCount;
			uint32 adwHiddenPrimitiveIDsEx[MAX_PIECES_PER_MODEL << 1];
			memcpy(adwHiddenPrimitiveIDsEx, adwHiddenPrimitiveIDs, sizeof(adwHiddenPrimitiveIDs));

			d3d_EnrichHiddenPartsData(dwHiddenPieceCountEx, adwHiddenPrimitiveIDsEx, i, pModel, pData);

			pRenderShader->SetPerObjectParamsGS(dwHiddenPieceCountEx, adwHiddenPrimitiveIDsEx);
			g_RenderStateMgr.SetBlendState(aBlendStateOverride[i]);

			pRenderShader->Render(pData->GetIndexCount(), 0);
		}
	}
}

void d3d_DrawModel(ViewParams* pParams, LTObject* pObject, BlendState eDefaultBlendState, uint32 dwExtraModeFlags)
{
	ModelInstance* pInstance = pObject->ToModel();

	g_RenderStateMgr.SavePrimaryStates();

	QueuedModelInfo modelInfo(pInstance);
	g_ModelSetup.FillModelInfo(&modelInfo, pInstance);

	uint32 dwHookedFlags = modelInfo.m_ModelHookData.m_dwObjectFlags;

	uint32 dwBaseRenderMode = (dwHookedFlags & FLAG_REALLYCLOSE) ? MODE_REALLY_CLOSE : 0;
	BlendState eBaseBlendState;

	g_RenderModeMgr.SetupRenderMode_Model(dwHookedFlags, pInstance->m_Base.m_dwFlags2, eBaseBlendState, 
		dwBaseRenderMode, eDefaultBlendState);

	dwBaseRenderMode |= dwExtraModeFlags;

	g_RenderStateMgr.SetRasterState((dwHookedFlags & FLAG_MODELWIREFRAME) || g_CV_Wireframe.m_Val ? 
		RASTER_STATE_CullbackWireframe : RASTER_STATE_Cullback);

	CReallyCloseData reallyCloseData;
	if (dwHookedFlags & FLAG_REALLYCLOSE)
		d3d_SetReallyClose(&reallyCloseData);

	d3d_DrawModelPieceless(pInstance, &modelInfo, dwBaseRenderMode, eBaseBlendState);

	if (dwHookedFlags & FLAG_REALLYCLOSE)
		d3d_UnsetReallyClose(&reallyCloseData);

	g_RenderStateMgr.RestorePrimaryStates();
}

static void d3d_DrawModel_Solid(ViewParams* pParams, LTObject* pObject)
{
	d3d_DrawModel(pParams, pObject, BLEND_STATE_Default, 0);
}

static void d3d_DrawModel_Alpha(ViewParams* pParams, LTObject* pObject)
{
	d3d_DrawModel(pParams, pObject, BLEND_STATE_Alpha, 0);
}

static void d3d_DrawModelEx_Alpha(ViewParams* pParams, LTObject* pObject, LTObject* pPrevObject)
{
	g_GlobalMgr.GetSpriteBatchMgr()->RenderBatch();
	g_GlobalMgr.GetCanvasBatchMgr()->RenderBatch();

	d3d_DrawModel(pParams, pObject, BLEND_STATE_Alpha, 0);
}

static void d3d_DrawModel_Chromakey(ViewParams* pParams, LTObject* pObject)
{
	d3d_DrawModel(pParams, pObject, BLEND_STATE_Default, MODE_CHROMAKEY);
}

#pragma warning(push)
#pragma warning(disable:6011)

void d3d_DrawModelPieceList(BlendState eDefaultBlendState, uint32 dwExtraModeFlags)
{
	Array_QueuedPieceInfo& queuedPieceInfo = g_ModelSetup.GetQueuedPieceInfo();

	if (eDefaultBlendState == BLEND_STATE_Alpha && dwExtraModeFlags == 0)
		std::sort(queuedPieceInfo.begin(), queuedPieceInfo.end(), QueuedPieceInfo::LessThan_Translucent);
	else
		std::sort(queuedPieceInfo.begin(), queuedPieceInfo.end());

	g_RenderStateMgr.SavePrimaryStates();

	CRenderShader_Model* pRenderShader = g_RenderShaderMgr.GetRenderShader<CRenderShader_Model>();

	ModelInstance* pCurrModelInstance = nullptr;
	bool bInReallyClose = false;
	CReallyCloseData reallyCloseData;

	BlendState aBlendStateOverride[MAX_MODEL_TEXTURES] = { };
	StencilState aStencilStateOverride[MAX_MODEL_TEXTURES] = { };
	CRenderShader_Model::VGPSPerObjectParams params = { };

	for (QueuedPieceInfo& queuedItem : queuedPieceInfo)
	{
		InternalModelHookData* pModelHookData = &queuedItem.m_pModelInfo->m_ModelHookData;
		uint32 dwHookedFlags = pModelHookData->m_dwObjectFlags;

		g_RenderStateMgr.SetRasterState((dwHookedFlags & FLAG_MODELWIREFRAME) || g_CV_Wireframe.m_Val ? 
			RASTER_STATE_CullbackWireframe : RASTER_STATE_Cullback);

		if (bInReallyClose)
		{
			if (!queuedItem.m_bReallyClose)
			{
				d3d_UnsetReallyClose(&reallyCloseData);
				bInReallyClose = false;
			}
		}
		else
		{
			if (queuedItem.m_bReallyClose)
			{
				d3d_SetReallyClose(&reallyCloseData);
				bInReallyClose = true;
			}
		}

		if (queuedItem.m_pInstance != pCurrModelInstance)
		{
			uint32 dwBaseRenderMode = queuedItem.m_bReallyClose ? MODE_REALLY_CLOSE : 0;
			BlendState eBaseBlendState;

			g_RenderModeMgr.SetupRenderMode_Model(dwHookedFlags, queuedItem.m_pInstance->m_Base.m_dwFlags2,
				eBaseBlendState, dwBaseRenderMode, eDefaultBlendState);

			QueuedModelInfo* pModelInfo = queuedItem.m_pModelInfo;
			pCurrModelInstance = queuedItem.m_pInstance;

			DirectX::XMFLOAT3 vLightAdd
			{
				pModelHookData->m_vLightAdd.x * 0.5f * MATH_ONE_OVER_255,
				pModelHookData->m_vLightAdd.y * 0.5f * MATH_ONE_OVER_255,
				pModelHookData->m_vLightAdd.z * 0.5f * MATH_ONE_OVER_255,
			};

			DirectX::XMFLOAT4 vDiffuseColor
			{
				(pModelInfo->m_vAmbientLight.x + vLightAdd.x) *
					(pModelHookData->m_vObjectColor.x * MATH_ONE_OVER_255),

				(pModelInfo->m_vAmbientLight.y + vLightAdd.y) *
					(pModelHookData->m_vObjectColor.y * MATH_ONE_OVER_255),

				(pModelInfo->m_vAmbientLight.z + vLightAdd.z) * 
					(pModelHookData->m_vObjectColor.z * MATH_ONE_OVER_255),

				(float)pCurrModelInstance->m_Base.m_nColorA * MATH_ONE_OVER_255,
			};		
			
			Model* pModel = queuedItem.m_pModel;
			DirectX::XMFLOAT4X4 mNormalRef;
			DirectX::XMFLOAT4X4* pNormalRef;
		
			if (pModel->m_bNormalRef)
			{			
				DirectX::XMMATRIX mNormalRefTemp = 
					DirectX::XMLoadFloat4x4(PLTMATRIX_TO_PXMFLOAT4X4(pModel->GetTransform(pModel->m_dwNormalRefNode))) *
					DirectX::XMLoadFloat4x4(PLTMATRIX_TO_PXMFLOAT4X4(&pModel->m_mNormalRef));
				
				DirectX::XMStoreFloat4x4(&mNormalRef, mNormalRefTemp);
				pNormalRef = &mNormalRef;
			}
			else
			{
				pNormalRef = &g_mIdentity;
			}

			params.m_pNodeTransforms = PLTMATRIX_TO_PXMFLOAT4X4(pCurrModelInstance->m_CachedTransforms.m_pArray);
			params.m_dwNodeCount = pCurrModelInstance->GetCachedTransformCount();
			params.m_pTransforms = &pModelInfo->m_sTransforms;
			params.m_pDiffuseColor = &vDiffuseColor;
			params.m_pNormalRef = pNormalRef;
			params.m_pDirLightColor = &pModelInfo->m_vDirLightColor;
			params.m_dwDynamicLightCount = pModelInfo->m_dwDynamicLightCount;
			params.m_dwStaticLightCount = pModelInfo->m_dwStaticLightCount;
			params.m_pDynamicLightIndices = pModelInfo->m_aDynamicLightIndex;
			params.m_ppStaticLights = pModelInfo->m_apStaticLight;

			d3d_SetupTexturesAndRelatedData(params.m_apTextures, params.m_adwMode, params.m_avModeColor, 
				params.m_afAlphaRef, aBlendStateOverride, aStencilStateOverride, 
				pCurrModelInstance, dwBaseRenderMode, eBaseBlendState);

			pRenderShader->SetPerObjectParamsVPS(&params);
		}

		uint32 dwTextureIndex = queuedItem.m_pModel->GetPiece(queuedItem.m_dwPieceIndex)->m_wTextureIndex;
		d3d_DrawModelPiece(&queuedItem, params.m_adwMode[dwTextureIndex], aBlendStateOverride[dwTextureIndex],
			aStencilStateOverride[dwTextureIndex]);
	}

	if (bInReallyClose)
		d3d_UnsetReallyClose(&reallyCloseData);

	g_RenderStateMgr.RestorePrimaryStates();

	queuedPieceInfo.clear();
}

#pragma warning(pop)

static void d3d_QueueModel(ViewParams* pParams, LTObject* pObject)
{
	ModelInstance* pModel = pObject->ToModel();

	if (!pModel->m_AnimTracker.IsValid())
		return;

	g_ModelSetup.QueueModel(pModel);
}

void d3d_QueueModel(ViewParams* pParams, LTObject* pObject, QueuedModelInfo* pInfo)
{
	ModelInstance* pModel = pObject->ToModel();

	if (!pModel->m_AnimTracker.IsValid())
		return;

	g_ModelSetup.QueueModel(pModel, pInfo);
}

bool d3d_IsTranslucentModel(LTObject* pObject)
{
	return pObject->m_nColorA < 255 || (pObject->m_dwFlags2 & FLAG2_ADDITIVE);
}

void d3d_ProcessModel(LTObject* pObject)
{
	if (!g_CV_DrawModels)
		return;

	VisibleSet* pVisibleSet = d3d_GetVisibleSet();

#ifndef MODEL_PIECELESS_RENDERING
	ModelInstance* pInstance = pObject->ToModel();
	g_ModelSetup.QueueModelInfo(pInstance);
#endif

	if (d3d_IsTranslucentModel(pObject))
	{
		pVisibleSet->GetTranslucentModels()->Add(pObject);
	}
	else
	{
		if (pObject->m_dwFlags2 & FLAG2_CHROMAKEY)
			pVisibleSet->GetChromakeyModels()->Add(pObject);
		else
			pVisibleSet->GetModels()->Add(pObject);
	}

#ifdef MODEL_PIECELESS_RENDERING
	ModelInstance* pInstance = pObject->ToModel();
#endif
	d3d_ProcessModelShadowLight(pInstance);
}

void d3d_DrawSolidModels(ViewParams* pParams)
{
	AllocSet* pSet = d3d_GetVisibleSet()->GetModels();
	if (!pSet->GetObjectCount())
		return;

#ifdef MODEL_PIECELESS_RENDERING
	pSet->Draw(pParams, d3d_DrawModel_Solid);
#else
	pSet->Draw(pParams, d3d_QueueModel);
	d3d_DrawModelPieceList(BLEND_STATE_Default, 0);
#endif
}

void d3d_DrawChromakeyModels(ViewParams* pParams)
{
	AllocSet* pSet = d3d_GetVisibleSet()->GetChromakeyModels();
	if (!pSet->GetObjectCount())
		return;

#ifdef MODEL_PIECELESS_RENDERING
	pSet->Draw(pParams, d3d_DrawModel_Chromakey);
#else
	pSet->Draw(pParams, d3d_QueueModel);
	d3d_DrawModelPieceList(BLEND_STATE_Alpha, MODE_CHROMAKEY);
#endif
}

static void d3d_DrawModelPieceListEx_Alpha(ViewParams* pParams, LTObject* pObject)
{
	g_GlobalMgr.GetSpriteBatchMgr()->RenderBatch();
	g_GlobalMgr.GetCanvasBatchMgr()->RenderBatch();

	d3d_QueueModel(pParams, pObject);
	d3d_DrawModelPieceList(BLEND_STATE_Alpha, 0);
}

void d3d_QueueTranslucentModels(ViewParams* pParams, ObjectDrawList* pDrawList)
{
#ifdef MODEL_PIECELESS_RENDERING
	d3d_GetVisibleSet()->GetTranslucentModels()->Queue(pDrawList, pParams, d3d_DrawModel_Alpha, 
		d3d_DrawModelEx_Alpha);
#else
	d3d_GetVisibleSet()->GetTranslucentModels()->Queue(pDrawList, pParams, d3d_DrawModelPieceList_Alpha, 
		d3d_DrawModelPieceListEx_Alpha);
#endif
}

void d3d_ModelPreFrame()
{
	g_aModelShadowLight.clear();
#ifndef MODEL_PIECELESS_RENDERING
	for (auto& pair : g_ModelSetup.GetQueuedModelInfo())
		pair.second.clear();
#endif
}