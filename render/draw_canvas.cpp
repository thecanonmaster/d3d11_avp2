#include "pch.h"

#include "draw_canvas.h"
#include "rendererconsolevars.h"
#include "common_stuff.h"
#include "renderstatemgr.h"
#include "d3d_texture.h"
#include "d3d_draw.h"
#include "d3d_device.h"
#include "3d_ops.h"
#include "d3d_mathhelpers.h"
#include "globalmgr.h"
#include "d3d_shader_canvas.h"
#include "common_draw.h"
#include "rendermodemgr.h"

InternalCustomDraw g_InternalCustomDraw;

void d3d_SetBuffersAndTopology(CCanvasData* pData);
void d3d_GetFinalCanvasTransform(ViewParams* pParams, DirectX::XMFLOAT4X4* pTransform, 
DirectX::XMFLOAT4X4* pTransformNoProj, DirectX::XMFLOAT4X4* pReallyCloseProj);
void d3d_GetFinalCanvasTransform_Normal(ViewParams* pParams, DirectX::XMFLOAT4X4* pTransform, 
	DirectX::XMFLOAT4X4* pTransformNoProj);
void d3d_GetFinalCanvasTransform_ReallyClose(DirectX::XMFLOAT4X4* pTransform, DirectX::XMFLOAT4X4* pReallyCloseProj);

void CCanvasData::Term()
{
	delete[] m_pData;

	if (m_dwBankedVBIndex != UINT32_MAX)
		g_GlobalMgr.GetBankedVertexBuffer(m_dwBankedVBIndex).m_bInUse = false;
}

CExpirableData::CompareResult CCanvasData::CompareTo(LTVertex* pOthers, uint32 dwVerts)
{
	if (dwVerts > m_dwVerts)
		return COMP_R_SizeDiffMore;
	else if (dwVerts < m_dwVerts)
		return COMP_R_SizeDiff;

#ifndef CANVAS_DATA_MEM_CMP
	for (uint32 i = 0; i < dwVerts; i++)
	{
		if (!m_pData[i].CompareTo(&pOthers[i]))
			return COMP_R_ContentDiff;
	}
#else
	if (memcmp(m_pData, pOthers, sizeof(LTVertex) * dwVerts))
		return CD_CR_CONTENT_DIFF;
#endif

	return COMP_R_Equal;
}

bool CCanvasData::CreateVertexBuffer()
{
	InternalCanvasVertex* pTempBuffer = m_dwVerts > CANVAS_TEMP_BUFFER_LEN ? 
		new InternalCanvasVertex[m_dwVerts] : 
		g_GlobalMgr.GetCanvasMgr()->GetTempBuffer();
	
	for (uint32 i = 0; i < m_dwVerts; i++)
	{
		// TODO - won't work with m_dwVerts > 4 ?
		uint32 dwNewIndex = (i % 2 == 0) ? (i >> 1) : m_dwVerts - 1 - (i >> 1);

		pTempBuffer[i].vPosition = *PLTVECTOR_TO_PXMFLOAT3(&m_pData[dwNewIndex].m_vVec);

		pTempBuffer[i].vDiffuseColor.x = (float)m_pData[dwNewIndex].m_Color.nR * MATH_ONE_OVER_255;
		pTempBuffer[i].vDiffuseColor.y = (float)m_pData[dwNewIndex].m_Color.nG * MATH_ONE_OVER_255;
		pTempBuffer[i].vDiffuseColor.z = (float)m_pData[dwNewIndex].m_Color.nB * MATH_ONE_OVER_255;
		pTempBuffer[i].vDiffuseColor.w = (float)m_pData[dwNewIndex].m_Color.nA * MATH_ONE_OVER_255;

		pTempBuffer[i].vTexCoords.x = m_pData[dwNewIndex].m_fTU;
		pTempBuffer[i].vTexCoords.y = m_pData[dwNewIndex].m_fTV;
	}

	m_dwBankedVBIndex = g_GlobalMgr.AssignBankedVertexBuffer(pTempBuffer, sizeof(InternalCanvasVertex) * m_dwVerts);

	if (m_dwVerts > CANVAS_TEMP_BUFFER_LEN)
		delete [] pTempBuffer;

	return (m_dwBankedVBIndex != UINT32_MAX);
}

bool CCanvasData::UpdateVertexBuffer(LTVertex* pNewVerts, uint32 dwVerts)
{
	CompareResult eResult = CompareTo(pNewVerts, dwVerts);

	auto lambdaUpdate = [dwVerts, pNewVerts](D3D11_MAPPED_SUBRESOURCE* pSubResource)
		{
			InternalCanvasVertex* pVertices = (InternalCanvasVertex*)pSubResource->pData;

			for (uint32 i = 0; i < dwVerts; i++)
			{
				uint32 dwNewIndex = (i % 2 == 0) ? (i >> 1) : dwVerts - 1 - (i >> 1);

				pVertices[i].vPosition = *PLTVECTOR_TO_PXMFLOAT3(&pNewVerts[dwNewIndex].m_vVec);

				pVertices[i].vDiffuseColor.x = (float)pNewVerts[dwNewIndex].m_Color.nR * MATH_ONE_OVER_255;
				pVertices[i].vDiffuseColor.y = (float)pNewVerts[dwNewIndex].m_Color.nG * MATH_ONE_OVER_255;
				pVertices[i].vDiffuseColor.z = (float)pNewVerts[dwNewIndex].m_Color.nB * MATH_ONE_OVER_255;
				pVertices[i].vDiffuseColor.w = (float)pNewVerts[dwNewIndex].m_Color.nA * MATH_ONE_OVER_255;

				pVertices[i].vTexCoords.x = pNewVerts[dwNewIndex].m_fTU;
				pVertices[i].vTexCoords.y = pNewVerts[dwNewIndex].m_fTV;
			}
		};

	if (eResult == COMP_R_ContentDiff)
	{	
		memcpy(m_pData, pNewVerts, sizeof(LTVertex) * dwVerts);

		if (!g_GlobalMgr.UpdateGenericBufferEx(g_GlobalMgr.GetBankedVertexBuffer(m_dwBankedVBIndex).m_pBuffer,
			D3D11_MAP_WRITE_DISCARD, lambdaUpdate))
		{
			return false;
		}
	}
	else if (eResult == COMP_R_SizeDiff)
	{
		m_dwVerts = dwVerts;
		memcpy(m_pData, pNewVerts, sizeof(LTVertex) * dwVerts);

		if (!g_GlobalMgr.UpdateGenericBufferEx(g_GlobalMgr.GetBankedVertexBuffer(m_dwBankedVBIndex).m_pBuffer,
			D3D11_MAP_WRITE_DISCARD, lambdaUpdate))
		{
			return false;
		}
	}
	else if (eResult == COMP_R_SizeDiffMore)
	{
		Term();
		
		m_pData = new LTVertex[dwVerts];
		m_dwVerts = dwVerts;
		memcpy(m_pData, pNewVerts, sizeof(LTVertex) * dwVerts);

		if (!CreateVertexBuffer())
			return false;
	}

	return true;
}

CCanvasData* CCanvasManager::GetCanvasData(SharedTexture* pTexture, LTVertex* pVerts, uint32 dwVerts)
{
	if (!dwVerts)
		return nullptr;

	auto iter = m_Data.find(pTexture);

	if (iter != m_Data.end())
	{
		CCanvasData* pData = (*iter).second;

		pData->UpdateVertexBuffer(pVerts, dwVerts);
		pData->SetLastUpdate(g_fLastClientTime);

		return pData;
	}

	CCanvasData* pNewData = new CCanvasData(dwVerts);
	memcpy(pNewData->GetData(), pVerts, sizeof(LTVertex) * dwVerts);

	if (!pNewData->CreateVertexBuffer())
	{
		delete pNewData;
		return nullptr;
	}

	pNewData->SetLastUpdate(g_fLastClientTime);
	m_Data[pTexture] = pNewData;

	return pNewData;
}

void CCanvasBatchManager::FreeAllData()
{
	RELEASE_INTERFACE(m_pVertexBuffer, g_szFreeError_VB);
}

void CCanvasBatchManager::Init()
{
	CBaseDataManager::Init();

	m_dwVertexCount = 0;
	m_dwCanvasCount = 0;
	m_dwTextureCount = 0;
	m_bHasReallyCloseItems = false;
	m_pMappedVertexData = nullptr;

	memset(m_apSRV, 0, sizeof(m_apSRV));
	m_aCommand.reserve(CANVAS_BATCH_SIZE);

	m_pVertexBuffer = g_GlobalMgr.CreateVertexBuffer(
		sizeof(InternalCanvasBatchVertex) * CANVAS_COMMON_VERTICES * CANVAS_BATCH_SIZE,
		D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
}

void CCanvasBatchManager::BatchPrimitive(LTVertex* pVerts, uint32 dwVerts, uint32 dwFlags, InternalCustomDraw* pDraw)
{
	uint32 dwMappedVerts = (dwVerts - 2) * 3;
	constexpr uint32 c_dwVertsPerBuffer = CANVAS_COMMON_VERTICES * CANVAS_BATCH_SIZE;

	// TODO - split into two if there is no room for the primitive
	if (IsFull() || (m_dwVertexCount + dwMappedVerts > c_dwVertsPerBuffer))
		InternalRenderBatch();

	if (!MapVertexBuffer())
		return;

	uint32 dwRenderMode = 0;
	SharedTexture* pTexture = pDraw->GetTexture();

	g_RenderModeMgr.SetupRenderMode_Canvas(pTexture, dwFlags, pDraw->GetCanvas()->m_Base.m_dwFlags2,
		dwRenderMode);

	if (dwFlags & FLAG_REALLYCLOSE)
		dwRenderMode |= MODE_REALLY_CLOSE;

	BlendState eBlendState = BLEND_STATE_Default;
	StencilState eStencilState = STENCIL_STATE_Default;
	pDraw->TranslateStates(dwRenderMode, eBlendState, eStencilState);

	d3d_BindTexture(pTexture, FALSE);

	CRenderShader_CanvasBatch::VPSPerObjectParams& params = m_aParams[m_dwCanvasCount];

	UpdateVertexBuffer(pVerts, dwVerts, m_dwVertexCount, m_dwCanvasCount);

	if (pTexture != nullptr && pTexture->m_pStateChange != nullptr)
	{
		BlendState eBlendStateOverride = BLEND_STATE_Invalid;
		StencilState eStencilStateOverride = STENCIL_STATE_Invalid;

		g_RenderModeMgr.ApplyStateChange(pTexture->m_pStateChange, eBlendStateOverride, eStencilStateOverride,
			dwRenderMode, &params.m_vModeColor);

		if (eBlendStateOverride != BLEND_STATE_Invalid)
			eBlendState = eBlendStateOverride;

		if (eStencilStateOverride != STENCIL_STATE_Invalid)
			eStencilState = eStencilStateOverride;
	}

	params.m_dwMode = dwRenderMode;
	params.m_dwTextureIndex = 
		pTexture == nullptr ? UINT32_MAX : BatchTexture(((RTexture*)pTexture->m_pRenderData)->m_pResourceView);

	AppendCommand(eBlendState, eStencilState, !!(dwRenderMode & MODE_REALLY_CLOSE), m_dwVertexCount, dwMappedVerts);

	m_dwCanvasCount++;
}

void CCanvasBatchManager::InternalRenderBatch()
{
	if (!m_dwCanvasCount)
		return;

	if (m_pMappedVertexData != nullptr)
	{
		g_GlobalMgr.UnmapGenericBuffer(m_pVertexBuffer);
		m_pMappedVertexData = nullptr;
	}

	SetBuffersAndTopology();

	CRenderShader_CanvasBatch* pRenderShader = g_RenderShaderMgr.GetRenderShader<CRenderShader_CanvasBatch>();

	XMFloat4x4Trinity sTransforms;
	XMFloat4x4Trinity sTransformsRC;

	sTransforms.m_mWorld = g_mIdentity;
	sTransforms.m_mWorldView = sTransforms.m_mWorld;
	
	if (m_bHasReallyCloseItems)
	{
		sTransformsRC = sTransforms;
		DirectX::XMFLOAT4X4 mReallyCloseProj;

		d3d_CalcReallyCloseMatrix(&mReallyCloseProj, g_CV_RCCanvasFOVOffset.m_Val, g_CV_RCCanvasFOVOffset.m_Val, false);
		d3d_GetFinalCanvasTransform_ReallyClose(&sTransformsRC.m_mWorldViewProj, &mReallyCloseProj);
	}

	d3d_GetFinalCanvasTransform_Normal(&g_ViewParams, &sTransforms.m_mWorldViewProj, &sTransforms.m_mWorldView);

	if (!pRenderShader->SetPerObjectParams(m_apSRV, m_dwTextureCount, &sTransforms, &sTransformsRC, m_aParams, m_dwCanvasCount))
		return;

	g_RenderStateMgr.SavePrimaryStates();

	g_RenderStateMgr.SetRasterState(!g_CV_Wireframe.m_Val ? RASTER_STATE_Default : RASTER_STATE_Wireframe);

	for (CCanvasBatchManager::Command& command : m_aCommand)
	{
		CReallyCloseData reallyCloseData;
		if (command.m_bReallyClose)
			d3d_SetReallyClose(&reallyCloseData);

		g_RenderStateMgr.SetBlendState(command.m_eBlendState);
		g_RenderStateMgr.SetStencilState(command.m_eStencilState);

		pRenderShader->Render(command.m_dwVertexCount, command.m_dwVertexOffset);

		if (command.m_bReallyClose)
			d3d_UnsetReallyClose(&reallyCloseData);
	}

	g_RenderStateMgr.RestorePrimaryStates();

	// TODO - not needed?
	for (uint32 i = 0; i < m_dwTextureCount; i++)
		m_apSRV[i]->Release();

	m_dwVertexCount = 0;
	m_dwCanvasCount = 0;
	m_dwTextureCount = 0;
	m_bHasReallyCloseItems = false;

	m_aCommand.clear();
}

uint32 CCanvasBatchManager::BatchTexture(ID3D11ShaderResourceView* pSRV)
{
	for (uint32 i = 0; i < m_dwTextureCount; i++)
	{
		if (m_apSRV[i] == pSRV)
			return i;
	}

	m_apSRV[m_dwTextureCount] = pSRV;
	m_apSRV[m_dwTextureCount]->AddRef(); // TODO - not needed?
	m_dwTextureCount++;

	return m_dwTextureCount - 1;
}

static inline void InitBatchCanvasVertex(InternalCanvasBatchVertex* pMappedVerts, uint32 dwMappedIndex, LTVertex* pVerts,
	uint32 dwIndex, uint32 dwDataIndex)
{
	pMappedVerts[dwMappedIndex].vPosition = *PLTVECTOR_TO_PXMFLOAT3(&pVerts[dwIndex].m_vVec);

	pMappedVerts[dwMappedIndex].vDiffuseColor.x = (float)pVerts[dwIndex].m_Color.nR * MATH_ONE_OVER_255;
	pMappedVerts[dwMappedIndex].vDiffuseColor.y = (float)pVerts[dwIndex].m_Color.nG * MATH_ONE_OVER_255;
	pMappedVerts[dwMappedIndex].vDiffuseColor.z = (float)pVerts[dwIndex].m_Color.nB * MATH_ONE_OVER_255;
	pMappedVerts[dwMappedIndex].vDiffuseColor.w = (float)pVerts[dwIndex].m_Color.nA * MATH_ONE_OVER_255;

	pMappedVerts[dwMappedIndex].vTexCoords.x = pVerts[dwIndex].m_fTU;
	pMappedVerts[dwMappedIndex].vTexCoords.y = pVerts[dwIndex].m_fTV;

	pMappedVerts[dwMappedIndex].dwDataIndex = dwDataIndex;
}

void CCanvasBatchManager::UpdateVertexBuffer(LTVertex* pVerts, uint32 dwVerts, uint32 dwStart, uint32 dwDataIndex)
{
	InternalCanvasBatchVertex* pMappedVerts = &m_pMappedVertexData[dwStart];
	
	uint32 dwMappedIndex = 0;
	for (uint32 dwIndex = 0; dwIndex < dwVerts; dwIndex++)
	{
		if (dwIndex > 2)
		{
			InitBatchCanvasVertex(pMappedVerts, dwMappedIndex, pVerts, 0, dwDataIndex);
			dwMappedIndex++;

			InitBatchCanvasVertex(pMappedVerts, dwMappedIndex, pVerts, dwIndex - 1, dwDataIndex);
			dwMappedIndex++;
		}

		InitBatchCanvasVertex(pMappedVerts, dwMappedIndex, pVerts, dwIndex, dwDataIndex);
		dwMappedIndex++;
	}
}

bool CCanvasBatchManager::MapVertexBuffer()
{
	if (m_pMappedVertexData == nullptr)
	{
		D3D11_MAPPED_SUBRESOURCE subResource;

		if (!g_GlobalMgr.MapGenericBuffer(m_pVertexBuffer, D3D11_MAP_WRITE_DISCARD, &subResource))
			return false;

		m_pMappedVertexData = (InternalCanvasBatchVertex*)subResource.pData;
	}

	return true;
}

void CCanvasBatchManager::SetBuffersAndTopology()
{
	g_RenderShaderMgr.SetVertexResource(VRS_Main, m_pVertexBuffer, sizeof(InternalCanvasBatchVertex), 0);
	g_RenderShaderMgr.SetIndexBuffer16(nullptr, 0);
	g_RenderShaderMgr.SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void CCanvasBatchManager::AppendCommand(BlendState eBlendState, StencilState eStencilState, bool bReallyClose,
	uint32 dwVertexOffset, uint32 dwVertexCount)
{
	if (bReallyClose)
		m_bHasReallyCloseItems = true;

	if (m_aCommand.empty())
	{
		m_aCommand.emplace_back(eBlendState, eStencilState, bReallyClose, dwVertexOffset, dwVertexCount);
		m_dwVertexCount += dwVertexCount;
		return;
	}

	Command& command = m_aCommand.back();

	if (command.m_eBlendState == eBlendState && command.m_eStencilState == eStencilState &&
		command.m_bReallyClose == bReallyClose)
	{
		command.m_dwVertexCount += dwVertexCount;
	}
	else
	{
		m_aCommand.emplace_back(eBlendState, eStencilState, bReallyClose, dwVertexOffset, dwVertexCount);
	}

	m_dwVertexCount += dwVertexCount;
}

InternalCustomDraw::InternalCustomDraw()
{
	m_adwLTRStates[LTRSTATE_ALPHABLENDENABLE] = 0;
	m_adwLTRStates[LTRSTATE_ZREADENABLE] = 1;
	m_adwLTRStates[LTRSTATE_ZWRITEENABLE] = 1;
	m_adwLTRStates[LTRSTATE_SRCBLEND] = LTBLEND_SRCALPHA;
	m_adwLTRStates[LTRSTATE_DESTBLEND] = LTBLEND_INVSRCALPHA;
	m_adwLTRStates[LTRSTATE_TEXADDR] = LTTEXADDR_WRAP;
	m_adwLTRStates[LTRSTATE_COLOROP] = LTOP_MODULATE;
	m_adwLTRStates[LTRSTATE_ALPHAOP] = LTOP_SELECTDIFFUSE; // LTOP_SELECTTEXTURE according to SDK

	m_pCanvas = nullptr;
	m_pTexture = nullptr;
	m_bExtendedDraw = false;
}

LTRESULT InternalCustomDraw::DrawPrimitive(LTVertex* pVerts, uint32 dwVerts, uint32 dwFlags)
{
	if (m_bExtendedDraw)
		g_GlobalMgr.GetSpriteBatchMgr()->RenderBatch();

	if (g_CV_BatchCanvases.m_Val)
	{
		if (dwVerts < 3)
			return LT_OK;
		
		g_GlobalMgr.GetCanvasBatchMgr()->BatchPrimitive(pVerts, dwVerts, dwFlags, this);
		return LT_OK;
	}

	g_RenderStateMgr.SavePrimaryStates();

	uint32 dwRenderMode = 0;

	// TODO - object flags / texture overrides or canvas specific states go first?
	g_RenderModeMgr.SetupRenderMode_Canvas(m_pTexture, dwFlags, m_pCanvas->m_Base.m_dwFlags2, dwRenderMode);
	g_RenderStateMgr.SetRasterState(!g_CV_Wireframe.m_Val ? RASTER_STATE_Default : RASTER_STATE_Wireframe);

	BlendState eBlendState = BLEND_STATE_Default;
	StencilState eStencilState = STENCIL_STATE_Default;
	TranslateStates(dwRenderMode, eBlendState, eStencilState);
	
	g_RenderStateMgr.SetStencilState(eStencilState);
	g_RenderStateMgr.SetBlendState(eBlendState);

	d3d_BindTexture(m_pTexture, FALSE);

	DirectX::XMFLOAT4X4 mReallyCloseProj;
	DirectX::XMFLOAT4X4* pReallyCloseProj = nullptr;
	CReallyCloseData reallyCloseData;
	if (dwFlags & FLAG_REALLYCLOSE)
	{
		pReallyCloseProj = &mReallyCloseProj;
		d3d_SetReallyClose(&reallyCloseData, pReallyCloseProj, 
			g_CV_RCCanvasFOVOffset.m_Val, g_CV_RCCanvasFOVOffset.m_Val, false);

		dwRenderMode |= MODE_REALLY_CLOSE;
	}

	DrawPrimitiveActual(pVerts, dwVerts, pReallyCloseProj, dwRenderMode);

	if (pReallyCloseProj != nullptr)
		d3d_UnsetReallyClose(&reallyCloseData);
	
	g_RenderStateMgr.RestorePrimaryStates();

	return LT_OK;
}

void InternalCustomDraw::DrawPrimitiveActual(LTVertex* pVerts, uint32 dwVerts, DirectX::XMFLOAT4X4* pReallyCloseProj,
	uint32 dwRenderMode)
{
	CCanvasData* pData = g_GlobalMgr.GetCanvasMgr()->GetCanvasData(m_pTexture, pVerts, dwVerts);

	if (pData == nullptr)
		return;

	d3d_SetBuffersAndTopology(pData);

	CRenderShader_Canvas* pRenderShader = g_RenderShaderMgr.GetRenderShader<CRenderShader_Canvas>();

	DirectX::XMFLOAT3 vModeColor = { };
	if (m_pTexture != nullptr && m_pTexture->m_pStateChange != nullptr)
	{
		BlendState eBlendStateOverride = BLEND_STATE_Invalid;
		StencilState eStencilStateOverride = STENCIL_STATE_Invalid;

		g_RenderModeMgr.ApplyStateChange(m_pTexture->m_pStateChange, eBlendStateOverride, eStencilStateOverride, 
			dwRenderMode, &vModeColor);

		if (eBlendStateOverride != BLEND_STATE_Invalid)
			g_RenderStateMgr.SetBlendState(eBlendStateOverride);

		if (eStencilStateOverride != STENCIL_STATE_Invalid)
			g_RenderStateMgr.SetStencilState(eStencilStateOverride);
	}

	XMFloat4x4Trinity sTransforms;
	sTransforms.m_mWorld = g_mIdentity;
	sTransforms.m_mWorldView = sTransforms.m_mWorld;

	d3d_GetFinalCanvasTransform(&g_ViewParams, &sTransforms.m_mWorldViewProj, &sTransforms.m_mWorldView, pReallyCloseProj);

	if (!pRenderShader->SetPerObjectParams(m_pTexture != nullptr ? ((RTexture*)m_pTexture->m_pRenderData)->m_pResourceView : nullptr,
		dwRenderMode, &vModeColor, &sTransforms))
	{
		return;
	}

	pRenderShader->Render(dwVerts);
}

void InternalCustomDraw::TranslateStates(uint32& dwRenderMode, BlendState& eBlendState, StencilState& eStencilState)
{
	if (m_adwLTRStates[LTRSTATE_ALPHABLENDENABLE])
	{
		if (m_adwLTRStates[LTRSTATE_SRCBLEND] == LTBLEND_ONE && m_adwLTRStates[LTRSTATE_DESTBLEND] == LTBLEND_ONE)
		{
			dwRenderMode &= ~MODE_FOG_ENABLED;
			eBlendState = BLEND_STATE_Add;
		}
		else if (m_adwLTRStates[LTRSTATE_SRCBLEND] == LTBLEND_ZERO && m_adwLTRStates[LTRSTATE_DESTBLEND] == LTBLEND_SRCCOLOR)
		{
			dwRenderMode |= MODE_BLEND_MULTIPLY;
			eBlendState = BLEND_STATE_Multiply3D;
		}
		else if (m_adwLTRStates[LTRSTATE_SRCBLEND] == LTBLEND_SRCALPHA && m_adwLTRStates[LTRSTATE_DESTBLEND] == LTBLEND_INVSRCALPHA)
		{
			eBlendState = BLEND_STATE_Alpha;
		}
		else
		{
			NotImplementedMessage("CanvasDrawMgr::ValidateStates - SRCBLEND / DESTBLEND");
			
			m_adwLTRStates[LTRSTATE_SRCBLEND] = LTBLEND_SRCALPHA;
			m_adwLTRStates[LTRSTATE_DESTBLEND] = LTBLEND_INVSRCALPHA;
			eBlendState = BLEND_STATE_Alpha;
		}

		if (m_adwLTRStates[LTRSTATE_COLOROP] != LTOP_MODULATE)
		{
			m_adwLTRStates[LTRSTATE_COLOROP] = LTOP_MODULATE;
			NotImplementedMessage("CanvasDrawMgr::ValidateStates - LTRSTATE_COLOROP");
		}

		if (m_adwLTRStates[LTRSTATE_ALPHAOP] != LTOP_SELECTTEXTURE && m_adwLTRStates[LTRSTATE_ALPHAOP] != LTOP_SELECTDIFFUSE)
		{
			m_adwLTRStates[LTRSTATE_ALPHAOP] = LTOP_SELECTTEXTURE;
			NotImplementedMessage("CanvasDrawMgr::ValidateStates - ALPHAOP");		
		}
	}
	else
	{
		eBlendState = BLEND_STATE_Default;
	}
	
	if (!m_adwLTRStates[LTRSTATE_ZREADENABLE])
	{
		m_adwLTRStates[LTRSTATE_ZREADENABLE] = 1;

		NotImplementedMessage("CanvasDrawMgr::ValidateStates - ZREADENABLE");
	}

	eStencilState = m_adwLTRStates[LTRSTATE_ZWRITEENABLE] ? STENCIL_STATE_Default : STENCIL_STATE_NoZWrite;
}

#pragma warning(push)
#pragma warning(disable: 33010)
LTRESULT InternalCustomDraw::SetState(LTRState eState, uint32 dwVal)
{
	if (eState >= NUM_LTRSTATES)
		return LT_ERROR;

	m_adwLTRStates[eState] = dwVal;
	return LT_OK;
}

LTRESULT InternalCustomDraw::GetState(LTRState eState, uint32& dwVal)
{
	if (eState >= NUM_LTRSTATES)
		return LT_ERROR;

	dwVal = m_adwLTRStates[eState];
	return LT_OK;
}
#pragma warning(pop)

LTRESULT InternalCustomDraw::SetTexture(const char* szTexture)
{
	m_pTexture = nullptr;

	if (szTexture != nullptr)
	{
		m_pTexture = g_pStruct->GetSharedTexture(szTexture);
		
		if (m_pTexture == nullptr)
			return LT_ERROR;
	}

	return LT_OK;
}

LTRESULT InternalCustomDraw::GetTexelSize(float& fSizeU, float& fSizeV)
{
	if (m_pTexture == nullptr)
		return LT_ERROR;

	TextureData* pTextureData = g_pStruct->GetTexture(m_pTexture);

	if (pTextureData == nullptr)
		return LT_ERROR;

	fSizeU = 1.0f / (float)pTextureData->m_DtxHeader.m_wBaseWidth;
	fSizeV = 1.0f / (float)pTextureData->m_DtxHeader.m_wBaseHeight;

	return LT_OK;
}

void InternalCustomDraw::DrawCanvases(Canvas** ppList, uint32& dwListSize)
{
	for (uint32 i = 0; i < dwListSize; ++i) 
		DrawCanvas(ppList[i]);

	dwListSize = 0;
}

void InternalCustomDraw::DrawCanvas(Canvas* pCanvas)
{
	m_pCanvas = pCanvas;
	
	if (pCanvas->m_pFn != nullptr)
		pCanvas->m_pFn(this, &pCanvas->m_Base, pCanvas->m_pFnUserData);
}

static void d3d_SetBuffersAndTopology(CCanvasData* pData)
{
	g_RenderShaderMgr.SetVertexResource(VRS_Main, g_GlobalMgr.GetBankedVertexBuffer(pData->m_dwBankedVBIndex).m_pBuffer,
		sizeof(InternalCanvasVertex), 0);
	g_RenderShaderMgr.SetIndexBuffer16(nullptr, 0);
	g_RenderShaderMgr.SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
}

static void d3d_GetFinalCanvasTransform(ViewParams* pParams, DirectX::XMFLOAT4X4* pTransform, 
	DirectX::XMFLOAT4X4* pTransformNoProj, DirectX::XMFLOAT4X4* pReallyCloseProj)
{
	DirectX::XMMATRIX mTransform;
	
	if (pReallyCloseProj == nullptr)
	{
		*pTransformNoProj = pParams->m_mViewTransposed;

		mTransform = DirectX::XMLoadFloat4x4(&pParams->m_mViewTransposed) *
			DirectX::XMLoadFloat4x4(&pParams->m_mProjectionTransposed);
	}
	else
	{
		mTransform = DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(pReallyCloseProj));
	}

	DirectX::XMStoreFloat4x4(pTransform, mTransform);
}

static void d3d_GetFinalCanvasTransform_Normal(ViewParams* pParams, DirectX::XMFLOAT4X4* pTransform,
	DirectX::XMFLOAT4X4* pTransformNoProj)
{
	*pTransformNoProj = pParams->m_mViewTransposed;

	DirectX::XMStoreFloat4x4(pTransform, DirectX::XMLoadFloat4x4(&pParams->m_mViewTransposed) * 
		DirectX::XMLoadFloat4x4(&pParams->m_mProjectionTransposed));
}

static void d3d_GetFinalCanvasTransform_ReallyClose(DirectX::XMFLOAT4X4* pTransform, DirectX::XMFLOAT4X4* pReallyCloseProj)
{
	DirectX::XMStoreFloat4x4(pTransform, DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(pReallyCloseProj)));
}

static void d3d_DrawCanvas(ViewParams* pParams, LTObject* pCanvas)
{
	g_InternalCustomDraw.SetExtendedDraw(false);
	g_InternalCustomDraw.DrawCanvas(pCanvas->ToCanvas());
}

static void d3d_DrawCanvasEx(ViewParams* pParams, LTObject* pCanvas, LTObject* pPrevObject)
{
	g_InternalCustomDraw.SetExtendedDraw(true);
	g_InternalCustomDraw.DrawCanvas(pCanvas->ToCanvas());
}

void d3d_ProcessCanvas(LTObject* pObject)
{
	if (!g_CV_DrawCanvases.m_Val)
		return;

	if (pObject->m_ClientData.m_nClientFlags & CF_SOLIDCANVAS)
		d3d_GetVisibleSet()->GetCanvases()->Add(pObject);
	else
		d3d_GetVisibleSet()->GetTranslucentCanvases()->Add(pObject);
}

void d3d_DrawSolidCanvases(ViewParams* pParams)
{
	d3d_GetVisibleSet()->GetCanvases()->Draw(pParams, d3d_DrawCanvas);
}

void d3d_QueueTranslucentCanvases(ViewParams* pParams, ObjectDrawList* pDrawList)
{
	d3d_GetVisibleSet()->GetTranslucentCanvases()->Queue(pDrawList, pParams, d3d_DrawCanvas, d3d_DrawCanvasEx);
}
