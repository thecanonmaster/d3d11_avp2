#include "pch.h"

#include "draw_polygrid.h"
#include "rendererconsolevars.h"
#include "tagnodes.h"
#include "renderstatemgr.h"
#include "d3d_draw.h"
#include "common_stuff.h"
#include "globalmgr.h"
#include "d3d_device.h"
#include "3d_ops.h"
#include "d3d_shader_polygrid.h"
#include "common_draw.h"
#include "rendermodemgr.h"

using namespace DirectX;

void d3d_GetFinalPolyGridTransform(ViewParams* pParams, DirectX::XMFLOAT4X4* pTransform, DirectX::XMFLOAT4X4* pTransformNoProj, 
	DirectX::XMFLOAT4X4* pGridTransform);
void d3d_SetBuffersAndTopology(CPolyGridData* pData);

static inline void GenerateNormal(char* pData, InternalPolyGridVertex* pVert, int32 nXOff1, int32 nXOff2, int32 nYOff1, int32 nYOff2,
	float fWidth, float fHeight, float fWidthTimesHeight, float fYScale)
{
	DirectX::XMVECTOR vTempNormal = DirectX::XMVectorSet(((int32)pData[nXOff1] - pData[nXOff2]) * fYScale * fHeight,
		fWidthTimesHeight, ((int32)pData[nYOff1] - pData[nYOff2]) * fYScale * fWidth, 0.0f);

	DirectX::XMStoreFloat3(&pVert->vNormal, DirectX::XMVector3Normalize(vTempNormal));
}

template <class Function>
static void GeneratePolyGridVectors(PolyGrid* pPolyGrid, InternalPolyGridVertex* pVert, Function pGenFunction)
{
	int nWidth = pPolyGrid->m_dwWidth;
	int nHeight = pPolyGrid->m_dwHeight;

	int nTotal = nWidth * nHeight;
	int nBottomRow = nWidth * (nHeight - 1);

	char* pData = pPolyGrid->m_pData;
	float fYScale = pPolyGrid->m_Base.m_vScale.y * 2.0f / 255.0f;
	float fWidth = 2.0f * pPolyGrid->m_fScaleX;
	float fHeight = 2.0f * pPolyGrid->m_fScaleY;
	float fWidthTimesHeight = fWidth * fHeight;
	float fTileXScale = fWidth / (float)(nWidth - 1);
	float fTileZScale = fHeight / (float)(nHeight - 1);

	pGenFunction(pData + 0, pVert + 0, 1, 0, nWidth, 0, fWidth, fHeight, fWidthTimesHeight, fYScale);
	pGenFunction(pData + nWidth - 1, pVert + nWidth - 1, -1, 0, 0, nWidth, fWidth, fHeight, fWidthTimesHeight, fYScale);
	pGenFunction(pData + nBottomRow, pVert + nBottomRow, 1, 0, 0, -nWidth, fWidth, fHeight, fWidthTimesHeight, fYScale);
	pGenFunction(pData + nTotal - 1, pVert + nTotal - 1, 0, -1, 0, -nWidth, fWidth, fHeight, fWidthTimesHeight, fYScale);

	int nCurrX, nCurrY;
	for (nCurrX = 1; nCurrX < nWidth - 1; nCurrX++)
	{
		pGenFunction(pData + nCurrX, pVert + nCurrX, -1, 1, 0, nWidth, fWidth, fHeight, fWidthTimesHeight, fYScale);
		pGenFunction(pData + nBottomRow + nCurrX, pVert + nBottomRow + nCurrX, -1, 1, -nWidth, 0, fWidth, fHeight, fWidthTimesHeight, fYScale);
	}

	for (nCurrY = nWidth; nCurrY < nTotal - nWidth; nCurrY += nWidth)
	{
		pGenFunction(pData + nCurrY, pVert + nCurrY, 0, 1, -nWidth, nWidth, fWidth, fHeight, fWidthTimesHeight, fYScale);
		pGenFunction(pData + nCurrY + nWidth - 1, pVert + nCurrY + nWidth - 1, 0, -1, nWidth, -nWidth, fWidth, fHeight, fWidthTimesHeight, fYScale);
	}

	for (nCurrY = 1; nCurrY < nHeight - 1; nCurrY++)
	{
		uint32 nPos = nCurrY * nWidth + 1;
		for (nCurrX = 1; nCurrX < nWidth - 1; nCurrX++)
		{
			pGenFunction(pData + nPos, pVert + nPos, -1, 1, -nWidth, nWidth, fWidth, fHeight, fWidthTimesHeight, fYScale);
			nPos++;
		}
	}
}

CPolyGridData::CPolyGridData(PolyGrid* pPolyGrid, uint32 dwVerts, uint32 dwIndices) : CExpirableData()
{
	m_InitialValues = { };

	m_dwVerts = dwVerts;
	m_dwIndices = dwIndices;
	m_pIndexData = new uint16[pPolyGrid->m_dwIndices];
	m_pTempBuffer = new InternalPolyGridVertex[dwVerts];
	InitVertexData(pPolyGrid);
	GeneratePolyGridVectors(pPolyGrid, m_pTempBuffer, GenerateNormal);
	InitIndexData(pPolyGrid);

	m_dwBankedVBIndex = UINT32_MAX;
	m_dwBankedIBIndex = UINT32_MAX;
}

void CPolyGridData::Term_VertexRelated()
{
	delete[] m_pTempBuffer;

	if (m_dwBankedVBIndex != UINT32_MAX)
		g_GlobalMgr.GetBankedVertexBuffer(m_dwBankedVBIndex).m_bInUse = false;
}

void CPolyGridData::Term_IndexRelated()
{
	delete[] m_pIndexData;

	if (m_dwBankedIBIndex != UINT32_MAX)
		g_GlobalMgr.GetBankedIndexBuffer(m_dwBankedIBIndex).m_bInUse = false;
}

CPolyGridData::CompareResultPair CPolyGridData::CompareTo(PolyGrid* pPolyGrid, uint32 dwVerts, uint32 dwIndices)
{
	std::pair<CPolyGridData::CompareResult, CPolyGridData::CompareResult> resultPair = { COMP_R_Unset, COMP_R_Unset };

	if (dwVerts > m_dwVerts)
		resultPair.first = COMP_R_SizeDiffMore;
	else if (dwVerts < m_dwVerts)
		resultPair.first = COMP_R_SizeDiff;

	if (resultPair.first == COMP_R_Unset)
	{
		if (m_InitialValues.CompareTo(pPolyGrid))
			resultPair.first = COMP_R_Equal;
		else
			resultPair.first = COMP_R_ContentDiff;
	}

	if (dwIndices > m_dwIndices)
	{
		resultPair.second = COMP_R_SizeDiffMore;
		return resultPair;
	}
	else if (dwIndices < m_dwIndices)
	{
		resultPair.second = COMP_R_SizeDiff;
		return resultPair;
	}

	if (memcmp(m_pIndexData, pPolyGrid->m_pIndices, dwIndices * sizeof(uint16)))
		resultPair.second = COMP_R_ContentDiff;
	else
		resultPair.second = COMP_R_Equal;

	return resultPair;
}

bool CPolyGridData::CreateVertexBuffer()
{
	m_dwBankedVBIndex = g_GlobalMgr.AssignBankedVertexBuffer(m_pTempBuffer, sizeof(InternalPolyGridVertex) * m_dwVerts);
	return (m_dwBankedVBIndex != UINT32_MAX);
}

bool CPolyGridData::CreateIndexBuffer()
{
	m_dwBankedIBIndex = g_GlobalMgr.AssignBankedIndexBuffer(m_pIndexData, sizeof(uint16) * m_dwIndices);
	return (m_dwBankedIBIndex != UINT32_MAX);
}

bool CPolyGridData::UpdateBuffers(PolyGrid* pPolyGrid, uint32 dwVerts, uint32 dwIndices)
{
	CompareResultPair pairResult = CompareTo(pPolyGrid, dwVerts, dwIndices);

	bool bVertexResult = UpdateVertexBuffer(pPolyGrid, dwVerts, pairResult.first);
	bool bIndexResult = UpdateIndexBuffer(pPolyGrid, dwIndices, pairResult.second);

	return bVertexResult && bIndexResult;
}

bool CPolyGridData::UpdateVertexBuffer(PolyGrid* pPolyGrid, uint32 dwVerts, CompareResult eResult)
{
	if (eResult == COMP_R_ContentDiff)
	{
		InitVertexData(pPolyGrid);
		GeneratePolyGridVectors(pPolyGrid, m_pTempBuffer, GenerateNormal);

		if (!g_GlobalMgr.UpdateGenericBuffer(g_GlobalMgr.GetBankedVertexBuffer(m_dwBankedVBIndex).m_pBuffer, 
			m_pTempBuffer, sizeof(InternalPolyGridVertex) * dwVerts, D3D11_MAP_WRITE_DISCARD))
		{
			return false;
		}
	}
	else if (eResult == COMP_R_SizeDiff)
	{
		m_dwVerts = dwVerts;
		InitVertexData(pPolyGrid);
		GeneratePolyGridVectors(pPolyGrid, m_pTempBuffer, GenerateNormal);

		if (!g_GlobalMgr.UpdateGenericBuffer(g_GlobalMgr.GetBankedVertexBuffer(m_dwBankedVBIndex).m_pBuffer, 
			m_pTempBuffer, sizeof(InternalPolyGridVertex) * dwVerts, D3D11_MAP_WRITE_DISCARD))
		{
			return false;
		}
	}
	else if (eResult == COMP_R_SizeDiffMore)
	{
		Term_VertexRelated();

		m_pTempBuffer = new InternalPolyGridVertex[dwVerts];
		m_dwVerts = dwVerts;
		InitVertexData(pPolyGrid);
		GeneratePolyGridVectors(pPolyGrid, m_pTempBuffer, GenerateNormal);

		if (!CreateVertexBuffer())
			return false;
	}

	return true;
}

bool CPolyGridData::UpdateIndexBuffer(PolyGrid* pPolyGrid, uint32 dwIndices, CompareResult eResult)
{
	if (eResult == COMP_R_ContentDiff)
	{
		InitIndexData(pPolyGrid);
		
		if (!g_GlobalMgr.UpdateGenericBuffer(g_GlobalMgr.GetBankedIndexBuffer(m_dwBankedIBIndex).m_pBuffer, 
			m_pIndexData, sizeof(uint16) * m_dwIndices, D3D11_MAP_WRITE_DISCARD))
		{
			return false;
		}
	}
	else if (eResult == COMP_R_SizeDiff)
	{	
		m_dwIndices = dwIndices;
		InitIndexData(pPolyGrid);

		if (!g_GlobalMgr.UpdateGenericBuffer(g_GlobalMgr.GetBankedIndexBuffer(m_dwBankedIBIndex).m_pBuffer, 
			m_pIndexData, sizeof(uint16) * dwIndices, D3D11_MAP_WRITE_DISCARD))
		{
			return false;
		}
	}
	else if (eResult == COMP_R_SizeDiffMore)
	{
		Term_IndexRelated();

		m_pIndexData = new uint16[dwIndices];
		m_dwIndices = dwIndices;
		InitIndexData(pPolyGrid);

		if (!CreateIndexBuffer())
			return false;
	}

	return true;
}

inline void CPolyGridData::CalcVertexDataEssentials(PolyGrid* pPolyGrid, VertexDataEssentials* pParams)
{
	constexpr float c_fPanSpeed = 0.001f;

	float fGridWidthMinusOne = (float)(pPolyGrid->m_dwWidth - 1);
	float fGridHeightMinusOne = (float)(pPolyGrid->m_dwHeight - 1);
	float fHalfGridWidth = fGridWidthMinusOne * 0.5f;
	float fHalfGridHeight = fGridHeightMinusOne * 0.5f;

	pParams->fXInc = pPolyGrid->m_Base.m_vScale.x * (float)pPolyGrid->m_dwWidth / fGridWidthMinusOne;
	pParams->fZInc = pPolyGrid->m_Base.m_vScale.z * (float)pPolyGrid->m_dwHeight / fGridHeightMinusOne;

	pParams->fYScale = pPolyGrid->m_Base.m_vScale.y * 2.0f;

	pParams->fXStart = -fHalfGridWidth * pParams->fXInc;
	pParams->fCurrX = pParams->fXStart;
	pParams->fCurrZ = -fHalfGridHeight * pParams->fZInc;

	float fMinInc = std::fmin(pParams->fXInc, pParams->fZInc);
	float fXScale = pPolyGrid->m_fScaleX / (fGridWidthMinusOne * fMinInc);
	float fZScale = pPolyGrid->m_fScaleY / (fGridHeightMinusOne * fMinInc);

	pParams->fStartU = fmod(pPolyGrid->m_fPanX * c_fPanSpeed, 1.0f);
	float fStartV = fmod(pPolyGrid->m_fPanY * c_fPanSpeed, 1.0f);

	pParams->fCurrU = pParams->fStartU;
	pParams->fCurrV = fStartV;

	// TODO - a wild guess
	pParams->fUInc = pParams->fXInc * fXScale * 0.1f;
	pParams->fVInc = pParams->fZInc * fZScale * 0.1f;
}

void CPolyGridData::InitVertexData(PolyGrid* pPolyGrid)
{
	m_InitialValues.Init(pPolyGrid);
	
	VertexDataEssentials params;
	CalcVertexDataEssentials(pPolyGrid, &params);

	char* pDataPos = pPolyGrid->m_pData;
	char* pDataEnd = pDataPos + pPolyGrid->m_dwWidth * pPolyGrid->m_dwHeight;
	char* pLineDataEnd = pDataPos + pPolyGrid->m_dwWidth;

	InternalPolyGridVertex* pVertexPos = m_pTempBuffer;

	while (pDataPos < pDataEnd)
	{
		while (pDataPos < pLineDataEnd)
		{
			pVertexPos->vPosition.x = params.fCurrX;
			pVertexPos->vPosition.y = *pDataPos * params.fYScale;
			pVertexPos->vPosition.z = params.fCurrZ;

			int nColorTablePos = (POLY_GRID_COLOR_TABLE_SIZE >> 1) + *pDataPos;
			pVertexPos->vDiffuseColor.x = (float)pPolyGrid->m_aColorTable[nColorTablePos].r * MATH_ONE_OVER_255;
			pVertexPos->vDiffuseColor.y = (float)pPolyGrid->m_aColorTable[nColorTablePos].g * MATH_ONE_OVER_255;
			pVertexPos->vDiffuseColor.z = (float)pPolyGrid->m_aColorTable[nColorTablePos].b * MATH_ONE_OVER_255;
			pVertexPos->vDiffuseColor.w = (float)pPolyGrid->m_aColorTable[nColorTablePos].a * MATH_ONE_OVER_255;
			pVertexPos->vTexCoords.x = params.fCurrU;
			pVertexPos->vTexCoords.y = params.fCurrV;

			pDataPos++;
			pVertexPos++;
			params.fCurrX += params.fXInc;
			params.fCurrU += params.fUInc;
		}

		params.fCurrX = params.fXStart;

		pLineDataEnd += pPolyGrid->m_dwWidth;

		params.fCurrZ += params.fZInc;
		params.fCurrU = params.fStartU;
		params.fCurrV += params.fVInc;
	}
}

void CPolyGridData::InitIndexData(PolyGrid* pPolyGrid)
{
	memcpy(m_pIndexData, pPolyGrid->m_pIndices, sizeof(uint16) * m_dwIndices);
}

CPolyGridData* CPolyGridManager::GetPolyGridData(PolyGrid* pPolyGrid)
{
	uint32 dwVerts = pPolyGrid->m_dwWidth * pPolyGrid->m_dwHeight;
	uint32 dwIndices = pPolyGrid->m_dwIndices;

	if (!dwVerts || !dwIndices)
		return nullptr;

	auto iter = m_Data.find(pPolyGrid);

	if (iter != m_Data.end())
	{
		CPolyGridData* pData = (*iter).second;

		pData->UpdateBuffers(pPolyGrid, dwVerts, dwIndices);
		pData->SetLastUpdate(g_fLastClientTime);

		return pData;
	}

	CPolyGridData* pNewData = new CPolyGridData(pPolyGrid, dwVerts, dwIndices);

	if (!pNewData->CreateVertexBuffer() || !pNewData->CreateIndexBuffer())
	{
		delete pNewData;
		return nullptr;
	}

	pNewData->SetLastUpdate(g_fLastClientTime);
	m_Data[pPolyGrid] = pNewData;

	return pNewData;
}

void d3d_DrawPolyGrid(ViewParams* pParams, LTObject* pObj)
{
	PolyGrid* pGrid = pObj->ToPolyGrid();

	if (pGrid->m_pData == nullptr || pGrid->m_pIndices == nullptr)
		return;

	if ((pGrid->m_dwWidth < MIN_POLYGRID_SIZE) || (pGrid->m_dwHeight < MIN_POLYGRID_SIZE))
		return;

	DirectX::XMFLOAT4X4 mGridTransform;
	LTVector vUnitScale = { 1.0f, 1.0f, 1.0f };
	d3d_SetupTransformation(&pGrid->m_Base.m_vPos, (float*)&pGrid->m_Base.m_rRot, &vUnitScale, &mGridTransform);

	SharedTexture* pBaseTexture = nullptr;
	SharedTexture* pEnvMapTexture = nullptr;

	if (pGrid->m_pSprite != nullptr)
	{
		SpriteTracker* pTracker = &pGrid->m_SpriteTracker;
		if (pTracker->m_pCurFrame != nullptr)
		{
			pBaseTexture = pTracker->m_pCurFrame->m_pTexture;
			if (pBaseTexture != nullptr && g_CV_EnvMapPolyGrids.m_Val)
			{
				pEnvMapTexture = pGrid->m_pEnvMap;
			}
		}
	}

	pGrid->m_Base.m_dwFlags |= FLAG_INTERNAL1;

	CPolyGridData* pPolyGridData = g_GlobalMgr.GetPolyGridMgr()->GetPolyGridData(pGrid);

	if (pPolyGridData == nullptr)
		return;

	uint32 dwRenderMode = 0;
	BlendState eBlendState;

	// TODO - g_CV_EnvMapWorld?
	if (pEnvMapTexture != nullptr && g_CV_EnvMapEnable.m_Val)
		dwRenderMode |= MODE_ENV_MAP;

	bool bEnvMapOnly = false;

	if (pGrid->m_Base.m_dwFlags & FLAG_ENVIRONMENTMAPONLY)
	{
		bEnvMapOnly = true;
		dwRenderMode |= MODE_ENV_MAP_ONLY;
	}

	g_RenderStateMgr.SavePrimaryStates();
	
	g_RenderModeMgr.SetupRenderMode_PolyGrid(bEnvMapOnly ? pEnvMapTexture : pBaseTexture,
		pObj->m_dwFlags, pObj->m_dwFlags2, eBlendState, dwRenderMode, BLEND_STATE_Alpha);

	g_RenderStateMgr.SetBlendState(eBlendState);
	g_RenderStateMgr.SetRasterState(!g_CV_Wireframe.m_Val ? RASTER_STATE_Default : RASTER_STATE_Wireframe);

	DirectX::XMFLOAT4 vColorScale =
	{
		(float)pGrid->m_Base.m_nColorR * MATH_ONE_OVER_255,
		(float)pGrid->m_Base.m_nColorG * MATH_ONE_OVER_255,
		(float)pGrid->m_Base.m_nColorB * MATH_ONE_OVER_255,
		(float)pGrid->m_Base.m_nColorA * MATH_ONE_OVER_255,
	};

	// TODO - not needed?
	/*if (pEnvMapTexture == nullptr || (pEnvMapTexture != nullptr && bEnvMapOnly))
		vColorScale.w = 1.0f;
	else
		vColorScale.w = 0.5f;*/

	d3d_SetBuffersAndTopology(pPolyGridData);

	CRenderShader_PolyGrid* pRenderShader = g_RenderShaderMgr.GetRenderShader<CRenderShader_PolyGrid>();

	DirectX::XMFLOAT3 vModeColor = { };
	SharedTexture* pStateChangeTexture = (pEnvMapTexture != nullptr && bEnvMapOnly) ? pEnvMapTexture : pBaseTexture;

	if (pStateChangeTexture != nullptr && pStateChangeTexture->m_pStateChange != nullptr)
	{
		BlendState eBlendStateOverride = BLEND_STATE_Invalid;
		StencilState eStencilStateOverride = STENCIL_STATE_Invalid;

		g_RenderModeMgr.ApplyStateChange(pStateChangeTexture->m_pStateChange, eBlendStateOverride, eStencilStateOverride,
			dwRenderMode, &vModeColor);

		if (eBlendStateOverride != BLEND_STATE_Invalid)
			g_RenderStateMgr.SetBlendState(eBlendStateOverride);

		if (eStencilStateOverride != STENCIL_STATE_Invalid)
			g_RenderStateMgr.SetStencilState(eStencilStateOverride);
	}

	XMFloat4x4Trinity sTransforms;
	sTransforms.m_mWorld = mGridTransform;

	d3d_GetFinalPolyGridTransform(pParams, &sTransforms.m_mWorldViewProj, &sTransforms.m_mWorldView, &mGridTransform);

	if (!pRenderShader->SetPerObjectParams(pBaseTexture != nullptr ? ((RTexture*)pBaseTexture->m_pRenderData)->m_pResourceView : nullptr,
		pEnvMapTexture != nullptr ? ((RTexture*)pEnvMapTexture->m_pRenderData)->m_pResourceView : nullptr, 
		dwRenderMode, &vModeColor, &vColorScale, &sTransforms))
	{
		return;
	}

	pRenderShader->Render(pPolyGridData->GetIndexCount());

	g_RenderStateMgr.RestorePrimaryStates();
}

void d3d_DrawPolyGridEx(ViewParams* pParams, LTObject* pObj, LTObject* pPrevObject)
{
	g_GlobalMgr.GetSpriteBatchMgr()->RenderBatch();
	g_GlobalMgr.GetCanvasBatchMgr()->RenderBatch();

	d3d_DrawPolyGrid(pParams, pObj);
}

static void d3d_GetFinalPolyGridTransform(ViewParams* pParams, DirectX::XMFLOAT4X4* pTransform,
	DirectX::XMFLOAT4X4* pTransformNoProj, DirectX::XMFLOAT4X4* pGridTransform)
{
	DirectX::XMMATRIX mTransform;

	mTransform = DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(pGridTransform)) *
		DirectX::XMLoadFloat4x4(&pParams->m_mViewTransposed);

	DirectX::XMStoreFloat4x4(pTransformNoProj, mTransform);

	mTransform *= DirectX::XMLoadFloat4x4(&pParams->m_mProjectionTransposed);

	DirectX::XMStoreFloat4x4(pTransform, mTransform);
}

static void d3d_SetBuffersAndTopology(CPolyGridData* pData)
{
	g_RenderShaderMgr.SetVertexResource(VRS_Main, g_GlobalMgr.GetBankedVertexBuffer(pData->m_dwBankedVBIndex).m_pBuffer,
		sizeof(InternalPolyGridVertex), 0);
	g_RenderShaderMgr.SetIndexBuffer16(g_GlobalMgr.GetBankedIndexBuffer(pData->m_dwBankedIBIndex).m_pBuffer, 0);
	g_RenderShaderMgr.SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

bool d3d_IsTranslucentPolyGrid(LTObject* pObject)
{
	return pObject->m_nColorA < 255 || (pObject->m_dwFlags2 & FLAG2_ADDITIVE);
}

void d3d_ProcessPolyGrid(LTObject* pObject)
{
	if (!g_CV_DrawPolyGrids)
		return;

	VisibleSet* pVisibleSet = d3d_GetVisibleSet();

	if (d3d_IsTranslucentPolyGrid(pObject))
		pVisibleSet->GetTranslucentPolyGrids()->Add(pObject);
	else
		pVisibleSet->GetPolyGrids()->Add(pObject);
}

void d3d_DrawSolidPolyGrids(ViewParams* pParams)
{
	d3d_GetVisibleSet()->GetPolyGrids()->Draw(pParams, d3d_DrawPolyGrid);
}

void d3d_QueueTranslucentPolyGrids(ViewParams* pParams, ObjectDrawList* pDrawList)
{
	d3d_GetVisibleSet()->GetTranslucentPolyGrids()->Queue(pDrawList, pParams, d3d_DrawPolyGrid, 
		d3d_DrawPolyGridEx);
}
