#include "pch.h"

#include "draw_particles.h"
#include "rendererconsolevars.h"
#include "tagnodes.h"
#include "renderstatemgr.h"
#include "d3d_draw.h"
#include "d3d_viewparams.h"
#include "3d_ops.h"
#include "globalmgr.h"
#include "common_stuff.h"
#include "d3d_shader_particles.h"
#include "common_draw.h"
#include "rendermodemgr.h"

using namespace DirectX;

void CParticleSystemData::Term_VertexRelated()
{
	if (m_dwBankedVBIndex != UINT32_MAX)
		g_GlobalMgr.GetBankedVertexBuffer(m_dwBankedVBIndex).m_bInUse = false;
}

void CParticleSystemData::Term_InstanceRelated()
{
	delete[] m_pInstanceData;

	if (m_dwBankedISIndex != UINT32_MAX)
		g_GlobalMgr.GetBankedVertexBuffer(m_dwBankedISIndex).m_bInUse = false;
}

CExpirableData::CompareResult CParticleSystemData::CompareTo_InstanceData(ParticleSystem* pParticleSystem)
{
	if (pParticleSystem->m_nParticles > m_nInstances)
		return COMP_R_SizeDiffMore;
	else if (pParticleSystem->m_nParticles < m_nInstances)
		return COMP_R_SizeDiff;

	LTParticle* pParticle = pParticleSystem->m_ParticleHead.m_pNext;

	for (int i = 0; i < m_nInstances; i++)
	{
		InternalParticleInstance instanceItem;

		instanceItem.vPosition =
		{
			pParticle->m_vPos.x, pParticle->m_vPos.y, pParticle->m_vPos.z
		};

		instanceItem.vDiffuseColor =
		{
			pParticle->m_vColor.x * MATH_ONE_OVER_255,
			pParticle->m_vColor.y * MATH_ONE_OVER_255,
			pParticle->m_vColor.z * MATH_ONE_OVER_255,
			pParticle->m_fAlpha
		};

		instanceItem.fScale = pParticle->m_fSize;

#ifndef PARTICLE_SYSTEM_DATA_MEM_CMP
		if (!m_pInstanceData[i].CompareTo(&instanceItem))
#else
		if (memcmp(m_pInstanceData[i], instanceItem, sizeof(InternalParticleInstance) * m_nInstances))
#endif
			return COMP_R_ContentDiff;

		pParticle = pParticle->m_pNext;
	}

	return COMP_R_Equal;
}

bool CParticleSystemData::CreateVertexBuffer()
{
	m_dwBankedVBIndex = g_GlobalMgr.AssignBankedVertexBuffer(m_aVertexData, sizeof(m_aVertexData));
	return (m_dwBankedVBIndex != UINT32_MAX);
}

bool CParticleSystemData::CreateInstanceBuffer()
{
	m_dwBankedISIndex = g_GlobalMgr.AssignBankedVertexBuffer(m_pInstanceData, sizeof(InternalParticleInstance) * m_nInstances);
	return (m_dwBankedISIndex != UINT32_MAX);
}

bool CParticleSystemData::UpdateBuffers(ParticleSystem* pParticleSystem, InternalParticleVertex* pVerts)
{
	bool bVertexResult = true;
	bool bInstanceResult = true;

	if (!CompareTo_VertexData(pVerts))
		bVertexResult = UpdateVertexBuffer(pParticleSystem, pVerts);
	
	CompareResult eResult = CompareTo_InstanceData(pParticleSystem);
	bInstanceResult = UpdateInstanceBuffer(pParticleSystem, eResult);

	return bVertexResult && bInstanceResult;
}

bool CParticleSystemData::UpdateVertexBuffer(ParticleSystem* pParticleSystem, InternalParticleVertex* pVerts)
{
	InitVertexData(pParticleSystem, pVerts);

	return g_GlobalMgr.UpdateGenericBuffer(g_GlobalMgr.GetBankedVertexBuffer(m_dwBankedVBIndex).m_pBuffer,
		m_aVertexData, sizeof(m_aVertexData), D3D11_MAP_WRITE_DISCARD);
}

bool CParticleSystemData::UpdateInstanceBuffer(ParticleSystem* pParticleSystem, CompareResult eResult)
{
	if (eResult == COMP_R_ContentDiff)
	{
		InitInstanceData(pParticleSystem);

		if (!g_GlobalMgr.UpdateGenericBuffer(g_GlobalMgr.GetBankedVertexBuffer(m_dwBankedISIndex).m_pBuffer, 
			m_pInstanceData, sizeof(InternalParticleInstance) * pParticleSystem->m_nParticles, 
			D3D11_MAP_WRITE_DISCARD))
		{
			return false;
		}
	}
	else if (eResult == COMP_R_SizeDiff)
	{
		m_nInstances = pParticleSystem->m_nParticles;
		InitInstanceData(pParticleSystem);

		if (!g_GlobalMgr.UpdateGenericBuffer(g_GlobalMgr.GetBankedVertexBuffer(m_dwBankedISIndex).m_pBuffer, 
			m_pInstanceData, sizeof(InternalParticleInstance) * pParticleSystem->m_nParticles, 
			D3D11_MAP_WRITE_DISCARD))
		{
			return false;
		}
	}
	else if (eResult == COMP_R_SizeDiffMore)
	{
		Term_InstanceRelated();

		m_nInstances = pParticleSystem->m_nParticles;
		m_pInstanceData = new InternalParticleInstance[m_nInstances];
		InitInstanceData(pParticleSystem);

		if (!CreateInstanceBuffer())
			return false;
	}
	
	return true;
}

void CParticleSystemData::InitVertexData(ParticleSystem* pParticleSystem, InternalParticleVertex* pVerts)
{
	memcpy(m_aVertexData, pVerts, sizeof(InternalParticleVertex) * PARTICLE_VERTICES);
}

void CParticleSystemData::InitInstanceData(ParticleSystem* pParticleSystem)
{
	LTParticle* pParticle = pParticleSystem->m_ParticleHead.m_pNext;

	for (int i = 0; i < m_nInstances; i++)
	{
		m_pInstanceData[i].vPosition = 
		{ 
			pParticle->m_vPos.x, pParticle->m_vPos.y, pParticle->m_vPos.z
		};

		m_pInstanceData[i].vDiffuseColor =
		{
			pParticle->m_vColor.x * MATH_ONE_OVER_255,
			pParticle->m_vColor.y * MATH_ONE_OVER_255,
			pParticle->m_vColor.z * MATH_ONE_OVER_255,
			pParticle->m_fAlpha
		};

		m_pInstanceData[i].fScale = pParticle->m_fSize;

		pParticle = pParticle->m_pNext;
	}
}

CParticleSystemData* CParticleSystemManager::GetParticleSystemData(ParticleSystem* pParticleSystem, InternalParticleVertex* pVerts)
{
	auto iter = m_Data.find(pParticleSystem->m_pSprite);

	if (iter != m_Data.end())
	{
		CParticleSystemData* pData = (*iter).second;

		pData->UpdateBuffers(pParticleSystem, pVerts);
		pData->SetLastUpdate(g_fLastClientTime);

		return pData;
	}

	CParticleSystemData* pNewData = new CParticleSystemData(pParticleSystem, pVerts);

	if (!pNewData->CreateVertexBuffer() || !pNewData->CreateInstanceBuffer())
	{
		delete pNewData;
		return nullptr;
	}

	pNewData->SetLastUpdate(g_fLastClientTime);
	m_Data[pParticleSystem->m_pSprite] = pNewData;

	return pNewData;
}

static void d3d_GetFinalParticleSystemTransform(ViewParams* pParams, DirectX::XMFLOAT4X4* pTransform, 
	DirectX::XMFLOAT4X4* pTransformNoProj, DirectX::XMFLOAT4X4* pSystemTransform, DirectX::XMFLOAT4X4* pReallyCloseProj)
{
	DirectX::XMMATRIX mTransform;

	if (pReallyCloseProj == nullptr)
	{
		mTransform = DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(pSystemTransform)) *
			DirectX::XMLoadFloat4x4(&pParams->m_mViewTransposed);

		DirectX::XMStoreFloat4x4(pTransformNoProj, mTransform);
		
		mTransform *= DirectX::XMLoadFloat4x4(&pParams->m_mProjectionTransposed);
	}
	else
	{
		mTransform = DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(pSystemTransform));

		DirectX::XMStoreFloat4x4(pTransformNoProj, mTransform);

		mTransform *= DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(pReallyCloseProj));
	}

	DirectX::XMStoreFloat4x4(pTransform, mTransform);
}

static void d3d_SetBuffersAndTopology(CParticleSystemData* pData)
{
	ID3D11Buffer* apBufferPointers[2] = 
	{ 
		g_GlobalMgr.GetBankedVertexBuffer(pData->m_dwBankedVBIndex).m_pBuffer,
		g_GlobalMgr.GetBankedVertexBuffer(pData->m_dwBankedISIndex).m_pBuffer 
	};

	uint32 adwStrides[2] = { sizeof(InternalParticleVertex), sizeof(InternalParticleInstance) };
	uint32 adwOffsets[2] = { 0, 0 };

	g_RenderShaderMgr.SetVertexResources(VRS_Main, 2, apBufferPointers, adwStrides, adwOffsets);
	g_RenderShaderMgr.SetIndexBuffer16(g_GlobalMgr.GetIndexBuffer_Sprite(), 0);
	g_RenderShaderMgr.SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

static void d3d_DrawParticles(ViewParams* pParams, ParticleSystem* pSystem, DirectX::XMFLOAT3* pParticleUp, 
	DirectX::XMFLOAT3* pParticleRight, DirectX::XMFLOAT4X4* pSystemTransform, DirectX::XMFLOAT4X4* pReallyCloseProj, 
	uint32 dwRenderMode)
{
	// TODO - not needed if texture coords are always 0.0 - 1.0
	float fWidth = (float)((RTexture*)pSystem->m_pCurTexture->m_pRenderData)->m_dwScaledWidth;
	float fHeight = (float)((RTexture*)pSystem->m_pCurTexture->m_pRenderData)->m_dwScaledHeight;

	float fSizeU = 1.0f / fWidth;
	float fSizeV = 1.0f / fHeight;
	float fMinU = fSizeU + fSizeU;
	float fMinV = fSizeV + fSizeV;
	float fMaxU = (fWidth - 2.0f) * fSizeU;
	float fMaxV = (fHeight - 2.0f) * fSizeV;

	InternalParticleVertex aParticleVerts[PARTICLE_VERTICES];
	aParticleVerts[0].vTexCoords = { fMinU, fMinV };
	aParticleVerts[1].vTexCoords = { fMaxU, fMinV };
	aParticleVerts[2].vTexCoords = { fMaxU, fMaxV };
	aParticleVerts[3].vTexCoords = { fMinU, fMaxV };

	CParticleSystemData* pParticleSystemData = g_GlobalMgr.GetParticleSystemMgr()->GetParticleSystemData(pSystem, aParticleVerts);

	if (pParticleSystemData == nullptr)
		return;

	d3d_SetBuffersAndTopology(pParticleSystemData);

	CRenderShader_ParticleSystem* pRenderShader = g_RenderShaderMgr.GetRenderShader<CRenderShader_ParticleSystem>();

	DirectX::XMFLOAT3 vModeColor = { };
	if (pSystem->m_pCurTexture->m_pStateChange != nullptr)
	{
		BlendState eBlendStateOverride = BLEND_STATE_Invalid;
		StencilState eStencilStateOverride = STENCIL_STATE_Invalid;

		g_RenderModeMgr.ApplyStateChange(pSystem->m_pCurTexture->m_pStateChange, eBlendStateOverride, eStencilStateOverride,
			dwRenderMode, &vModeColor);

		if (eBlendStateOverride != BLEND_STATE_Invalid)
			g_RenderStateMgr.SetBlendState(eBlendStateOverride);

		if (eStencilStateOverride != STENCIL_STATE_Invalid)
			g_RenderStateMgr.SetStencilState(eStencilStateOverride);
	}

	DirectX::XMFLOAT4 vColorScale
	{
		(float)pSystem->m_Base.m_nColorR * MATH_ONE_OVER_255,
		(float)pSystem->m_Base.m_nColorG * MATH_ONE_OVER_255,
		(float)pSystem->m_Base.m_nColorB * MATH_ONE_OVER_255,
		(float)pSystem->m_Base.m_nColorA * MATH_ONE_OVER_255,
	};

	XMFloat4x4Trinity sTransforms;
	sTransforms.m_mWorld = *pSystemTransform;

	d3d_GetFinalParticleSystemTransform(pParams, &sTransforms.m_mWorldViewProj, &sTransforms.m_mWorldView, 
		pSystemTransform, pReallyCloseProj);
	
	if (!pRenderShader->SetPerObjectParams(((RTexture*)pSystem->m_pCurTexture->m_pRenderData)->m_pResourceView, 
		dwRenderMode, &vModeColor, &vColorScale, pParticleUp, pParticleRight, &sTransforms))
	{
		return;
	}

	pRenderShader->Render(PARTICLE_INDICES, pParticleSystemData->GetInstanceCount());
}

void d3d_DrawParticleSystem(ViewParams* pParams, ParticleSystem* pParticleSystem, uint32 dwRenderMode)
{
	if (pParticleSystem->m_pCurTexture == nullptr)
		return;

	DirectX::XMFLOAT3 vParticleUp;
	DirectX::XMFLOAT3 vParticleRight;

	if (pParticleSystem->m_Base.m_dwFlags & FLAG_REALLYCLOSE)
	{
		vParticleUp = { 0.0f, 1.0f, 0.0f };
		vParticleRight = { 1.0f, 0.0f, 0.0f };
	}
	else
	{
		vParticleUp = pParams->m_vUp;
		vParticleRight = pParams->m_vRight;
	}

	// TODO - always in object space? transpose?
	DirectX::XMFLOAT4X4 mSystemTransform;
	d3d_SetupTransformation(&pParticleSystem->m_Base.m_vPos, (float*)&pParticleSystem->m_Base.m_rRot, &pParticleSystem->m_Base.m_vScale,
		&mSystemTransform);

	DirectX::XMMATRIX mInverse = DirectX::XMMatrixInverse(nullptr, DirectX::XMLoadFloat4x4(&mSystemTransform));

	DirectX::XMFLOAT4X4 mInverseTemp;
	DirectX::XMStoreFloat4x4(&mInverseTemp, mInverse);

	// TODO - replace with DirectXMath?
	Matrix_Apply3x3LT(&mInverseTemp, &vParticleUp);
	Matrix_Apply3x3LT(&mInverseTemp, &vParticleRight);

	Vector_Mult(&vParticleUp, 2.0f);
	Vector_Mult(&vParticleRight, 2.0f);

	/*g_vOffset[PSOC_UPPER_LEFT] = vParticleUp - vParticleRight;
	g_vOffset[PSOC_UPPER_RIGHT] = vParticleUp + vParticleRight;
	g_vOffset[PSOC_BOTTOM_LEFT] = -vParticleUp - vParticleRight;
	g_vOffset[PSOC_BOTTOM_RIGHT] = -vParticleUp + vParticleRight;*/

	DirectX::XMFLOAT4X4 mReallyCloseProj;
	DirectX::XMFLOAT4X4* pReallyCloseProj = nullptr;
	CReallyCloseData reallyCloseData;
	if (pParticleSystem->m_Base.m_dwFlags & FLAG_REALLYCLOSE)
	{
		pReallyCloseProj = &mReallyCloseProj;
		d3d_SetReallyClose(&reallyCloseData, pReallyCloseProj, 
			g_CV_RCParticlesFOVOffset.m_Val, g_CV_RCParticlesFOVOffset.m_Val, false);
	}

	d3d_DrawParticles(pParams, pParticleSystem, &vParticleUp, &vParticleRight, &mSystemTransform, 
		pReallyCloseProj, dwRenderMode);

	if (pReallyCloseProj != nullptr)
		d3d_UnsetReallyClose(&reallyCloseData);
}

void d3d_ProcessParticles(LTObject* pObject)
{
	if (g_CV_DrawParticles)
		d3d_GetVisibleSet()->GetParticleSystems()->Add(pObject);
}

void d3d_TestAndDrawParticleSystem(ViewParams* pParams, LTObject* pObj)
{
	uint32 dwRenderMode = 0;
	BlendState eBlendState;
	ParticleSystem* pSystem = pObj->ToParticleSystem();

	if (!pSystem->m_nParticles)
		return;
	
	float fRadius = pSystem->m_fSystemRadius * 
		LTMAX(pSystem->m_Base.m_vScale.x, LTMAX(pSystem->m_Base.m_vScale.y, pSystem->m_Base.m_vScale.z));

	pSystem->m_Base.m_dwFlags |= FLAG_INTERNAL1;

	g_RenderStateMgr.SavePrimaryStates();

	g_RenderModeMgr.SetupRenderMode_ParticleSystem(pSystem->m_pCurTexture, pObj->m_dwFlags, pObj->m_dwFlags2, 
		eBlendState, dwRenderMode, BLEND_STATE_Alpha);

	g_RenderStateMgr.SetBlendState(eBlendState);
	g_RenderStateMgr.SetRasterState(!g_CV_Wireframe.m_Val ? RASTER_STATE_Default : RASTER_STATE_Wireframe);

	d3d_DrawParticleSystem(pParams, pSystem, dwRenderMode);

	g_RenderStateMgr.RestorePrimaryStates();
}

void d3d_TestAndDrawParticleSystemEx(ViewParams* pParams, LTObject* pObj, LTObject* pPrevObject)
{
	g_GlobalMgr.GetSpriteBatchMgr()->RenderBatch();
	g_GlobalMgr.GetCanvasBatchMgr()->RenderBatch();

	d3d_TestAndDrawParticleSystem(pParams, pObj);
}

void d3d_QueueTranslucentParticles(ViewParams* pParams, ObjectDrawList* pDrawList)
{
	d3d_GetVisibleSet()->GetParticleSystems()->Queue(pDrawList, pParams, d3d_TestAndDrawParticleSystem, 
		d3d_TestAndDrawParticleSystemEx);
}
