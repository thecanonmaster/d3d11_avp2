#include "pch.h"

#include "d3d_lightmap.h"
#include "common_draw.h"
#include "d3d_device.h"
#include "rendererconsolevars.h"
#include "d3d_texture.h"
#include "d3d_draw.h"

// 0x8000 - lightmap
// 0x4000 - colors
inline uint16 UpdateLMPlane(WorldPoly* pPoly)
{
	// TODO - to fail IsValidLMPlane test?
	return pPoly->m_wLMPlaneAndFlags & 0xFFC0 | 1;
}

static DirectX::XMFLOAT3 g_avLMPlanes[] =
{
	// +X -Z +Y
	{ 1.0f, 0.0f, 0.0f },
	{ 0.0f, 0.0f, -1.0f },
	{ 0.0f, 1.0f, 0.0f },

	// +X +Z -Y
	{ 1.0f, 0.0f, 0.0f },
	{ 0.0f, 0.0f, 1.0f },
	{ 0.0f, -1.0f, 0.0f },

	// +X +Y +Z
	{ 1.0f, 0.0f, 0.0f },
	{ 0.0f, 1.0f, 0.0f },
	{ 0.0f, 0.0f, 1.0f },

	// +X -Y -Z
	{ 1.0f, 0.0f, 0.0f },
	{ 0.0f, -1.0f, 0.0f },
	{ 0.0f, 0.0f, -1.0f },

	// +Z -Y +X
	{ 0.0f, 0.0f, 1.0f },
	{ 0.0f, -1.0f, 0.0f },
	{ 1.0f, 0.0f, 0.0f },

	// -Z -Y -X
	{ 0.0f, 0.0f, -1.0f },
	{ 0.0f, -1.0f, 0.0f },
	{ -1.0f, 0.0f, 0.0f },
};

void SetupLMPlaneVectors(uint32 dwIndex, DirectX::XMFLOAT3* pNormal, DirectX::XMFLOAT3* pOut1,
	DirectX::XMFLOAT3* pOut2)
{
	dwIndex = dwIndex * 3 + 1;
	
	float fTempX = g_avLMPlanes[dwIndex].x;
	float fTempY = g_avLMPlanes[dwIndex].y;
	float fTempZ = g_avLMPlanes[dwIndex].z;

	pOut1->x = fTempY * pNormal->z - fTempZ * pNormal->y;
	pOut1->y = fTempZ * pNormal->x - fTempX * pNormal->z;
	pOut1->z = fTempX * pNormal->y - fTempY * pNormal->x;

	pOut2->x = pNormal->y * pOut1->z - pNormal->z * pOut1->y;
	pOut2->y = pNormal->z * pOut1->x - pNormal->x * pOut1->z;
	pOut2->z = pNormal->x * pOut1->y - pNormal->y * pOut1->x;	
}

static void SimpleBlt(uint32* pDest, uint32 dwPosX, uint32 dwPosY, uint32* pSrc, 
	uint32 dwSrcWidth, uint32 dwSrcHeight)
{
	uint32* pSrcLine = pSrc;
	uint32* pDestLine = pDest + (dwPosY * LIGHTMAP_PAGE_DIMS + dwPosX);

	while (dwSrcHeight)
	{
		dwSrcHeight--;

		memcpy(pDestLine, pSrcLine, dwSrcWidth << 2);

		pSrcLine += dwSrcWidth;
		pDestLine += LIGHTMAP_PAGE_DIMS;
	}
}

static bool DecompressLightmap(uint8* pCompressed, uint32 dwDataLen, uint32* pOut)
{
	if (pCompressed == nullptr || !dwDataLen)
		return false;

	int nBreaker = LIGHTMAP_MAX_DATA_SIZE;
	uint8* pCurrPos = pCompressed;

	while (pCurrPos < pCompressed + dwDataLen)
	{
		uint8 dwCopyCount;
		uint32 dwTag = *(uint32*)pCurrPos;

		pCurrPos += 4;
		if (dwTag & 0x80000000)
		{
			dwTag &= 0x7FFFFFFFu;
			dwCopyCount = *pCurrPos++;
		}
		else
		{
			dwCopyCount = 1;
		}

		nBreaker -= dwCopyCount;
		if (nBreaker < 0)
			return false;

		while (dwCopyCount)
		{
			dwCopyCount--;
			*pOut = dwTag;
			pOut++;
		}
	}

	return true;
}

static bool DecompressShadowMap(uint8* pCompressed, uint32 dwDataLen, uint32* pOut)
{
	if (pCompressed == nullptr || !dwDataLen)
		return false;

	uint8 anMap[2] = { 0, 255 };
	bool bSwitch = false;
	int nBreaker = LIGHTMAP_MAX_DATA_SIZE;

	while (dwDataLen)
	{
		uint32 dwTag = *pCompressed;

		memset(pOut, anMap[bSwitch], dwTag << 2);

		pOut += dwTag;
		bSwitch = !bSwitch;

		nBreaker -= dwTag;
		if (nBreaker < 0)
			return false;

		pCompressed++;
		dwDataLen--;
	}

	return true;
}

void CLightMapManager::FreeAllData()
{
	m_dwAllFrameCount = 0;

	RELEASE_INTERFACE(m_pPages, g_szFreeError_SRV);

	for (auto& kvPagedPoly : m_PagedPolies)
		delete kvPagedPoly.second;

	m_PagedPolies.clear();
}

void CLightMapManager::CreateLightmapPages(LMAnim* pAnims, uint32 dwAnimCount)
{
	float fStartTime = Timer_GetTime();

	CalcAllFrameCount(pAnims, dwAnimCount);

	uint32 dwCurrFrame = 0;
	
	for (uint32 dwAnim = 0; dwAnim < dwAnimCount; dwAnim++)
	{
		LMAnim* pCurrAnim = &pAnims[dwAnim];

		for (uint32 dwFrame = 0; dwFrame < pCurrAnim->m_nFrames; dwFrame++)
		{	
			for (uint32 dwData = 0; dwData < pCurrAnim->m_wPolyRefs; dwData++)
			{
				WorldBsp* pTargetBsp = g_pSceneDesc->m_pRenderContext->m_pMainWorld->GetWorldModelData(
					pCurrAnim->m_pPolyRefs[dwData].m_wModel)->m_pOriginalBsp;

				WorldPoly* pTargetPoly = pTargetBsp->m_pPolies[pCurrAnim->m_pPolyRefs[dwData].m_wPoly];

				LMFramePolyData* pCurrPolyData = &pCurrAnim->m_pFrames[dwFrame].m_pPolyDataList[dwData];

				bool bDecompressResult;
				uint32 adwPixels[LIGHTMAP_MAX_DATA_SIZE];

				if (!pCurrAnim->m_bShadowMap)
				{
					bDecompressResult = 
						DecompressLightmap(pCurrPolyData->m_pData, pCurrPolyData->m_wSize, adwPixels);
				}
				else
				{
					bDecompressResult = 
						DecompressShadowMap(pCurrPolyData->m_pData, pCurrPolyData->m_wSize, adwPixels);
				}

				LMPagedPoly* pPagedPoly = GetPagedPoly(pTargetBsp->m_wIndex, pTargetPoly->m_wIndex);
				pTargetPoly->m_pLMData = pPagedPoly;

				if (pCurrPolyData->m_wSize > 0)
				{
					if (!bDecompressResult)
						AddDebugMessage(0, g_szLMDecompessionError, dwAnim, dwFrame, dwData);

					AddToPage(adwPixels, pTargetPoly->m_nLightmapWidth, pTargetPoly->m_nLightmapHeight, 
						pPagedPoly, dwCurrFrame);
				}
			}

			dwCurrFrame++;
		}
	}

	m_pPages = CreatePages();

	if (m_pPages == nullptr)
		AddDebugMessage(0, g_szLMCreatePagesError);

	RemoveTempData();

	AddConsoleMessage("Lightmaps paged in %f seconds", Timer_GetTime() - fStartTime);
}

void CLightMapManager::Init()
{
	m_bInitialized = true;
}

void CLightMapManager::Term()
{
	m_bInitialized = false;
	FreeAllData();
}

ID3D11ShaderResourceView* CLightMapManager::CreateLMVertexDataBuffer(void* pData, uint32 dwSize)
{
	D3D11_BUFFER_DESC desc = { };

	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.ByteWidth = dwSize * sizeof(LMVertexData);
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	desc.StructureByteStride = sizeof(LMVertexData);

	D3D11_SUBRESOURCE_DATA subData;

	subData.pSysMem = pData;
	subData.SysMemPitch = 0;
	subData.SysMemSlicePitch = 0;

	ID3D11Buffer* pBuffer;
	HRESULT hResult = g_D3DDevice.GetDevice()->CreateBuffer(&desc, &subData, &pBuffer);
	if (FAILED(hResult))
		return nullptr;

	D3D11_SHADER_RESOURCE_VIEW_DESC resourceDesc = { };
	resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	resourceDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	resourceDesc.Buffer.FirstElement = 0;
	resourceDesc.Buffer.NumElements = dwSize;

	ID3D11ShaderResourceView* pResourceView;
	hResult = g_D3DDevice.GetDevice()->CreateShaderResourceView(pBuffer, &resourceDesc, &pResourceView);
	if (FAILED(hResult))
	{
		pBuffer->Release();
		return nullptr;
	}

	pBuffer->Release();

	return pResourceView;
}

DirectX::XMFLOAT2 CLightMapManager::SetupBaseLMVertexTexCoords(LMPagedPoly* pPagedPoly, WorldPoly* pPoly, 
	uint32 dwVert, DirectX::XMFLOAT3* pLMVectorU, DirectX::XMFLOAT3* pLMVectorV)
{
	LTVector& vPoint = pPoly->m_Vertices[dwVert].m_pPoints->m_vVec;

	float fDiffX = vPoint.x - pPoly->m_vLMCenter.x;
	float fDiffY = vPoint.y - pPoly->m_vLMCenter.y;
	float fDiffZ = vPoint.z - pPoly->m_vLMCenter.z;

	constexpr float c_fTexelSize = 1.0f / (float)LIGHTMAP_PAGE_DIMS;
	float fGridSize = g_pSceneDesc->m_pRenderContext->m_pMainWorld->m_fLMGridSize;

	DirectX::XMFLOAT2 vResult;	

	vResult.x = ((fDiffZ * pLMVectorU->z + fDiffX * pLMVectorU->x + fDiffY * pLMVectorU->y) / fGridSize +
		0.5f) * c_fTexelSize;
	vResult.y = ((fDiffZ * pLMVectorV->z + fDiffX * pLMVectorV->x + fDiffY * pLMVectorV->y) / fGridSize +
		0.5f) * c_fTexelSize;

	return vResult;
}

void CLightMapManager::CalcAllFrameCount(LMAnim* pAnims, uint32 dwAnimCount)
{
	uint32 dwPagedPolyCount = 0;
	
	for (uint32 dwAnim = 0; dwAnim < dwAnimCount; dwAnim++)
	{
		LMAnim* pCurrAnim = &pAnims[dwAnim];

		for (uint32 dwFrame = 0; dwFrame < pCurrAnim->m_nFrames; dwFrame++)
			m_dwAllFrameCount++;
	}
}

LMPagedPoly* CLightMapManager::GetPagedPoly(uint16 wBspIndex, uint16 wPolyIndex)
{
	uint32 dwKey = (wBspIndex << 16 | wPolyIndex);
	Map_PLMPagedPoly::iterator iter = m_PagedPolies.find(dwKey);

	if (iter == m_PagedPolies.end())
	{
		LMPagedPoly* pNewPoly = new LMPagedPoly(wBspIndex, wPolyIndex, m_dwAllFrameCount);
		m_PagedPolies[dwKey] = pNewPoly;
		
		return pNewPoly;
	}
	else
	{
		return (*iter).second;
	}
}

void CLightMapManager::AddToPage(uint32* pPixels, uint32 dwWidth, uint32 dwHeight, LMPagedPoly* pPagedPoly,
	uint32 dwFrame)
{
	for (uint32 i = 0; i < m_aTempPage.size(); i++)
	{
		LMTempPage& currPageInfo = m_aTempPage[i];
		uint32 dwPosX = currPageInfo.m_dwNextPosX;
		uint32 dwPosY = currPageInfo.m_dwNextPosY;
		uint32 dwRowHeight = currPageInfo.m_dwRowHeight;

		if (dwPosX + dwWidth > LIGHTMAP_PAGE_DIMS)
		{
			dwPosX = 0;
			dwPosY += dwRowHeight;
			dwRowHeight = 0;
		}

		if (dwPosY + dwHeight > LIGHTMAP_PAGE_DIMS)
			continue;

		pPagedPoly->SetFrameInfo(dwFrame, i, (float)dwPosX / LIGHTMAP_PAGE_DIMS_F,
			(float)dwPosY / LIGHTMAP_PAGE_DIMS_F);

		SimpleBlt(currPageInfo.m_pPixels, dwPosX, dwPosY, pPixels, dwWidth, dwHeight);

		currPageInfo.Init(dwPosX + dwWidth, dwPosY, dwHeight > dwRowHeight ? dwHeight : dwRowHeight);

		return;
	}

	m_aTempPage.emplace_back();
	LMTempPage& newPageInfo = m_aTempPage.back();
	newPageInfo.Init(dwWidth, 0, dwHeight);
	newPageInfo.InitPixels();

	pPagedPoly->SetFrameInfo(dwFrame, m_aTempPage.size() - 1, 0.0f, 0.0f);

	SimpleBlt(newPageInfo.m_pPixels, 0, 0, pPixels, dwWidth, dwHeight);
}

void CLightMapManager::RemoveTempData()
{
	for (LMTempPage& tempPage : m_aTempPage)
		tempPage.TermPixels();

	Array_LMTempPage aEmpty;
	m_aTempPage.swap(aEmpty);
}

ID3D11ShaderResourceView* CLightMapManager::CreatePages()
{
	D3D11_TEXTURE2D_DESC texDesc = { };

	texDesc.Width = LIGHTMAP_PAGE_DIMS;
	texDesc.Height = LIGHTMAP_PAGE_DIMS;
	texDesc.MipLevels = 1;

	texDesc.ArraySize = m_aTempPage.size();
	texDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_IMMUTABLE;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA* pTexData = new D3D11_SUBRESOURCE_DATA[texDesc.ArraySize];
	for (uint32 i = 0; i < texDesc.ArraySize; i++)
	{
		pTexData[i].pSysMem = m_aTempPage[i].m_pPixels;
		pTexData[i].SysMemPitch = texDesc.Width << 2;
		pTexData[i].SysMemSlicePitch = 0;
	}

	ID3D11Texture2D* pD3DTexture;
	HRESULT hResult = g_D3DDevice.GetDevice()->CreateTexture2D(&texDesc, pTexData, &pD3DTexture);

	delete [] pTexData;

	if (FAILED(hResult))
		return nullptr;

	D3D11_SHADER_RESOURCE_VIEW_DESC resourceDesc = { };
	resourceDesc.Format = texDesc.Format;
	resourceDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
	resourceDesc.Texture2DArray.ArraySize = texDesc.ArraySize;
	resourceDesc.Texture2DArray.MipLevels = texDesc.MipLevels;

	ID3D11ShaderResourceView* pResourceView;
	hResult = g_D3DDevice.GetDevice()->CreateShaderResourceView(pD3DTexture, &resourceDesc, &pResourceView);
	if (FAILED(hResult))
	{
		pD3DTexture->Release();
		return nullptr;
	}

	// TEST
	//for (uint32 i = 0; i < m_aTempPage.size(); i++)
	//	d3d_SaveTextureAsTGA(("lightmap_page" + std::to_string(i) + ".tga").c_str(), pD3DTexture, i, true);

	pD3DTexture->Release();

	return pResourceView;
}
