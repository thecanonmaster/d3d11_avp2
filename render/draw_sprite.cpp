#include "pch.h"

#include "draw_sprite.h"
#include "rendererconsolevars.h"
#include "tagnodes.h"
#include "draw_objects.h"
#include "d3d_draw.h"
#include "renderstatemgr.h"
#include "3d_ops.h"
#include "d3d_viewparams.h"
#include "d3d_mathhelpers.h"
#include "d3d_texture.h"
#include "common_draw.h"
#include "common_stuff.h"
#include "d3d_vertextypes.h"
#include "d3d_device.h"
#include "globalmgr.h"
#include "d3d_shader_sprite.h"
#include "d3d_utils.h"
#include "rendermodemgr.h"
#include "draw_light.h"

using namespace DirectX;

#define SPRITE_POSITION_ZBIAS_RS	-20.0f
#define SPRITE_SCALE_ZBIAS_RS		0.227f
#define SPRITE_POSITION_ZBIAS		-20.0f

#define SPRITE_MINFACTORDIST	10.0f
#define SPRITE_MAXFACTORDIST	500.0f
#define SPRITE_MINFACTOR		0.1f
#define SPRITE_MAXFACTOR		2.0f

static void d3d_GetFinalSpriteTransform_Normal(ViewParams* pParams, DirectX::XMFLOAT4X4* pTransform, DirectX::XMFLOAT4X4* pTransformNoProj);
static void d3d_GetFinalSpriteTransform_ReallyClose(DirectX::XMFLOAT4X4* pTransform, DirectX::XMFLOAT4X4* pReallyCloseProj);

template<typename T>
static void XM_CALLCONV d3d_BiasSprite_Jupiter(T* pPoints, float fZ, DirectX::FXMVECTOR vBasisPos,
	DirectX::FXMVECTOR vBasisRight, DirectX::FXMVECTOR vBasisUp, DirectX::GXMVECTOR vBasisForward);

template<typename T>
static void XM_CALLCONV d3d_BiasSprite_Simple(T* pPoints, float fZ, DirectX::FXMVECTOR vBasisPos);

template<typename T>
static void d3d_BiasRotatableSprite_Jupiter(ViewParams* pParams, T* pPoints, uint32 dwPoints, float fBiasDistIn);

template<typename T>
static void d3d_BiasRotatableSprite_Simple(ViewParams* pParams, T* pPoints, uint32 dwPoints, float fBiasDistIn);

static void d3d_GetSpriteColor(LTObject* pObject, DirectX::XMFLOAT4* pColor);

#define PLANETEST(pt) (Plane_DistTo(&plane, &pt) > 0.0f)
#define DOPLANECLIP(pt1, pt2) \
	fD1 = Plane_DistTo(&plane, &pt1); \
	fD2 = Plane_DistTo(&plane, &pt2); \
	t = -fD1 / (fD2 - fD1); \
	pOut->vPosition.x = pt1.x + ((pt2.x - pt1.x) * t);\
	pOut->vPosition.y = pt1.y + ((pt2.y - pt1.y) * t);\
	pOut->vPosition.z = pt1.z + ((pt2.z - pt1.z) * t);

template<class T>
inline bool d3d_ClipSprite(SpriteInstance* pInstance, HPOLY hPoly, T** ppPoints, uint32* pdwPoints, T* pOut)
{
	if (!g_bHaveWorld)
		return false;

	WorldPoly* pPoly = g_pSceneDesc->m_pRenderContext->m_pMainWorld->GetPolyFromHPoly(hPoly);
	if (pPoly == nullptr)
		return false;

	float fDot = Plane_DistTo(PLTPLANE_TO_PXMPLANE(pPoly->m_pPlane), &g_ViewParams.m_vPos); // TODO - no clipping in sky
	if (fDot <= 0.01f)
		return false;

	T* pVerts = *ppPoints;
	uint32 dwVerts = *pdwPoints;

	XMPLANE plane;
	float fD1, fD2;

	SPolyVertex* pEndPoint = &pPoly->m_Vertices[pPoly->m_wNumVerts];
	SPolyVertex* pPrevPoint = pEndPoint - 1; 

	for (SPolyVertex* pCurPoint = pPoly->m_Vertices; pCurPoint != pEndPoint; )
	{
		DirectX::XMVECTOR vPrevPoint = 
			DirectX::XMLoadFloat3(PLTVECTOR_TO_PXMFLOAT3(&pPrevPoint->m_pPoints->m_vVec));
		DirectX::XMVECTOR vCurPoint =
			DirectX::XMLoadFloat3(PLTVECTOR_TO_PXMFLOAT3(&pCurPoint->m_pPoints->m_vVec));

		DirectX::XMVECTOR vVecTo = vCurPoint - vPrevPoint;
		
		DirectX::XMVECTOR vPlaneNormal = DirectX::XMVector3Cross(
			DirectX::XMLoadFloat3(PLTVECTOR_TO_PXMFLOAT3(&pPoly->m_pPlane->m_vNormal)), 
			vVecTo
		);

		vPlaneNormal = DirectX::XMVector3Normalize(vPlaneNormal);
		DirectX::XMStoreFloat3(&plane.m_vNormal, vPlaneNormal);

		plane.m_fDist = DirectX::XMVectorGetX(DirectX::XMVector3Dot(vPlaneNormal, vCurPoint));

		#define CLIPTEST PLANETEST
		#define DOCLIP DOPLANECLIP
		#include "polyclip.h"
		#undef CLIPTEST
		#undef DOCLIP

		pPrevPoint = pCurPoint;
		++pCurPoint;
	}

	*ppPoints = pVerts;
	*pdwPoints = dwVerts;

	return true;
}

CSpriteData::~CSpriteData()
{
	if (m_dwBankedVBIndex != UINT32_MAX)
		g_GlobalMgr.GetBankedVertexBuffer(m_dwBankedVBIndex).m_bInUse = false;
}

bool CSpriteData::CreateVertexBuffer()
{
	m_dwBankedVBIndex = g_GlobalMgr.AssignBankedVertexBuffer(m_aData, sizeof(m_aData));
	return (m_dwBankedVBIndex != UINT32_MAX);
}

bool CSpriteData::UpdateVertexBuffer(InternalSpriteVertex* pNewVerts)
{
	if (!CompareTo(pNewVerts))
	{
		memcpy(m_aData, pNewVerts, sizeof(InternalSpriteVertex) * SPRITE_MAX_VERTICES);

		if (!g_GlobalMgr.UpdateGenericBuffer(g_GlobalMgr.GetBankedVertexBuffer(m_dwBankedVBIndex).m_pBuffer,
			pNewVerts, sizeof(InternalSpriteVertex) * SPRITE_MAX_VERTICES, D3D11_MAP_WRITE_DISCARD))
		{
			return false;
		}
	}

	return true;
}

void CSpriteBatchManager::FreeAllData()
{
	RELEASE_INTERFACE(m_pVertexBuffer, g_szFreeError_VB);
	RELEASE_INTERFACE(m_pIndexBuffer, g_szFreeError_IB);
}

void CSpriteBatchManager::Init()
{
	CBaseDataManager::Init();

	m_dwVertexCount = 0;
	m_dwIndexCount = 0;
	m_dwSpriteCount = 0;
	m_dwTextureCount = 0;
	m_bHasReallyCloseItems = false;
	m_pMappedVertexData = nullptr;
	m_pMappedIndexData = nullptr;

	memset(m_apSRV, 0, sizeof(m_apSRV));
	m_aCommand.reserve(SPRITE_BATCH_SIZE);

	m_pVertexBuffer = g_GlobalMgr.CreateVertexBuffer(
		sizeof(InternalCanvasBatchVertex) * SPRITE_DEFAULT_VERTICES * SPRITE_BATCH_SIZE,
		D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

	m_pIndexBuffer = g_GlobalMgr.CreateIndexBuffer(
		sizeof(uint16) * SPRITE_DEFAULT_INDICES * SPRITE_BATCH_SIZE,
		D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
}

void CSpriteBatchManager::BatchSprite(SpriteInstance* pInstance, SpriteEntry* pEntry)
{
	if (IsFull())
		InternalRenderBatch();

	uint32 dwRenderMode = (pInstance->m_Base.m_dwFlags & FLAG_REALLYCLOSE) ? MODE_REALLY_CLOSE : 0;
	uint32 dwMappedVerts = 0;
	uint32 dwMappedIndices = 0;

	BlendState eBlendState = BLEND_STATE_Default;
	StencilState eStencilState = g_RenderStateMgr.GetStencilState();
	SharedTexture* pTexture = pEntry->m_pTexture;

	g_RenderModeMgr.SetupRenderMode_Sprite(pTexture, pInstance->m_Base.m_dwFlags, pInstance->m_Base.m_dwFlags2,
		eBlendState, dwRenderMode, BLEND_STATE_Alpha);

	CRenderShader_SpriteBatch::VPSPerObjectParams& params = m_aParams[m_dwSpriteCount];

	InternalSpriteBatchVertex aSpriteVertex[SPRITE_DEFAULT_VERTICES];
	InternalSpriteBatchVertex* pFinalSpriteVerts = aSpriteVertex;

	if (pInstance->m_Base.m_dwFlags & FLAG_ROTATEABLESPRITE)
	{	
		InternalSpriteBatchVertex aClippedSpriteVertex[SPRITE_CLIPPED_VERTICES];

		dwMappedVerts = BatchSprite_Rotatable(m_pViewParams, pInstance, &pFinalSpriteVerts, aClippedSpriteVertex,
			pTexture, !!(dwRenderMode & MODE_REALLY_CLOSE), m_dwSpriteCount);
	}
	else
	{
		dwMappedVerts = BatchSprite_Normal(m_pViewParams, pInstance, pFinalSpriteVerts, pTexture,
			!!(dwRenderMode & MODE_REALLY_CLOSE), m_dwSpriteCount);
	}

	if (!dwMappedVerts)
		return;

	dwMappedIndices = (dwMappedVerts - 2) * 3;

	constexpr uint32 c_dwVertsPerBuffer = SPRITE_DEFAULT_VERTICES * SPRITE_BATCH_SIZE;
	constexpr uint32 c_dwIndicesPerBuffer = SPRITE_DEFAULT_INDICES * SPRITE_BATCH_SIZE;

	if ((m_dwVertexCount + dwMappedVerts > c_dwVertsPerBuffer) ||
		(m_dwIndexCount + dwMappedIndices > c_dwIndicesPerBuffer))
	{
		InternalRenderBatch();
	}

	if (!MapBuffers())
		return;

	UpdateBuffers(pFinalSpriteVerts, m_dwVertexCount, dwMappedVerts, m_dwIndexCount, dwMappedIndices);

	if (pTexture->m_pStateChange != nullptr)
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
	params.m_dwTextureIndex = BatchTexture(((RTexture*)pTexture->m_pRenderData)->m_pResourceView);

	d3d_GetSpriteColor(&pInstance->m_Base, &params.m_vDiffuseColor);

	AppendCommand(eBlendState, eStencilState, !!(dwRenderMode & MODE_REALLY_CLOSE), m_dwVertexCount, dwMappedVerts, 
		m_dwIndexCount, dwMappedIndices);

	m_dwSpriteCount++;
}

void CSpriteBatchManager::InternalRenderBatch()
{
	if (m_pMappedIndexData != nullptr)
	{
		g_GlobalMgr.UnmapGenericBuffer(m_pVertexBuffer);
		m_pMappedVertexData = nullptr;

		g_GlobalMgr.UnmapGenericBuffer(m_pIndexBuffer);
		m_pMappedIndexData = nullptr;
	}

	SetBuffersAndTopology();

	CRenderShader_SpriteBatch* pRenderShader = g_RenderShaderMgr.GetRenderShader<CRenderShader_SpriteBatch>();

	XMFloat4x4Trinity sTransforms;
	XMFloat4x4Trinity sTransformsRC;

	sTransforms.m_mWorld = g_mIdentity;
	sTransforms.m_mWorldView = sTransforms.m_mWorld;

	if (m_bHasReallyCloseItems)
	{
		sTransformsRC = sTransforms;
		DirectX::XMFLOAT4X4 mReallyCloseProj;

		d3d_CalcReallyCloseMatrix(&mReallyCloseProj, g_CV_RCSpriteFOVOffset.m_Val, g_CV_RCSpriteFOVOffset.m_Val, false);
		d3d_GetFinalSpriteTransform_ReallyClose(&sTransformsRC.m_mWorldViewProj, &mReallyCloseProj);
	}

	d3d_GetFinalSpriteTransform_Normal(m_pViewParams, &sTransforms.m_mWorldViewProj, &sTransforms.m_mWorldView);

	if (!pRenderShader->SetPerObjectParams(m_apSRV, m_dwTextureCount, &sTransforms, &sTransformsRC, m_aParams, m_dwSpriteCount))
		return;

	g_RenderStateMgr.SavePrimaryStates();

	g_RenderStateMgr.SetRasterState(!g_CV_Wireframe.m_Val ? RASTER_STATE_Default : RASTER_STATE_Wireframe);

	for (CSpriteBatchManager::Command& command : m_aCommand)
	{
		CReallyCloseData reallyCloseData;
		if (command.m_bReallyClose)
			d3d_SetReallyClose(&reallyCloseData);

		g_RenderStateMgr.SetBlendState(command.m_eBlendState);
		g_RenderStateMgr.SetStencilState(command.m_eStencilState);

		pRenderShader->Render(command.m_dwIndexCount, command.m_dwIndexOffset);

		if (command.m_bReallyClose)
			d3d_UnsetReallyClose(&reallyCloseData);
	}

	g_RenderStateMgr.RestorePrimaryStates();

	// TODO - not needed?
	for (uint32 i = 0; i < m_dwTextureCount; i++)
		m_apSRV[i]->Release();

	m_dwVertexCount = 0;
	m_dwIndexCount = 0;
	m_dwSpriteCount = 0;
	m_dwTextureCount = 0;
	m_bHasReallyCloseItems = false;

	m_aCommand.clear();
}

uint32 CSpriteBatchManager::BatchTexture(ID3D11ShaderResourceView* pSRV)
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

uint32 CSpriteBatchManager::BatchSprite_Normal(ViewParams* pParams, SpriteInstance* pInstance, 
	InternalSpriteBatchVertex* pSpriteVerts, SharedTexture* pSharedTexture, bool bReallyClose, 
	uint32 dwDataIndex)
{
	DirectX::XMVECTOR vBasisRight;
	DirectX::XMVECTOR vBasisUp;
	DirectX::XMVECTOR vBasisPos;
	DirectX::XMVECTOR vBasisForward;

	if (bReallyClose)
	{
		vBasisRight = DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
		vBasisUp = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
		vBasisForward = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
		vBasisPos = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	}
	else
	{
		vBasisRight = DirectX::XMLoadFloat3(&pParams->m_vRight);
		vBasisUp = DirectX::XMLoadFloat3(&pParams->m_vUp);
		vBasisForward = DirectX::XMLoadFloat3(&pParams->m_vForward);
		vBasisPos = DirectX::XMLoadFloat3(&pParams->m_vPos);
	}

	DirectX::XMVECTOR vPos = DirectX::XMLoadFloat3(PLTVECTOR_TO_PXMFLOAT3(&pInstance->m_Base.m_vPos));

	float fZ = DirectX::XMVectorGetX(DirectX::XMVector3Dot((vPos - vBasisPos), vBasisForward));
	float fCheckZ = !bReallyClose ? g_CV_NearZ.m_Val : g_CV_ReallyCloseNearZ.m_Val;

	if (fZ < fCheckZ)
		return 0;

	float fWidth = (float)((RTexture*)pSharedTexture->m_pRenderData)->m_dwScaledWidth;
	float fHeight = (float)((RTexture*)pSharedTexture->m_pRenderData)->m_dwScaledHeight;

	float fSizeU = 1.0f / fWidth;
	float fSizeV = 1.0f / fHeight;
	float fMinU = fSizeU + fSizeU;
	float fMinV = fSizeV + fSizeV;
	float fMaxU = (fWidth - 2.0f) * fSizeU;
	float fMaxV = (fHeight - 2.0f) * fSizeV;

	float fSizeX = fWidth * pInstance->m_Base.m_vScale.x;
	float fSizeY = fHeight * pInstance->m_Base.m_vScale.y;

	if (pInstance->m_Base.m_dwFlags & FLAG_GLOWSPRITE)
	{
		float fFactor = (fZ - SPRITE_MINFACTORDIST) / (SPRITE_MAXFACTORDIST - SPRITE_MINFACTORDIST);
		fFactor = LTCLAMP(fFactor, 0.0f, 1.0f);
		fFactor = SPRITE_MINFACTOR + ((SPRITE_MAXFACTOR - SPRITE_MINFACTOR) * fFactor);

		fSizeX *= fFactor;
		fSizeY *= fFactor;
	}

	DirectX::XMVECTOR vRight = vBasisRight * fSizeX;
	DirectX::XMVECTOR vUp = vBasisUp * fSizeY;

	DirectX::XMStoreFloat3(&pSpriteVerts[0].vPosition, vPos + vUp - vRight);
	pSpriteVerts[0].vTexCoords = { fMinU, fMinV };
	pSpriteVerts[0].dwDataIndex = dwDataIndex;

	DirectX::XMStoreFloat3(&pSpriteVerts[1].vPosition, vPos + vUp + vRight);
	pSpriteVerts[1].vTexCoords = { fMaxU, fMinV };
	pSpriteVerts[1].dwDataIndex = dwDataIndex;

	DirectX::XMStoreFloat3(&pSpriteVerts[2].vPosition, vPos + vRight - vUp);
	pSpriteVerts[2].vTexCoords = { fMaxU, fMaxV };
	pSpriteVerts[2].dwDataIndex = dwDataIndex;

	DirectX::XMStoreFloat3(&pSpriteVerts[3].vPosition, vPos - vRight - vUp);
	pSpriteVerts[3].vTexCoords = { fMinU, fMaxV };
	pSpriteVerts[3].dwDataIndex = dwDataIndex;

	if (pInstance->m_Base.m_dwFlags2 & FLAG2_SPRITE_TROTATE)
	{
		DirectX::XMFLOAT4X4 mRotation;
		Matrix_FromQuaternionLT(&mRotation, PLTROTATION_TO_PXMFLOAT4(&pInstance->m_Base.m_rRot));

		float fHalfU = (fMinU + fMaxU) * 0.5f;
		float fHalfV = (fMinV + fMaxV) * 0.5f;

		float fCos1 = mRotation.m[0][0]; // cos
		float fSin1 = mRotation.m[0][1]; // sin
		float fSin2 = mRotation.m[1][0]; // -sin
		float fCos2 = mRotation.m[1][1]; // cos

		for (uint32 dwCurrPt = 0; dwCurrPt < SPRITE_DEFAULT_VERTICES; dwCurrPt++)
		{
			float fCenterV = pSpriteVerts[dwCurrPt].vTexCoords.y - fHalfV;
			float fCenterU = pSpriteVerts[dwCurrPt].vTexCoords.x - fHalfU;

			pSpriteVerts[dwCurrPt].vTexCoords.x = fCenterU * fCos1 + fCenterV * fSin1 + fHalfU;
			pSpriteVerts[dwCurrPt].vTexCoords.y = fCenterU * fSin2 + fCenterV * fCos2 + fHalfV;
		}
	}

	if ((pInstance->m_Base.m_dwFlags & FLAG_SPRITEBIAS) && !bReallyClose)
		d3d_BiasSprite_Jupiter(pSpriteVerts, fZ, vBasisPos, vBasisRight, vBasisUp, vBasisForward);

	return SPRITE_DEFAULT_VERTICES;
}

uint32 CSpriteBatchManager::BatchSprite_Rotatable(ViewParams* pParams, SpriteInstance* pInstance, 
	InternalSpriteBatchVertex** ppSpriteVerts, InternalSpriteBatchVertex* pClippedSpriteVerts,
	SharedTexture* pSharedTexture, bool bReallyClose, uint32 dwDataIndex)
{
	float fWidth = (float)(((RTexture*)pSharedTexture->m_pRenderData)->m_dwScaledWidth);
	float fHeight = (float)(((RTexture*)pSharedTexture->m_pRenderData)->m_dwScaledHeight);

	float fSizeU = 1.0f / fWidth;
	float fSizeV = 1.0f / fHeight;
	float fMinU = fSizeU + fSizeU;
	float fMinV = fSizeV + fSizeV;
	float fMaxU = (fWidth - 2.0f) * fSizeU;
	float fMaxV = (fHeight - 2.0f) * fSizeV;

	DirectX::XMVECTOR vPos = DirectX::XMLoadFloat3(PLTVECTOR_TO_PXMFLOAT3(&pInstance->m_Base.m_vPos));

	DirectX::XMFLOAT4X4 mRotation;
	d3d_SetupTransformation(&pInstance->m_Base.m_vPos, (float*)&pInstance->m_Base.m_rRot, &pInstance->m_Base.m_vScale, &mRotation);

	DirectX::XMFLOAT3 vRightTemp;
	DirectX::XMFLOAT3 vUpTemp;
	DirectX::XMFLOAT3 vForwardTemp;

	Matrix_GetBasisVectorsLT(&mRotation, &vRightTemp, &vUpTemp, &vForwardTemp);

	DirectX::XMVECTOR vRight = DirectX::XMLoadFloat3(&vRightTemp);
	DirectX::XMVECTOR vUp = DirectX::XMLoadFloat3(&vUpTemp);
	DirectX::XMVECTOR vForward = DirectX::XMLoadFloat3(&vForwardTemp);

	vRight *= fWidth;
	vUp *= fHeight;

	InternalSpriteBatchVertex* pSpriteVerts = *ppSpriteVerts;

	DirectX::XMStoreFloat3(&pSpriteVerts[0].vPosition, vPos + vUp - vRight);
	pSpriteVerts[0].vTexCoords = { fMinU, fMinV };
	pSpriteVerts[0].dwDataIndex = dwDataIndex;

	DirectX::XMStoreFloat3(&pSpriteVerts[1].vPosition, vPos + vUp + vRight);
	pSpriteVerts[1].vTexCoords = { fMaxU, fMinV };
	pSpriteVerts[1].dwDataIndex = dwDataIndex;

	DirectX::XMStoreFloat3(&pSpriteVerts[2].vPosition, vPos + vRight - vUp);
	pSpriteVerts[2].vTexCoords = { fMaxU, fMaxV };
	pSpriteVerts[2].dwDataIndex = dwDataIndex;

	DirectX::XMStoreFloat3(&pSpriteVerts[3].vPosition, vPos - vRight - vUp);
	pSpriteVerts[3].vTexCoords = { fMinU, fMaxV };
	pSpriteVerts[3].dwDataIndex = dwDataIndex;

	uint32 dwPoints = SPRITE_DEFAULT_VERTICES;

	if (pInstance->m_hClipperPoly != INVALID_HPOLY)
	{
		if (!d3d_ClipSprite(pInstance, pInstance->m_hClipperPoly, ppSpriteVerts, &dwPoints, pClippedSpriteVerts))
			return 0;

		if (dwPoints > SPRITE_MAX_VERTICES)
			dwPoints = SPRITE_MAX_VERTICES;

		for (uint32 i = 0; i < dwPoints; i++)
			(*ppSpriteVerts)[i].dwDataIndex = dwDataIndex;
	}

	if ((pInstance->m_Base.m_dwFlags & FLAG_SPRITEBIAS) && !bReallyClose)
		d3d_BiasRotatableSprite_Simple(pParams, pSpriteVerts, dwPoints, SPRITE_POSITION_ZBIAS_RS);

	return dwPoints;
}

void CSpriteBatchManager::UpdateBuffers(InternalSpriteBatchVertex* pSpriteVerts, uint32 dwVertexStart, uint32 dwVertexCount,
	uint32 dwIndexStart, uint32 dwIndexCount)
{
	memcpy(m_pMappedVertexData + dwVertexStart, pSpriteVerts, sizeof(InternalSpriteBatchVertex) * dwVertexCount);
	
	uint16 awIndex[SPRITE_MAX_INDICES] = { 0, 1, 2, 0, 2, 3, 0, 3, 4, 0, 4, 5, 0, 5, 6, 0, 6, 7 };
	for (uint32 i = 0; i < dwIndexCount; i++)
		m_pMappedIndexData[dwIndexStart + i] = awIndex[i] + m_dwVertexCount;
}

bool CSpriteBatchManager::MapBuffers()
{
	if (m_pMappedVertexData == nullptr)
	{
		D3D11_MAPPED_SUBRESOURCE subResource;

		if (!g_GlobalMgr.MapGenericBuffer(m_pVertexBuffer, D3D11_MAP_WRITE_DISCARD, &subResource))
			return false;

		m_pMappedVertexData = (InternalSpriteBatchVertex*)subResource.pData;
	}

	if (m_pMappedIndexData == nullptr)
	{
		D3D11_MAPPED_SUBRESOURCE subResource;

		if (!g_GlobalMgr.MapGenericBuffer(m_pIndexBuffer, D3D11_MAP_WRITE_DISCARD, &subResource))
		{
			g_GlobalMgr.UnmapGenericBuffer(m_pVertexBuffer);
			m_pMappedVertexData = nullptr;

			return false;
		}

		m_pMappedIndexData = (uint16*)subResource.pData;
	}

	return true;
}

void CSpriteBatchManager::SetBuffersAndTopology()
{
	g_RenderShaderMgr.SetVertexResource(VRS_Main, m_pVertexBuffer, sizeof(InternalSpriteBatchVertex), 0);
	g_RenderShaderMgr.SetIndexBuffer16(m_pIndexBuffer, 0);
	g_RenderShaderMgr.SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void CSpriteBatchManager::AppendCommand(BlendState eBlendState, StencilState eStencilState, bool bReallyClose,
	uint32 dwVertexOffset, uint32 dwVertexCount, uint32 dwIndexOffset, uint32 dwIndexCount)
{
	if (bReallyClose)
		m_bHasReallyCloseItems = true;

	if (m_aCommand.empty())
	{
		m_aCommand.emplace_back(eBlendState, eStencilState, bReallyClose, dwVertexOffset, dwVertexCount, 
			dwIndexOffset, dwIndexCount);

		m_dwVertexCount += dwVertexCount;
		m_dwIndexCount += dwIndexCount;

		return;
	}

	Command& command = m_aCommand.back();

	if (command.m_eBlendState == eBlendState && command.m_eStencilState == eStencilState &&
		command.m_bReallyClose == bReallyClose)
	{
		command.m_dwVertexCount += dwVertexCount;
		command.m_dwIndexCount += dwIndexCount;
	}
	else
	{
		m_aCommand.emplace_back(eBlendState, eStencilState, bReallyClose, dwVertexOffset, dwVertexCount,
			dwIndexOffset, dwIndexCount);
	}

	m_dwVertexCount += dwVertexCount;
	m_dwIndexCount += dwIndexCount;
}

CSpriteData* CSpriteManager::GetSpriteData(SpriteInstance* pInstance, InternalSpriteVertex* pVerts)
{
	auto iter = m_Data.find(pInstance->m_SpriteTracker.m_pSprite);

	if (iter != m_Data.end())
	{
		CSpriteData* pData = (*iter).second;

		pData->UpdateVertexBuffer(pVerts);
		pData->SetLastUpdate(g_fLastClientTime);

		return pData;
	}

	CSpriteData* pNewData = new CSpriteData();
	memcpy(pNewData->GetData(), pVerts, sizeof(InternalSpriteVertex) * SPRITE_MAX_VERTICES);

	if (!pNewData->CreateVertexBuffer())
	{
		delete pNewData;
		return nullptr;
	}

	pNewData->SetLastUpdate(g_fLastClientTime);
	m_Data[pInstance->m_SpriteTracker.m_pSprite] = pNewData;

	return pNewData;
}

static void d3d_GetSpriteColor(LTObject* pObject, DirectX::XMFLOAT4* pColor)
{
	float fAlpha = (float)pObject->m_nColorA * MATH_ONE_OVER_255;

	// TODO - additive? multiply?
	if ((pObject->m_dwFlags & FLAG_NOLIGHT) || (pObject->m_dwFlags2 & FLAG2_ADDITIVE) ||
		!g_bHaveWorld || (!g_CV_DynamicLightSprites.m_Val))
	{
		pColor->x = (float)pObject->m_nColorR * MATH_ONE_OVER_255;
		pColor->y = (float)pObject->m_nColorG * MATH_ONE_OVER_255;
		pColor->z = (float)pObject->m_nColorB * MATH_ONE_OVER_255;
		pColor->w = fAlpha;
	}
	else
	{
		DirectX::XMFLOAT3 vAmbientColor;
		DirectX::XMFLOAT3 vDynamicColor = { };

		DirectX::XMFLOAT3 vPos = *PLTVECTOR_TO_PXMFLOAT3(&pObject->m_vPos);
		if (pObject->m_dwFlags & FLAG_REALLYCLOSE)
		{
			DirectX::XMStoreFloat3(&vPos, DirectX::XMVector4Transform(DirectX::XMLoadFloat3(&vPos), 
				DirectX::XMLoadFloat4x4(&g_ViewParams.m_mInvViewTransposed)));
		}

		d3d_LightLookup_Original(&g_pSceneDesc->m_pRenderContext->m_pMainWorld->m_LightTable,
			&vPos, &vAmbientColor);

		d3d_CalcLightAdd(&vPos, &vDynamicColor, MAX_SPRITE_LIGHTS);

		//DirectX::XMVECTOR vFinalColor = DirectX::XMVectorSet((float)pObject->m_nColorR * MATH_ONE_OVER_255, 
		//	(float)pObject->m_nColorG /* MATH_ONE_OVER_255, 
		// (float)pObject->m_nColorB * MATH_ONE_OVER_255, 0.0f);

		DirectX::XMFLOAT3 vObjectColor =
		{
			(float)pObject->m_nColorR * MATH_ONE_OVER_255,
			(float)pObject->m_nColorG * MATH_ONE_OVER_255,
			(float)pObject->m_nColorB * MATH_ONE_OVER_255,
		};

		DirectX::XMVECTOR vAmbientColorTemp = DirectX::XMLoadFloat3(&vAmbientColor) * DirectX::XMLoadFloat3(&vObjectColor);
		DirectX::XMVECTOR vDynamicColorTemp = DirectX::XMLoadFloat3(&vDynamicColor);

		DirectX::XMStoreFloat4(pColor, 
			DirectX::XMVectorSaturate(vAmbientColorTemp + vDynamicColorTemp));

		pColor->w = fAlpha;
	}
}

static void d3d_GetFinalSpriteTransform(ViewParams* pParams, DirectX::XMFLOAT4X4* pTransform, 
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

static void d3d_GetFinalSpriteTransform_Normal(ViewParams* pParams, DirectX::XMFLOAT4X4* pTransform,
	DirectX::XMFLOAT4X4* pTransformNoProj)
{
	*pTransformNoProj = pParams->m_mViewTransposed;

	DirectX::XMStoreFloat4x4(pTransform, DirectX::XMLoadFloat4x4(&pParams->m_mViewTransposed) *
		DirectX::XMLoadFloat4x4(&pParams->m_mProjectionTransposed));
}

static void d3d_GetFinalSpriteTransform_ReallyClose(DirectX::XMFLOAT4X4* pTransform, DirectX::XMFLOAT4X4* pReallyCloseProj)
{
	DirectX::XMStoreFloat4x4(pTransform, DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(pReallyCloseProj)));
}

static void d3d_SetBuffersAndTopology(CSpriteData* pData, uint32 dwIndexCount)
{
	g_RenderShaderMgr.SetVertexResource(VRS_Main, g_GlobalMgr.GetBankedVertexBuffer(pData->m_dwBankedVBIndex).m_pBuffer,
		sizeof(InternalSpriteVertex), 0);
	g_RenderShaderMgr.SetIndexBuffer16(
		dwIndexCount == SPRITE_DEFAULT_INDICES ? g_GlobalMgr.GetIndexBuffer_Sprite() : g_GlobalMgr.GetIndexBuffer_SpriteEx(), 
		0);
	g_RenderShaderMgr.SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

template<typename T>
static void d3d_BiasRotatableSprite_Jupiter(ViewParams* pParams, T* pPoints, uint32 dwPoints, 
	float fBiasDistIn)
{
	DirectX::XMVECTOR vParamsPos = DirectX::XMLoadFloat3(&pParams->m_vPos);
	DirectX::XMVECTOR vParamsRight = DirectX::XMLoadFloat3(&pParams->m_vRight);
	DirectX::XMVECTOR vParamsUp = DirectX::XMLoadFloat3(&pParams->m_vUp);
	DirectX::XMVECTOR vParamsForward = DirectX::XMLoadFloat3(&pParams->m_vForward);

	for (uint32 dwCurrPt = 0; dwCurrPt < dwPoints; dwCurrPt++)
	{
		DirectX::XMVECTOR vPt = DirectX::XMLoadFloat3(&pPoints[dwCurrPt].vPosition);
		DirectX::XMVECTOR vPtRelCamera = vPt - vParamsPos;

		float fZ = DirectX::XMVectorGetX(DirectX::XMVector3Dot(vPtRelCamera, vParamsForward));
		float fNearZ = g_CV_NearZ.m_Val;

		if (fZ <= fNearZ)
			continue;

		float fBiasDist = fBiasDistIn;
		if ((fZ + fBiasDist) < fNearZ)
			fBiasDist = fNearZ - fZ + 0.001f; // TODO - floating point precision?

		float fScale = 1.0f + fBiasDist / fZ;

		float fDotRight = DirectX::XMVectorGetX(DirectX::XMVector3Dot(vPtRelCamera, vParamsRight));
		float fDotUp = DirectX::XMVectorGetX(DirectX::XMVector3Dot(vPtRelCamera, vParamsUp));

		DirectX::XMVECTOR vNewPt = vParamsRight * fDotRight * fScale + vParamsUp * fDotUp * fScale +
			(fZ + fBiasDist) * vParamsForward + vParamsPos;

		DirectX::XMStoreFloat3(&pPoints[dwCurrPt].vPosition, vNewPt);
	}
}

template<typename T>
static void d3d_BiasRotatableSprite_Simple(ViewParams* pParams, T* pPoints, uint32 dwPoints, 
	float fBiasDistIn)
{
	DirectX::XMVECTOR vParamsPos = DirectX::XMLoadFloat3(&pParams->m_vPos);
	DirectX::XMVECTOR vParamsForward = DirectX::XMLoadFloat3(&pParams->m_vForward);

	float fNearZ = g_CV_NearZ.m_Val;

	for (uint32 dwCurrPt = 0; dwCurrPt < dwPoints; dwCurrPt++)
	{
		DirectX::XMVECTOR vPt = DirectX::XMLoadFloat3(&pPoints[dwCurrPt].vPosition);
		DirectX::XMVECTOR vPtRelCamera = vPt - vParamsPos;

		float fZ = DirectX::XMVectorGetX(DirectX::XMVector3Dot(vPtRelCamera, vParamsForward));

		if (fZ <= fNearZ)
			continue;

		float fBiasDist = fBiasDistIn;
		if ((fZ + fBiasDist) < fNearZ)
			fBiasDist = fNearZ - fZ;

		float fScale = 1.0f - fBiasDist / fZ * SPRITE_SCALE_ZBIAS_RS;

		vPt = vPt + fBiasDist * DirectX::XMVector3Normalize(vPtRelCamera) * fScale;

		DirectX::XMStoreFloat3(&pPoints[dwCurrPt].vPosition, vPt);
	}
}

// TODO - sprites are incorrectly warping close to camera (see exo-laser impact puffs and lamp sprites on dm_reservoir)
static void d3d_DrawRotatableSprite(ViewParams* pParams, SpriteInstance* pInstance, SharedTexture* pSharedTexture, 
	DirectX::XMFLOAT4X4* pReallyCloseProj, uint32 dwRenderMode)
{
	float fWidth = (float)(((RTexture*)pSharedTexture->m_pRenderData)->m_dwScaledWidth);
	float fHeight = (float)(((RTexture*)pSharedTexture->m_pRenderData)->m_dwScaledHeight);

	float fSizeU = 1.0f / fWidth;
	float fSizeV = 1.0f / fHeight;
	float fMinU = fSizeU + fSizeU;
	float fMinV = fSizeV + fSizeV;
	float fMaxU = (fWidth - 2.0f) * fSizeU;
	float fMaxV = (fHeight - 2.0f) * fSizeV;

	DirectX::XMVECTOR vPos = DirectX::XMLoadFloat3(PLTVECTOR_TO_PXMFLOAT3(&pInstance->m_Base.m_vPos));

	DirectX::XMFLOAT4X4 mRotation;
	d3d_SetupTransformation(&pInstance->m_Base.m_vPos, (float*)&pInstance->m_Base.m_rRot, &pInstance->m_Base.m_vScale, &mRotation);

	DirectX::XMFLOAT3 vRightTemp;
	DirectX::XMFLOAT3 vUpTemp;
	DirectX::XMFLOAT3 vForwardTemp;

	Matrix_GetBasisVectorsLT(&mRotation, &vRightTemp, &vUpTemp, &vForwardTemp);
	
	DirectX::XMVECTOR vRight = DirectX::XMLoadFloat3(&vRightTemp);
	DirectX::XMVECTOR vUp = DirectX::XMLoadFloat3(&vUpTemp);
	DirectX::XMVECTOR vForward = DirectX::XMLoadFloat3(&vForwardTemp);
	
	vRight *= fWidth;
	vUp *= fHeight;

	// TODO - 10 vertices should be safe enough?
	InternalSpriteVertex aSpriteVerts[SPRITE_DEFAULT_VERTICES];
	DirectX::XMStoreFloat3(&aSpriteVerts[0].vPosition, vPos + vUp - vRight);
	aSpriteVerts[0].vTexCoords = { fMinU, fMinV };

	DirectX::XMStoreFloat3(&aSpriteVerts[1].vPosition, vPos + vUp + vRight);
	aSpriteVerts[1].vTexCoords = { fMaxU, fMinV };

	DirectX::XMStoreFloat3(&aSpriteVerts[2].vPosition, vPos + vRight - vUp);
	aSpriteVerts[2].vTexCoords = { fMaxU, fMaxV };

	DirectX::XMStoreFloat3(&aSpriteVerts[3].vPosition, vPos - vRight - vUp);
	aSpriteVerts[3].vTexCoords = { fMinU, fMaxV };

	InternalSpriteVertex* pPoints = aSpriteVerts;
	uint32 dwPoints = SPRITE_DEFAULT_VERTICES;;
	InternalSpriteVertex aClippedSpriteVerts[SPRITE_CLIPPED_VERTICES];

	// TODO - rewrite / cleanup clipping stuff
	if (pInstance->m_hClipperPoly != INVALID_HPOLY)
	{	
		if (!d3d_ClipSprite(pInstance, pInstance->m_hClipperPoly, &pPoints, &dwPoints, aClippedSpriteVerts))
			return;

		if (dwPoints > SPRITE_MAX_VERTICES)
			dwPoints = SPRITE_MAX_VERTICES;
	}

	if ((pInstance->m_Base.m_dwFlags & FLAG_SPRITEBIAS) && pReallyCloseProj == nullptr)
	{
		d3d_BiasRotatableSprite_Simple(pParams, pPoints, dwPoints, SPRITE_POSITION_ZBIAS_RS);
	}

	CSpriteData* pSpriteData = g_GlobalMgr.GetSpriteMgr()->GetSpriteData(pInstance, pPoints);

	if (pSpriteData == nullptr)
		return;

	uint32 dwIndexCount = (dwPoints - 2) * 3;
	d3d_SetBuffersAndTopology(pSpriteData, dwIndexCount);

	CRenderShader_Sprite* pRenderShader = g_RenderShaderMgr.GetRenderShader<CRenderShader_Sprite>();

	DirectX::XMFLOAT3 vModeColor = { };
	if (pSharedTexture->m_pStateChange != nullptr)
	{
		BlendState eBlendStateOverride = BLEND_STATE_Invalid;
		StencilState eStencilStateOverride = STENCIL_STATE_Invalid;

		g_RenderModeMgr.ApplyStateChange(pSharedTexture->m_pStateChange, eBlendStateOverride, eStencilStateOverride,
			dwRenderMode, &vModeColor);

		if (eBlendStateOverride != BLEND_STATE_Invalid)
			g_RenderStateMgr.SetBlendState(eBlendStateOverride);

		if (eStencilStateOverride != STENCIL_STATE_Invalid)
			g_RenderStateMgr.SetStencilState(eStencilStateOverride);
	}

	DirectX::XMFLOAT4 vDiffuseColor;
	d3d_GetSpriteColor(&pInstance->m_Base, &vDiffuseColor);

	XMFloat4x4Trinity sTransforms;
	sTransforms.m_mWorld = g_mIdentity;
	sTransforms.m_mWorldView = sTransforms.m_mWorld;

	d3d_GetFinalSpriteTransform(pParams, &sTransforms.m_mWorldViewProj, &sTransforms.m_mWorldView, pReallyCloseProj);

	if (!pRenderShader->SetPerObjectParams(((RTexture*)pSharedTexture->m_pRenderData)->m_pResourceView, dwRenderMode, 
		&vModeColor, &vDiffuseColor, &sTransforms))
	{
		return;
	}

	pRenderShader->Render(dwIndexCount);
}

template<typename T>
static void XM_CALLCONV d3d_BiasSprite_Jupiter(T* pPoints, float fZ, DirectX::FXMVECTOR vBasisPos,
	DirectX::FXMVECTOR vBasisRight, DirectX::FXMVECTOR vBasisUp, DirectX::GXMVECTOR vBasisForward)
{
	float fBiasDist = SPRITE_POSITION_ZBIAS;

	if ((fZ + fBiasDist) < g_CV_NearZ.m_Val)
		fBiasDist = g_CV_NearZ.m_Val - fZ + 0.001f; // TODO - floating point precision?

	float fScale = 1.0f + fBiasDist / fZ;

	for (uint32 dwCurrPt = 0; dwCurrPt < SPRITE_DEFAULT_VERTICES; dwCurrPt++)
	{
		DirectX::XMVECTOR vPt = DirectX::XMLoadFloat3(&pPoints[dwCurrPt].vPosition);

		float fDotRight = DirectX::XMVectorGetX(DirectX::XMVector3Dot(vPt - vBasisPos, vBasisRight));
		float fDotUp = DirectX::XMVectorGetX(DirectX::XMVector3Dot(vPt - vBasisPos, vBasisUp));

		vPt = vBasisRight * fDotRight * fScale + vBasisUp * fDotUp * fScale +
			(fZ + fBiasDist) * vBasisForward + vBasisPos;

		DirectX::XMStoreFloat3(&pPoints[dwCurrPt].vPosition, vPt);
	}
}

template<typename T>
static void XM_CALLCONV d3d_BiasSprite_Simple(T* pPoints, float fZ, DirectX::FXMVECTOR vBasisPos)
{
	float fBiasDist = SPRITE_POSITION_ZBIAS;

	if ((fZ + fBiasDist) < g_CV_NearZ.m_Val)
		fBiasDist = g_CV_NearZ.m_Val;

	float fScale = 1.0f + fBiasDist / fZ;

	for (uint32 dwCurrPt = 0; dwCurrPt < SPRITE_DEFAULT_VERTICES; dwCurrPt++)
	{
		DirectX::XMVECTOR vPt = DirectX::XMLoadFloat3(&pPoints[dwCurrPt].vPosition);
		DirectX::XMVECTOR vPtRelCamera = vPt - vBasisPos;

		vPt = vPt + fBiasDist * DirectX::XMVector3Normalize(vPtRelCamera);

		DirectX::XMStoreFloat3(&pPoints[dwCurrPt].vPosition, vPt);
	}
}

static void d3d_DrawSprite(ViewParams* pParams, SpriteInstance* pInstance, SharedTexture* pSharedTexture, 
	DirectX::XMFLOAT4X4* pReallyCloseProj, uint32 dwRenderMode)
{
	DirectX::XMVECTOR vBasisRight;
	DirectX::XMVECTOR vBasisUp;
	DirectX::XMVECTOR vBasisPos;
	DirectX::XMVECTOR vBasisForward;

	if (pReallyCloseProj != nullptr)
	{
		vBasisRight = DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
		vBasisUp = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
		vBasisForward = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
		vBasisPos = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	}
	else
	{
		vBasisRight = DirectX::XMLoadFloat3(&pParams->m_vRight);
		vBasisUp = DirectX::XMLoadFloat3(&pParams->m_vUp);
		vBasisForward = DirectX::XMLoadFloat3(&pParams->m_vForward);
		vBasisPos = DirectX::XMLoadFloat3(&pParams->m_vPos);
	}

	DirectX::XMVECTOR vPos = DirectX::XMLoadFloat3(PLTVECTOR_TO_PXMFLOAT3(&pInstance->m_Base.m_vPos));

	float fZ = DirectX::XMVectorGetX(DirectX::XMVector3Dot((vPos - vBasisPos), vBasisForward));
	float fCheckZ = pReallyCloseProj == nullptr ? g_CV_NearZ.m_Val : g_CV_ReallyCloseNearZ.m_Val;
	
	//if (pReallyCloseProj == nullptr && (fZ < g_CV_NearZ.m_Val))
	if (fZ < fCheckZ)
		return;

	float fWidth = (float)((RTexture*)pSharedTexture->m_pRenderData)->m_dwScaledWidth;
	float fHeight = (float)((RTexture*)pSharedTexture->m_pRenderData)->m_dwScaledHeight;

	float fSizeU = 1.0f / fWidth;
	float fSizeV = 1.0f / fHeight;
	float fMinU = fSizeU + fSizeU;
	float fMinV = fSizeV + fSizeV;
	float fMaxU = (fWidth - 2.0f) * fSizeU;
	float fMaxV = (fHeight - 2.0f) * fSizeV;

	float fSizeX = fWidth * pInstance->m_Base.m_vScale.x;
	float fSizeY = fHeight * pInstance->m_Base.m_vScale.y;

	if (pInstance->m_Base.m_dwFlags & FLAG_GLOWSPRITE)
	{
		float fFactor = (fZ - SPRITE_MINFACTORDIST) / (SPRITE_MAXFACTORDIST - SPRITE_MINFACTORDIST);
		fFactor = LTCLAMP(fFactor, 0.0f, 1.0f);
		fFactor = SPRITE_MINFACTOR + ((SPRITE_MAXFACTOR - SPRITE_MINFACTOR) * fFactor);

		fSizeX *= fFactor;
		fSizeY *= fFactor;
	}

	DirectX::XMVECTOR vRight = vBasisRight * fSizeX;
	DirectX::XMVECTOR vUp = vBasisUp * fSizeY;

	InternalSpriteVertex aSpriteVerts[SPRITE_DEFAULT_VERTICES];
	DirectX::XMStoreFloat3(&aSpriteVerts[0].vPosition, vPos + vUp - vRight);
	aSpriteVerts[0].vTexCoords = { fMinU, fMinV };

	DirectX::XMStoreFloat3(&aSpriteVerts[1].vPosition, vPos + vUp + vRight);
	aSpriteVerts[1].vTexCoords = { fMaxU, fMinV };

	DirectX::XMStoreFloat3(&aSpriteVerts[2].vPosition, vPos + vRight - vUp);
	aSpriteVerts[2].vTexCoords = { fMaxU, fMaxV };

	DirectX::XMStoreFloat3(&aSpriteVerts[3].vPosition, vPos - vRight - vUp);
	aSpriteVerts[3].vTexCoords = { fMinU, fMaxV };

	if (pInstance->m_Base.m_dwFlags2 & FLAG2_SPRITE_TROTATE)
	{
		DirectX::XMFLOAT4X4 mRotation;
		Matrix_FromQuaternionLT(&mRotation, PLTROTATION_TO_PXMFLOAT4(&pInstance->m_Base.m_rRot));

		float fHalfU = (fMinU + fMaxU) * 0.5f;
		float fHalfV = (fMinV + fMaxV) * 0.5f;

		float fCos1 = mRotation.m[0][0]; // cos
		float fSin1 = mRotation.m[0][1]; // sin
		float fSin2 = mRotation.m[1][0]; // -sin
		float fCos2 = mRotation.m[1][1]; // cos

		for (uint32 dwCurrPt = 0; dwCurrPt < SPRITE_DEFAULT_VERTICES; dwCurrPt++)
		{
			float fCenterV = aSpriteVerts[dwCurrPt].vTexCoords.y - fHalfV;
			float fCenterU = aSpriteVerts[dwCurrPt].vTexCoords.x - fHalfU;

			aSpriteVerts[dwCurrPt].vTexCoords.x = fCenterU * fCos1 + fCenterV * fSin1 + fHalfU;
			aSpriteVerts[dwCurrPt].vTexCoords.y = fCenterU * fSin2 + fCenterV * fCos2 + fHalfV;
		}
	}

	if ((pInstance->m_Base.m_dwFlags & FLAG_SPRITEBIAS) && pReallyCloseProj == nullptr)
	{
		d3d_BiasSprite_Jupiter(aSpriteVerts, fZ, vBasisPos, vBasisRight, vBasisUp, vBasisForward);
	}

	CSpriteData* pSpriteData = g_GlobalMgr.GetSpriteMgr()->GetSpriteData(pInstance, aSpriteVerts);

	if (pSpriteData == nullptr)
		return;

	d3d_SetBuffersAndTopology(pSpriteData, SPRITE_DEFAULT_INDICES);
	
	CRenderShader_Sprite* pRenderShader = g_RenderShaderMgr.GetRenderShader<CRenderShader_Sprite>();

	DirectX::XMFLOAT3 vModeColor = { };
	if (pSharedTexture->m_pStateChange != nullptr)
	{
		BlendState eBlendStateOverride = BLEND_STATE_Invalid;
		StencilState eStencilStateOverride = STENCIL_STATE_Invalid;

		g_RenderModeMgr.ApplyStateChange(pSharedTexture->m_pStateChange, eBlendStateOverride, eStencilStateOverride,
			dwRenderMode, &vModeColor);

		if (eBlendStateOverride != BLEND_STATE_Invalid)
			g_RenderStateMgr.SetBlendState(eBlendStateOverride);

		if (eStencilStateOverride != STENCIL_STATE_Invalid)
			g_RenderStateMgr.SetStencilState(eStencilStateOverride);
	}

	DirectX::XMFLOAT4 vDiffuseColor;
	d3d_GetSpriteColor(&pInstance->m_Base, &vDiffuseColor);

	XMFloat4x4Trinity sTransforms;
	sTransforms.m_mWorld = g_mIdentity;
	sTransforms.m_mWorldView = sTransforms.m_mWorld;

	d3d_GetFinalSpriteTransform(pParams, &sTransforms.m_mWorldViewProj, &sTransforms.m_mWorldView, pReallyCloseProj);
	
	if (!pRenderShader->SetPerObjectParams(((RTexture*)pSharedTexture->m_pRenderData)->m_pResourceView, dwRenderMode, 
		&vModeColor, &vDiffuseColor, &sTransforms))
	{
		return;
	}

	pRenderShader->Render(SPRITE_DEFAULT_INDICES);
}

void d3d_DrawSprite(ViewParams* pParams, LTObject* pObj)
{
	SpriteInstance* pInstance = pObj->ToSprite();
	SpriteEntry* pEntry = pInstance->m_SpriteTracker.m_pCurFrame;
	SpriteAnim* pAnim = pInstance->m_SpriteTracker.m_pCurAnim;

	if (pAnim != nullptr && pEntry != nullptr && pEntry->m_pTexture != nullptr)
	{
		if (g_CV_BatchSprites.m_Val)
		{
			g_GlobalMgr.GetSpriteBatchMgr()->SetViewParams(pParams);
			g_GlobalMgr.GetSpriteBatchMgr()->BatchSprite(pInstance, pEntry);
			return;
		}

		uint32 dwRenderMode = 0;
		BlendState eBlendState;

		g_RenderStateMgr.SavePrimaryStates();

		g_RenderModeMgr.SetupRenderMode_Sprite(pEntry->m_pTexture, pObj->m_dwFlags, pObj->m_dwFlags2, eBlendState, 
			dwRenderMode, BLEND_STATE_Alpha);

		g_RenderStateMgr.SetBlendState(eBlendState);
		g_RenderStateMgr.SetRasterState(!g_CV_Wireframe.m_Val ? RASTER_STATE_Default : RASTER_STATE_Wireframe);

		//bool bMultiply = (pObj->m_dwFlags2 & FLAG2_MULTIPLY) && !(pObj->m_dwFlags2 & FLAG2_ADDITIVE);

		DirectX::XMFLOAT4X4 mReallyCloseProj;
		DirectX::XMFLOAT4X4* pReallyCloseProj = nullptr;
		CReallyCloseData reallyCloseData;
		if (pObj->m_dwFlags & FLAG_REALLYCLOSE)
		{
			pReallyCloseProj = &mReallyCloseProj;
			d3d_SetReallyClose(&reallyCloseData, pReallyCloseProj, 
				g_CV_RCSpriteFOVOffset.m_Val, g_CV_RCSpriteFOVOffset.m_Val, false);

			dwRenderMode |= MODE_REALLY_CLOSE;
		}

		if (pObj->m_dwFlags & FLAG_ROTATEABLESPRITE)
			d3d_DrawRotatableSprite(pParams, pInstance, pEntry->m_pTexture, pReallyCloseProj, dwRenderMode);
		else
			d3d_DrawSprite(pParams, pInstance, pEntry->m_pTexture, pReallyCloseProj, dwRenderMode);

		if (pReallyCloseProj != nullptr)
			d3d_UnsetReallyClose(&reallyCloseData);

		g_RenderStateMgr.RestorePrimaryStates();
	}
}

void d3d_DrawSpriteEx(ViewParams* pParams, LTObject* pObj, LTObject* pPrevObject)
{
	g_GlobalMgr.GetCanvasBatchMgr()->RenderBatch();

	d3d_DrawSprite(pParams, pObj);
}

void d3d_ProcessSprite(LTObject* pObject)
{
	if (!g_CV_DrawSprites.m_Val)
		return;

	VisibleSet* pVisibleSet = d3d_GetVisibleSet();
	AllocSet* pSet = (pObject->m_dwFlags & FLAG_SPRITE_NOZ) ? pVisibleSet->GetNoZSprites() : pVisibleSet->GetSprites();

	pSet->Add(pObject);
}

void d3d_QueueTranslucentSprites(ViewParams* pParams, ObjectDrawList* pDrawList)
{
	d3d_GetVisibleSet()->GetSprites()->Queue(pDrawList, pParams, d3d_DrawSprite, d3d_DrawSpriteEx);
}

void d3d_DrawNoZSprites(ViewParams* pParams)
{
	if (!g_CV_DrawSprites.m_Val)
		return;

	 VisibleSet* pVisibleSet = d3d_GetVisibleSet();

	AllocSet* pSet = pVisibleSet->GetNoZSprites();
	if (pSet->GetObjectCount())
	{
		g_RenderStateMgr.SaveStencilState();
		g_RenderStateMgr.SetStencilState(STENCIL_STATE_NoZ);

		pSet->Draw(pParams, d3d_DrawSprite);

		g_RenderStateMgr.RestoreStencilState();
	}
}