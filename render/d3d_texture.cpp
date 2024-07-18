#include "pch.h"

#include "d3d_texture.h"
#include "d3d_device.h"
#include "common_init.h"
#include "d3d_utils.h"
#include "globalmgr.h"
#include "conparse.h"
#include "rendererconsolevars.h"

static bool ShouldFreeSystemTexture(SharedTexture* pSharedTexture)
{
	if (g_CV_CacheTextures.m_Val)
		return false;

	if (pSharedTexture->m_pFile == nullptr)
		return false;

	return true;
}

static RTexture* d3d_CreateAndLoadTexture(SharedTexture* pSharedTexture)
{
	RTexture* pTexture = nullptr;

	TextureData* pTextureData = g_pStruct->GetTexture(pSharedTexture);
	if (pTextureData)
	{
		CTextureManager* pTextureMgr = g_GlobalMgr.GetTextureManager();

		pTexture = pTextureMgr->CreateRTexture(pSharedTexture, pTextureData);

		if (pTexture)
		{
			if (!pTextureMgr->UploadRTexture(pTexture, pTextureData))
			{
				AddDebugMessage(0, "Unable to transfer texture data to video memory.");
				pTextureMgr->FreeTexture(pTexture);

				pTexture = nullptr;
			}
			else
			{
				if (ShouldFreeSystemTexture(pSharedTexture))
					g_pStruct->FreeTexture(pSharedTexture);
			}
		}
	}

	return pTexture;
}

void d3d_BindTexture(SharedTexture* pSharedTexture, LTBOOL bTextureChanged)
{
	if (!pSharedTexture)
		return;

	if (pSharedTexture->m_pRenderData)
	{
		if (bTextureChanged)
		{
			RTexture* pTexture = (RTexture*)pSharedTexture->m_pRenderData;
			TextureData* pTextureData = g_pStruct->GetTexture(pSharedTexture);

			if (pTextureData)
			{
				if (g_GlobalMgr.GetTextureManager()->UploadRTexture(pTexture, pTextureData))
				{
					if (ShouldFreeSystemTexture(pSharedTexture))
						g_pStruct->FreeTexture(pSharedTexture);
				}
				else
				{
					AddDebugMessage(0, "Unable to transfer texture data to video memory");
				}
			}
		}
	}
	else
	{
		d3d_CreateAndLoadTexture(pSharedTexture);
	}
}

void d3d_UnbindTexture(SharedTexture* pSharedTexture)
{
	if (pSharedTexture->m_pRenderData)
	{
		RTexture* pTexture = (RTexture*)pSharedTexture->m_pRenderData;
		g_GlobalMgr.GetTextureManager()->FreeTexture(pTexture);
	}
}

void CTextureManager::Init()
{
	m_bInitialized = true;
}

void CTextureManager::Term()
{
	FreeAllTextures();

	m_bInitialized = false;
}

RTexture* CTextureManager::CreateRTexture(SharedTexture* pSharedTexture, TextureData* pTextureData)
{
	if (!m_bInitialized) 
		return nullptr;

	uint32 dwFlags = 0;

	if (pTextureData->m_DtxHeader.m_nIFlags & DTX_FULLBRITE)
		dwFlags |= RT_FULLBRITE;

	uint32 dwBPP = pTextureData->m_DtxHeader.GetBPPIdent();
	uint32 dwFormat = CTextureManager::QueryDXFormat(dwBPP, pTextureData->m_DtxHeader.m_nIFlags);
	float fDetailTextureAngle = MATH_DEGREES_TO_RADIANS((float)(pTextureData->m_DtxHeader.m_Extra.m_nDetTexAngle));

	RTexture* pTexture = new RTexture();

	pTexture->m_dwFormat = dwFormat;
	pTexture->m_dwFlags = dwFlags;
	pTexture->m_dwBaseWidth = pTextureData->m_Mips[0].m_dwWidth;
	pTexture->m_dwBaseHeight = pTextureData->m_Mips[0].m_dwHeight;
	pTexture->m_dwScaledWidth = pTexture->m_dwBaseWidth >> pTextureData->m_pSharedTexture->m_dwMipMapOffsetED;
	pTexture->m_dwScaledHeight = pTexture->m_dwBaseHeight >> pTextureData->m_pSharedTexture->m_dwMipMapOffsetED;
	pTexture->m_fDetailTextureScale = pTextureData->m_DtxHeader.m_Extra.m_fDetTexScale + 1.0f;
	pTexture->m_fDetailTextureAngleC = cosf(fDetailTextureAngle);
	pTexture->m_fDetailTextureAngleS = sinf(fDetailTextureAngle);

	ConParse parser;
	parser.Init(pTextureData->m_DtxHeader.m_szCommandString);
	constexpr float c_fThreshold = 0.5f / 255.0f;

	if (parser.ParseFind((char*)"AlphaRef", false, 1))
		pTexture->m_fAlphaRef = (float)atoi(parser.m_pArgs[1]) / 255.0f + c_fThreshold;
	else
		pTexture->m_fAlphaRef = 0.0f;

	parser.Init(pTextureData->m_DtxHeader.m_szCommandString);
	if (parser.ParseFind((char*)"ColorKey", false, 3))
	{
		pTexture->m_anColorKey[0] = atoi(parser.m_pArgs[1]);
		pTexture->m_anColorKey[1] = atoi(parser.m_pArgs[2]);
		pTexture->m_anColorKey[2] = atoi(parser.m_pArgs[3]);
	}

	pTexture->m_pSharedTexture = pSharedTexture;
	pSharedTexture->m_pRenderData = pTexture;

	m_Textures.push_back(pTexture);
	pTexture->m_GlobalIter = --m_Textures.end();

	return pTexture;
}

bool CTextureManager::UploadRTexture(RTexture* pTexture, TextureData* pTextureData)
{
	D3D11_TEXTURE2D_DESC texDesc = { };

	texDesc.Width = pTexture->m_dwBaseWidth;
	texDesc.Height = pTexture->m_dwBaseHeight;

	if (g_CV_UseMipMapsInUse.m_Val)
	{
		DtxExtra& dtxExtra = pTextureData->m_DtxHeader.m_Extra;

		if (!dtxExtra.m_nMipMapsInUse || dtxExtra.m_nMipMapsInUse > pTextureData->m_DtxHeader.m_wMipmaps)
			texDesc.MipLevels = pTextureData->m_DtxHeader.m_wMipmaps;
		else
			texDesc.MipLevels = pTextureData->m_DtxHeader.m_Extra.m_nMipMapsInUse;
	}
	else
	{
		texDesc.MipLevels = pTextureData->m_DtxHeader.m_wMipmaps;
	}

	texDesc.ArraySize = 1;
	texDesc.Format = (DXGI_FORMAT)pTexture->m_dwFormat;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_IMMUTABLE;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA aMipMapData[MAX_DTX_MIPMAPS] = { };
	for (uint32 i = 0; i < texDesc.MipLevels; i++)
	{
		if (pTextureData->m_Mips[i].m_pData == nullptr)
			Sleep(0);
		
		aMipMapData[i].pSysMem = pTextureData->m_Mips[i].m_pData;
		aMipMapData[i].SysMemPitch = GetTexturePitch(pTexture->m_dwFormat, pTextureData->m_Mips[i].m_dwWidth);
		aMipMapData[i].SysMemSlicePitch = 0;
	}

	ID3D11Texture2D* pD3DTexture;
	HRESULT hResult = g_D3DDevice.GetDevice()->CreateTexture2D(&texDesc, aMipMapData, &pD3DTexture);

	if (FAILED(hResult))
		return false;

	D3D11_SHADER_RESOURCE_VIEW_DESC resourceDesc = { };
	resourceDesc.Format = texDesc.Format;

	resourceDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	resourceDesc.Texture2D.MipLevels = texDesc.MipLevels; // !pTextureData->m_DtxHeader.m_wMipmaps ? -1 : texDesc.MipLevels;

	ID3D11ShaderResourceView* pResourceView;
	hResult = g_D3DDevice.GetDevice()->CreateShaderResourceView(pD3DTexture, &resourceDesc, &pResourceView);
	if (FAILED(hResult))
	{
		pD3DTexture->Release();
		return false;
	}

	pTexture->m_pResourceView = pResourceView;
	pD3DTexture->Release();

	return true;
}

void CTextureManager::FreeAllTextures()
{
	for (RTexture* pTexture : m_Textures)
		FreeTexture(pTexture);
}

void CTextureManager::FreeTexture(RTexture* pTexture)
{
	if (pTexture->m_pSharedTexture)
	{
		pTexture->m_pSharedTexture->m_pRenderData = nullptr;
		pTexture->m_pSharedTexture = nullptr;
	}

	RELEASE_INTERFACE(pTexture->m_pResourceView, g_szFreeError_SRV);
	
	m_Textures.erase(pTexture->m_GlobalIter);
	delete pTexture;
}

uint32 CTextureManager::QueryDXFormat(uint32 dwBPP, uint32 dwFlags)
{
	if (IsS3TCFormat(dwBPP))
		return S3TCFormatConv_BPP(dwBPP);

	return g_D3DDevice.GetModeInfo()->m_dwFormat;
}
