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

using namespace DirectX;

#define SPRITE_ZBIAS			4
#define SPRITE_POSITION_ZBIAS	-20.0f

#define SPRITE_MINFACTORDIST	10.0f
#define SPRITE_MAXFACTORDIST	500.0f
#define SPRITE_MINFACTOR		0.1f
#define SPRITE_MAXFACTOR		2.0f

CSpriteData::~CSpriteData()
{
	if (m_pVertexBuffer)
	{
		uint32 dwRefCount = m_pVertexBuffer->Release();
		if (dwRefCount)
			AddDebugMessage(0, g_szFreeError_VB, dwRefCount);

		m_pVertexBuffer = nullptr;
	}
}

bool CSpriteData::CreateVertexBuffer()
{
	InternalSpriteVertex aVertices[SPRITE_VERTICES];

	for (int i = 0; i < SPRITE_VERTICES; i++)
	{
		aVertices[i].vPosition = m_aData[i].m_vPos;

		aVertices[i].vDiffuseColor.x = RGBA_GETFR(m_aData[i].m_dwColor) / 255.0f;
		aVertices[i].vDiffuseColor.y = RGBA_GETFG(m_aData[i].m_dwColor) / 255.0f;
		aVertices[i].vDiffuseColor.z = RGBA_GETFB(m_aData[i].m_dwColor) / 255.0f;
		aVertices[i].vDiffuseColor.w = RGBA_GETFA(m_aData[i].m_dwColor) / 255.0f;
		
		aVertices[i].vTexCoords.x = m_aData[i].m_fU;
		aVertices[i].vTexCoords.y = m_aData[i].m_fV;
	}

	D3D11_BUFFER_DESC desc = { };

	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.ByteWidth = sizeof(aVertices);
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.MiscFlags = 0;
	desc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA vertexData;

	vertexData.pSysMem = aVertices;
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;

	HRESULT hResult = g_D3DDevice.GetDevice()->CreateBuffer(&desc, &vertexData, &m_pVertexBuffer);
	if (FAILED(hResult))
		return false;

	return true;
}

bool CSpriteData::UpdateVertexBuffer(CSpriteVertex* pNewVerts)
{
	if (!CompareTo(pNewVerts))
	{
		D3D11_MAPPED_SUBRESOURCE subResource;
		HRESULT hResult = g_D3DDevice.GetDeviceContext()->Map(m_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &subResource);
		if (FAILED(hResult))
			return false;

		InternalSpriteVertex* pVertices = (InternalSpriteVertex*)subResource.pData;

		for (int i = 0; i < SPRITE_VERTICES; i++)
		{
			pVertices[i].vPosition = pNewVerts[i].m_vPos;

			pVertices[i].vDiffuseColor.x = RGBA_GETFR(pNewVerts[i].m_dwColor) / 255.0f;
			pVertices[i].vDiffuseColor.y = RGBA_GETFG(pNewVerts[i].m_dwColor) / 255.0f;
			pVertices[i].vDiffuseColor.z = RGBA_GETFB(pNewVerts[i].m_dwColor) / 255.0f;
			pVertices[i].vDiffuseColor.w = RGBA_GETFA(pNewVerts[i].m_dwColor) / 255.0f;

			pVertices[i].vTexCoords.x = pNewVerts[i].m_fU;
			pVertices[i].vTexCoords.y = pNewVerts[i].m_fV;
		}

		g_D3DDevice.GetDeviceContext()->Unmap(m_pVertexBuffer, 0);

		memcpy(m_aData, pNewVerts, sizeof(CSpriteVertex) * SPRITE_VERTICES);
	}

	return true;
}

bool CSpriteData::IsExpired()
{
	if (g_fLastClientTime - m_fLastTimeDrawn > g_CV_SpriteVertexBufferTTL.m_Val)
		return true;

	return false;
}

#ifdef SPRITE_DATA_EXTENDED
CSpriteData* CSpriteManager::GetSpriteData(CSpriteVertex* pVerts)
{
	auto iter = m_Data.find(*(CSpriteVerts*)(pVerts));

	if (iter != m_Data.end())
	{
		CSpriteData* pData = (*iter).second;
		pData->SetLastTimeDrawn(g_fLastClientTime);
		return pData;
	}

	CSpriteData* pNewData = new CSpriteData();
	memcpy(pNewData->m_aData, pVerts, sizeof(CSpriteVertex) * SPRITE_VERTICES);

	if (!pNewData->CreateVertexBuffer())
	{
		delete pNewData;
		return nullptr;
	}

	pNewData->SetLastTimeDrawn(g_fLastClientTime);
	m_Data[*(CSpriteVerts*)pNewData->m_aData] = pNewData;

	return pNewData;
}
#else
CSpriteData* CSpriteManager::GetSpriteData(SpriteInstance* pInstance, CSpriteVertex* pVerts)
{
	auto iter = m_Data.find(pInstance);

	if (iter != m_Data.end())
	{
		CSpriteData* pData = (*iter).second;

		pData->UpdateVertexBuffer(pVerts);
		pData->SetLastTimeDrawn(g_fLastClientTime);

		return pData;
	}

	CSpriteData* pNewData = new CSpriteData();
	memcpy(pNewData->m_aData, pVerts, sizeof(CSpriteVertex) * SPRITE_VERTICES);

	if (!pNewData->CreateVertexBuffer())
	{
		delete pNewData;
		return nullptr;
	}

	pNewData->SetLastTimeDrawn(g_fLastClientTime);
	m_Data[pInstance] = pNewData;

	return pNewData;
}
#endif

static void d3d_GetSpriteColor(LTObject* pObject, RGBColor* pColor)
{
	pColor->rgb.a = pObject->m_nColorA;

	if ((pObject->m_dwFlags & FLAG_NOLIGHT) || !g_bHaveWorld || (!g_CV_DynamicLightSprites.m_Val))
	{
		pColor->rgb.r = pObject->m_nColorR;
		pColor->rgb.g = pObject->m_nColorG;
		pColor->rgb.b = pObject->m_nColorB;
	}
	else
	{
		// TODO - query light table
	}
}

static void d3d_RenderSprite(CSpriteData* pData)
{
	UINT dwStride = sizeof(InternalSpriteVertex);
	UINT dwOffset = 0;

	g_D3DDevice.GetDeviceContext()->IASetVertexBuffers(0, 1, &pData->m_pVertexBuffer, &dwStride, &dwOffset);
	g_D3DDevice.GetDeviceContext()->IASetIndexBuffer(g_GlobalManager.GetIndexBuffer_Sprite(), DXGI_FORMAT_R16_UINT, 0);
	g_D3DDevice.GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

static void d3d_DrawRotatableSprite(ViewParams* pParams, SpriteInstance* pInstance, SharedTexture* pSharedTexture, bool bReallyClose)
{
	float fWidth = (float)((RTexture*)pSharedTexture->m_pRenderData)->m_dwBaseWidth;
	float fHeight = (float)((RTexture*)pSharedTexture->m_pRenderData)->m_dwBaseHeight;

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

	RGBColor color;
	d3d_GetSpriteColor(&pInstance->m_Base, &color);
	uint32 dwColor = color.color;

	CSpriteVertex spriteVerts[SPRITE_VERTICES];
	DirectX::XMStoreFloat3(&spriteVerts[0].m_vPos, vPos + vUp - vRight);
	spriteVerts[0].SetupVertColorAndUVs(dwColor, 0.0f, 0.0f);

	DirectX::XMStoreFloat3(&spriteVerts[1].m_vPos, vPos + vUp + vRight);
	spriteVerts[1].SetupVertColorAndUVs(dwColor, 1.0f, 0.0f);

	DirectX::XMStoreFloat3(&spriteVerts[2].m_vPos, vPos + vRight - vUp);
	spriteVerts[2].SetupVertColorAndUVs(dwColor, 1.0f, 1.0f);

	DirectX::XMStoreFloat3(&spriteVerts[3].m_vPos, vPos - vRight - vUp);
	spriteVerts[3].SetupVertColorAndUVs(dwColor, 0.0f, 1.0f);

	CSpriteVertex* pPoints;
	uint32 dwPoints;
	//CSpriteVertex clippedSpriteVerts[CLIPPED_SPRITE_VERTICES];

	// TODO - clip function
	/*if (pInstance->m_hClipperPoly != INVALID_HPOLY)
	{
		
		if (!d3d_ClipSprite(pInstance, pInstance->m_ClipperPoly, &pPoints, &dwPoints, clippedSpriteVerts))
		{
			return;
		}
	}
	else*/
	{
		pPoints = spriteVerts;
		dwPoints = SPRITE_VERTICES;
	}

	if ((pInstance->m_Base.m_dwFlags & FLAG_SPRITEBIAS) && !bReallyClose)
	{
		DirectX::XMVECTOR vParamsPos = DirectX::XMLoadFloat3(&pParams->m_vPos);
		DirectX::XMVECTOR vParamsRight = DirectX::XMLoadFloat3(&pParams->m_vForward);
		DirectX::XMVECTOR vParamsUp = DirectX::XMLoadFloat3(&pParams->m_vUp);
		DirectX::XMVECTOR vParamsForward = DirectX::XMLoadFloat3(&pParams->m_vForward);
		
		for (uint32 dwCurrPt = 0; dwCurrPt < dwPoints; dwCurrPt++)
		{
			DirectX::XMVECTOR vPt = DirectX::XMLoadFloat3(&pPoints[dwCurrPt].m_vPos);
			DirectX::XMVECTOR vPtRelCamera = vPt - vParamsPos;

			float fZ = DirectX::XMVectorGetX(DirectX::XMVector3Dot(vPtRelCamera, vParamsForward));
			float fNearZ = g_CV_NearZ.m_Val;

			if (fZ <= fNearZ)
				continue;

			float fBiasDist = SPRITE_POSITION_ZBIAS;
			if ((fZ + fBiasDist) < fNearZ)
				fBiasDist = fNearZ - fZ;

			float fScale = 1.0f + fBiasDist / fZ;

			vPt = vParamsRight * DirectX::XMVector3Dot(vPtRelCamera, vParamsRight) * fScale +
				vParamsUp * DirectX::XMVector3Dot(vPtRelCamera, vParamsUp) * fScale +
				(fZ + fBiasDist) * vParamsForward + vParamsPos;

			DirectX::XMStoreFloat3(&pPoints[dwCurrPt].m_vPos, vPt);
		}
	}

#ifdef SPRITE_DATA_EXTENDED
	CSpriteData* pSpriteData = g_SpriteManager.GetSpriteData(pPoints);
#else
	CSpriteData* pSpriteData = g_SpriteManager.GetSpriteData(pInstance, pPoints);
#endif

	if (pSpriteData == nullptr)
		return;

	d3d_RenderSprite(pSpriteData);

	CRenderShader_Sprite* pRenderShader = g_RenderShaderMgr.GetRenderShader<CRenderShader_Sprite>();

	if (!pRenderShader->SetPerObjectParams(((RTexture*)pSharedTexture->m_pRenderData)->m_pResourceView, bReallyClose))
		return;

	pRenderShader->Render();
}

static void d3d_DrawSprite(ViewParams* pParams, SpriteInstance* pInstance, SharedTexture* pSharedTexture, bool bReallyClose)
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

	if (!bReallyClose && (fZ < g_CV_NearZ.m_Val))
		return;

	float fWidth = (float)((RTexture*)pSharedTexture->m_pRenderData)->m_dwBaseWidth;
	float fHeight = (float)((RTexture*)pSharedTexture->m_pRenderData)->m_dwBaseHeight;

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

	RGBColor color;
	d3d_GetSpriteColor(&pInstance->m_Base, &color);
	uint32 dwColor = color.color;

	DirectX::XMVECTOR vRight = vBasisRight * fSizeX;
	DirectX::XMVECTOR vUp = vBasisUp * fSizeY;

	CSpriteVertex spriteVerts[SPRITE_VERTICES];
	DirectX::XMStoreFloat3(&spriteVerts[0].m_vPos, vPos + vUp - vRight);
	spriteVerts[0].SetupVertColorAndUVs(dwColor, 0.0f, 0.0f);

	DirectX::XMStoreFloat3(&spriteVerts[1].m_vPos, vPos + vUp + vRight);
	spriteVerts[1].SetupVertColorAndUVs(dwColor, 1.0f, 0.0f);

	DirectX::XMStoreFloat3(&spriteVerts[2].m_vPos, vPos + vRight - vUp);
	spriteVerts[2].SetupVertColorAndUVs(dwColor, 1.0f, 1.0f);

	DirectX::XMStoreFloat3(&spriteVerts[3].m_vPos, vPos - vRight - vUp);
	spriteVerts[3].SetupVertColorAndUVs(dwColor, 0.0f, 1.0f);

	if ((pInstance->m_Base.m_dwFlags & FLAG_SPRITEBIAS) && !bReallyClose)
	{
		float fBiasDist = SPRITE_POSITION_ZBIAS;

		if ((fZ + fBiasDist) < g_CV_NearZ.m_Val)
			fBiasDist = g_CV_NearZ.m_Val - fZ;

		float fScale = 1.0f + fBiasDist / fZ;

		for (uint32 dwCurrPt = 0; dwCurrPt < SPRITE_VERTICES; dwCurrPt++)
		{
			DirectX::XMVECTOR vPt = DirectX::XMLoadFloat3(&spriteVerts[dwCurrPt].m_vPos);

			DirectX::XMVECTOR vDotRight = DirectX::XMVector3Dot(vPt - vBasisPos, vBasisRight);
			DirectX::XMVECTOR vDotUp = DirectX::XMVector3Dot(vPt - vBasisPos, vBasisUp);

			vPt = vBasisRight * vDotRight * fScale + vBasisUp * vDotUp * fScale +
				(fZ + fBiasDist) * vBasisForward + vBasisPos;

			DirectX::XMStoreFloat3(&spriteVerts[dwCurrPt].m_vPos, vPt);
		}
	}

#ifdef SPRITE_DATA_EXTENDED
	CSpriteData* pSpriteData = g_SpriteManager.GetSpriteData(spriteVerts);
#else
	CSpriteData* pSpriteData = g_SpriteManager.GetSpriteData(pInstance, spriteVerts);
#endif

	if (pSpriteData == nullptr)
		return;

	d3d_RenderSprite(pSpriteData);
	
	CRenderShader_Sprite* pRenderShader = g_RenderShaderMgr.GetRenderShader<CRenderShader_Sprite>();

	if (!pRenderShader->SetPerObjectParams(((RTexture*)pSharedTexture->m_pRenderData)->m_pResourceView, bReallyClose))
		return;

	pRenderShader->Render();
}

void d3d_DrawSprite(ViewParams* pParams, LTObject* pObj)
{
	SpriteInstance* pInstance = (SpriteInstance*)pObj;
	SpriteEntry* pEntry = pInstance->m_SpriteTracker.m_pCurFrame;
	SpriteAnim* pAnim = pInstance->m_SpriteTracker.m_pCurAnim;

	// TODO - render with no texture?
	if (pAnim != nullptr && pEntry != nullptr && pEntry->m_pTexture != nullptr)
	{
		bool bFogEnabled;
		uint32 dwFogColor;
		HRENDERSTATE hBlendState;

		g_RenderStateMgr.SaveBlendState();
		g_RenderStateMgr.SaveRasterState();

		d3d_GetBlendAndFogStates(pObj, hBlendState, bFogEnabled, dwFogColor);

		g_RenderStateMgr.SetBlendState(hBlendState);
		g_RenderStateMgr.SetRasterState(!g_CV_Wireframe.m_Val ? RASTER_STATE_DEFAULT : RASTER_STATE_WIREFRAME);

		bool bMultiply = (pObj->m_dwFlags2 & FLAG2_MULTIPLY) && !(pObj->m_dwFlags2 & FLAG2_ADDITIVE);
		
		if (bMultiply)
		{
			// TODO - not needed?
		}

		bool bReallyClose = false;
		CReallyCloseData reallyCloseData;
		if (pObj->m_dwFlags & FLAG_REALLYCLOSE)
		{
			bReallyClose = true;
			d3d_SetReallyClose(&reallyCloseData, 0.0f, 0.0f);
		}

		if (pObj->m_dwFlags & FLAG_ROTATEABLESPRITE)
			d3d_DrawRotatableSprite(pParams, pInstance, pEntry->m_pTexture, bReallyClose);
		else
			d3d_DrawSprite(pParams, pInstance, pEntry->m_pTexture, bReallyClose);

		if (bReallyClose)
			d3d_UnsetReallyClose(&reallyCloseData);

		if (bMultiply)
		{
			// TODO - not needed?
		}

		g_RenderStateMgr.RestoreBlendState();
		g_RenderStateMgr.RestoreRasterState();
	}
}

void d3d_ProcessSprite(LTObject* pObject)
{
	if (!g_CV_DrawSprites)
		return;

	VisibleSet* pVisibleSet = d3d_GetVisibleSet();
	AllocSet* pSet = (pObject->m_dwFlags & FLAG_SPRITE_NOZ) ? pVisibleSet->GetNoZSprites() : pVisibleSet->GetSprites();

	pSet->Add(pObject);
}

void d3d_QueueTranslucentSprites(ViewParams* pParams, ObjectDrawList* pDrawList)
{
	d3d_GetVisibleSet()->GetSprites()->Queue(pDrawList, pParams, d3d_DrawSprite);
}

void d3d_DrawNoZSprites(ViewParams* pParams)
{
	 VisibleSet* pVisibleSet = d3d_GetVisibleSet();

	AllocSet* pSet = pVisibleSet->GetNoZSprites();
	if (pSet->GetObjectCount())
	{
		g_RenderStateMgr.SaveStencilState();
		g_RenderStateMgr.SetStencilState(STENCIL_STATE_NOZ);

		pSet->Draw(pParams, d3d_DrawSprite);

		g_RenderStateMgr.RestoreStencilState();
	}
}