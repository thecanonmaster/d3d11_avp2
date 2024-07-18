#include "pch.h"

#include "draw_linesystem.h"
#include "rendererconsolevars.h"
#include "tagnodes.h"
#include "common_stuff.h"
#include "d3d_mathhelpers.h"
#include "d3d_device.h"
#include "d3d_draw.h"
#include "3d_ops.h"
#include "globalmgr.h"
#include "d3d_shader_linesystem.h"
#include "common_draw.h"
#include "rendermodemgr.h"

void d3d_SetBuffersAndTopology(CLineSystemData* pData);
uint32 GetLineSystemVertexCount(LineSystem* pLineSystem);

void CLineSystemData::Term()
{
	delete[] m_pData;

	if (m_dwBankedVBIndex != UINT32_MAX)
		g_GlobalMgr.GetBankedVertexBuffer(m_dwBankedVBIndex).m_bInUse = false;
}

CExpirableData::CompareResult CLineSystemData::CompareTo(LineSystem* pLineSystem, uint32 dwVerts)
{
	if (dwVerts > m_dwVerts)
		return COMP_R_SizeDiffMore;
	else if (dwVerts < m_dwVerts)
		return COMP_R_SizeDiff;

	int i = 0;
	LTLine* pLine = pLineSystem->m_LineHead.m_pNext;
	while (pLine != &pLineSystem->m_LineHead)
	{
#ifndef LINE_SYSTEM_DATA_MEM_CMP
		if (!m_pData[i].CompareTo(&pLine->m_Points[0]))
#else
		if (memcmp(&m_pData[i], &pLine->m_Points[0], sizeof(LTLinePt)))
#endif
			return COMP_R_ContentDiff;

		i++;

#ifndef LINE_SYSTEM_DATA_MEM_CMP
		if (!m_pData[i].CompareTo(&pLine->m_Points[1]))
#else
		if (memcmp(&m_pData[i], &pLine->m_Points[1], sizeof(LTLinePt)))
#endif
			return COMP_R_ContentDiff;

		i++;
		pLine = pLine->m_pNext;
	}

	return COMP_R_Equal;
}

bool CLineSystemData::CreateVertexBuffer()
{
	InternalLineVertex* pTempBuffer =m_dwVerts > LINE_SYSTEM_TEMP_BUFFER_LEN ? 
		new InternalLineVertex[m_dwVerts] : 
		g_GlobalMgr.GetLineSystemMgr()->GetTempBuffer();

	for (uint32 i = 0; i < m_dwVerts; i++)
	{
		pTempBuffer[i].vPosition = *PLTVECTOR_TO_PXMFLOAT3(&m_pData[i].m_vPos);

		pTempBuffer[i].vDiffuseColor.x = m_pData[i].m_fR;
		pTempBuffer[i].vDiffuseColor.y = m_pData[i].m_fG;
		pTempBuffer[i].vDiffuseColor.z = m_pData[i].m_fB;
		pTempBuffer[i].vDiffuseColor.w = m_pData[i].m_fA;
	}

	m_dwBankedVBIndex = g_GlobalMgr.AssignBankedVertexBuffer(pTempBuffer, sizeof(InternalLineVertex) * m_dwVerts);

	if (m_dwVerts > LINE_SYSTEM_TEMP_BUFFER_LEN)
		delete [] pTempBuffer;

	return (m_dwBankedVBIndex != UINT32_MAX);
}

bool CLineSystemData::UpdateVertexBuffer(LineSystem* pLineSystem, uint32 dwVerts)
{
	CompareResult eResult = CompareTo(pLineSystem, dwVerts);

	auto lambdaUpdate = [dwVerts, this](D3D11_MAPPED_SUBRESOURCE* pSubResource)
		{
			InternalLineVertex* pVertices = (InternalLineVertex*)pSubResource->pData;

			for (uint32 i = 0; i < dwVerts; i++)
			{
				pVertices[i].vPosition = *PLTVECTOR_TO_PXMFLOAT3(&m_pData[i].m_vPos);

				pVertices[i].vDiffuseColor.x = m_pData[i].m_fR;
				pVertices[i].vDiffuseColor.y = m_pData[i].m_fG;
				pVertices[i].vDiffuseColor.z = m_pData[i].m_fB;
				pVertices[i].vDiffuseColor.w = m_pData[i].m_fA;
			}
		};

	if (eResult == COMP_R_ContentDiff)
	{
		InitData(pLineSystem);
		if (!g_GlobalMgr.UpdateGenericBufferEx(g_GlobalMgr.GetBankedVertexBuffer(m_dwBankedVBIndex).m_pBuffer,
			D3D11_MAP_WRITE_DISCARD, lambdaUpdate))
		{
			return false;
		}
	}
	else if (eResult == COMP_R_SizeDiff)
	{
		m_dwVerts = dwVerts;
		InitData(pLineSystem);
		if (!g_GlobalMgr.UpdateGenericBufferEx(g_GlobalMgr.GetBankedVertexBuffer(m_dwBankedVBIndex).m_pBuffer,
			D3D11_MAP_WRITE_DISCARD, lambdaUpdate))
		{
			return false;
		}
	}
	else if (eResult == COMP_R_SizeDiffMore)
	{
		Term();

		m_pData = new LTLinePt[dwVerts];
		m_dwVerts = dwVerts;
		InitData(pLineSystem);

		if (!CreateVertexBuffer())
			return false;
	}

	return true;
}

void CLineSystemData::InitData(LineSystem* pLineSystem)
{
	int i = 0;
	LTLine* pLine = pLineSystem->m_LineHead.m_pNext;
	while (pLine != &pLineSystem->m_LineHead)
	{
		m_pData[i] = pLine->m_Points[0];
		i++;

		m_pData[i] = pLine->m_Points[1];
		i++;
		pLine = pLine->m_pNext;
	}
}

CLineSystemData* CLineSystemManager::GetLineSystemData(LineSystem* pLineSystem)
{
	uint32 dwVerts = GetLineSystemVertexCount(pLineSystem);
	if (!dwVerts)
		return nullptr;

	auto iter = m_Data.find(pLineSystem);

	if (iter != m_Data.end())
	{
		CLineSystemData* pData = (*iter).second;

		pData->UpdateVertexBuffer(pLineSystem, dwVerts);
		pData->SetLastUpdate(g_fLastClientTime);

		return pData;
	}

	CLineSystemData* pNewData = new CLineSystemData(pLineSystem, dwVerts);

	if (!pNewData->CreateVertexBuffer())
	{
		delete pNewData;
		return nullptr;
	}

	pNewData->SetLastUpdate(g_fLastClientTime);
	m_Data[pLineSystem] = pNewData;

	return pNewData;
}

static void d3d_GetFinalLineSystemTransform(ViewParams* pParams, DirectX::XMFLOAT4X4* pTransform, 
	DirectX::XMFLOAT4X4* pTransformNoProj, DirectX::XMFLOAT4X4* pSystemTransform)
{
	DirectX::XMMATRIX mTransform;
	
	if (pSystemTransform != nullptr)
	{
		mTransform = DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(pSystemTransform)) *
			DirectX::XMLoadFloat4x4(&pParams->m_mViewTransposed);

		DirectX::XMStoreFloat4x4(pTransformNoProj, mTransform);

		mTransform *= DirectX::XMLoadFloat4x4(&pParams->m_mProjectionTransposed);
	}
	else
	{
		*pTransformNoProj = pParams->m_mViewTransposed;

		mTransform = DirectX::XMLoadFloat4x4(&pParams->m_mViewTransposed) *
			DirectX::XMLoadFloat4x4(&pParams->m_mProjectionTransposed);
	}

	DirectX::XMStoreFloat4x4(pTransform, mTransform);
}

static void d3d_DrawLineSystem(ViewParams* pParams, LTObject* pObject)
{
	LineSystem* pSystem = pObject->ToLineSystem();

	CLineSystemData* pLineSystemData = g_GlobalMgr.GetLineSystemMgr()->GetLineSystemData(pSystem);

	if (pLineSystemData == nullptr)
		return;

	uint32 dwRenderMode = 0;
	g_RenderModeMgr.SetupRenderMode_LineSystem(pObject->m_dwFlags, dwRenderMode);

	d3d_SetBuffersAndTopology(pLineSystemData);

	DirectX::XMFLOAT4X4 mSystemTransform;
	d3d_SetupTransformation(&pObject->m_vPos, (float*)&pObject->m_rRot, &pObject->m_vScale, &mSystemTransform);

	CRenderShader_LineSystem* pRenderShader = g_RenderShaderMgr.GetRenderShader<CRenderShader_LineSystem>();

	XMFloat4x4Trinity sTransforms;
	sTransforms.m_mWorld = mSystemTransform;

	d3d_GetFinalLineSystemTransform(pParams, &sTransforms.m_mWorldViewProj, &sTransforms.m_mWorldView, 
		&mSystemTransform);

	if (!pRenderShader->SetPerObjectParams(dwRenderMode, &sTransforms, (float)pObject->m_nColorA * MATH_ONE_OVER_255))
		return;

	pRenderShader->Render(pLineSystemData->GetVertexCount());
}

static void d3d_DrawLineSystemEx(ViewParams* pParams, LTObject* pObject, LTObject* pPrevObject)
{
	g_GlobalMgr.GetSpriteBatchMgr()->RenderBatch();
	g_GlobalMgr.GetCanvasBatchMgr()->RenderBatch();

	d3d_DrawLineSystem(pParams, pObject);
}

static void d3d_SetBuffersAndTopology(CLineSystemData* pData)
{
	g_RenderShaderMgr.SetVertexResource(VRS_Main, g_GlobalMgr.GetBankedVertexBuffer(pData->m_dwBankedVBIndex).m_pBuffer,
		sizeof(VertexTypes::PositionColor), 0);
	g_RenderShaderMgr.SetIndexBuffer16(nullptr, 0);
	g_RenderShaderMgr.SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
}

void d3d_ProcessLineSystem(LTObject* pObject)
{
	if (!g_CV_DrawLineSystems.m_Val)
		return;

	d3d_GetVisibleSet()->GetLineSystems()->Add(pObject);
}

void d3d_QueueLineSystems(ViewParams* pParams, ObjectDrawList* pDrawList)
{
	d3d_GetVisibleSet()->GetLineSystems()->Queue(pDrawList, pParams, d3d_DrawLineSystem, d3d_DrawLineSystemEx);
}

void d3d_DrawLine(DirectX::XMFLOAT3* pSrc, DirectX::XMFLOAT3* pDest, DirectX::XMFLOAT4* pColor1, DirectX::XMFLOAT4* pColor2)
{
	auto lambdaUpdate = [pSrc, pColor1, pDest, pColor2](D3D11_MAPPED_SUBRESOURCE* pSubResource)
		{
			InternalLineVertex* pVertices = (InternalLineVertex*)pSubResource->pData;

			pVertices[0].vPosition = *pSrc;
			pVertices[0].vDiffuseColor = *pColor1;
			pVertices[1].vPosition = *pDest;
			pVertices[1].vDiffuseColor = *pColor2;
		};

	if (!g_GlobalMgr.UpdateGenericBufferEx(g_GlobalMgr.GetVertexBuffer_Line(), D3D11_MAP_WRITE_DISCARD, lambdaUpdate))
		return;

	g_RenderShaderMgr.SetVertexResource(VRS_Main, g_GlobalMgr.GetVertexBuffer_Line(), sizeof(VertexTypes::PositionColor), 0);
	g_RenderShaderMgr.SetIndexBuffer16(nullptr, 0);
	g_RenderShaderMgr.SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	CRenderShader_LineSystem* pRenderShader = g_RenderShaderMgr.GetRenderShader<CRenderShader_LineSystem>();

	XMFloat4x4Trinity sTransforms;
	DirectX::XMStoreFloat4x4(&sTransforms.m_mWorld, DirectX::XMMatrixIdentity());

	d3d_GetFinalLineSystemTransform(&g_ViewParams, &sTransforms.m_mWorldViewProj, &sTransforms.m_mWorldView, nullptr);

	if (!pRenderShader->SetPerObjectParams(0, &sTransforms, 1.0f))
		return;

	pRenderShader->Render(LINE_VERTICES);
}

void d3d_DrawLine(DirectX::XMFLOAT3* pSrc, DirectX::XMFLOAT3* pDest, DirectX::XMFLOAT4* pColor)
{
	d3d_DrawLine(pSrc, pDest, pColor, pColor);
}

static uint32 GetLineSystemVertexCount(LineSystem* pLineSystem)
{
	uint32 i = 0;

	LTLine* pLine = pLineSystem->m_LineHead.m_pNext;
	while (pLine != &pLineSystem->m_LineHead)
	{
		i += LINE_VERTICES;
		pLine = pLine->m_pNext;
	}

	return i;
}
