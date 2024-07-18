#include "pch.h"

#include "globalmgr.h"
#include "d3d_device.h"
#include "d3d_optimizedsurface.h"
#include "common_stuff.h"
#include "d3d_draw.h"
#include "rendererconsolevars.h"
#include "common_draw.h"
#include "crc32.h"
#include <resource.h>
#include "conparse.h"

using namespace DirectX;

CGlobalManager g_GlobalMgr;

DirectX::XMFLOAT4X4 g_mIdentity = { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f };

static uint16 g_awIndex_Opt2D[OPTIMIZED_SURFACE_INDICES] = { 0, 1, 2, 0, 3, 1 };
static uint16 g_awIndex_Sprite[SPRITE_MAX_INDICES] = { 0, 1, 2, 0, 2, 3 };
static uint16 g_awIndex_SpriteEx[SPRITE_MAX_INDICES] = { 0, 1, 2, 0, 2, 3, 0, 3, 4, 0, 4, 5, 0, 5, 6, 0, 6, 7 };

CGlobalManager::CGlobalManager()
{
	m_pTextureMgr = new CTextureManager();

	m_pModelMgr = new CModelManager();
	m_pWorldModelMgr = new CWorldModelManager();
	m_pSpriteMgr = new CSpriteManager();
	m_pParticleSystemMgr = new CParticleSystemManager();
	m_pCanvasMgr = new CCanvasManager();
	m_pPolyGridMgr = new CPolyGridManager();
	m_pLineSystemMgr = new CLineSystemManager();

	m_pLightMapMgr = new CLightMapManager();

	m_pOptimizedSurfaceBatchMgr = new COptimizedSurfaceBatchManager();
	m_pCanvasBatchMgr = new CCanvasBatchManager();
	m_pSpriteBatchMgr = new CSpriteBatchManager();

#ifdef WORLD_TWEAKS_ENABLED
	m_pWorldTweakMgr = new CWorldTweakManager();
#endif

	m_pVertexBuffer_Line = nullptr;

	m_pIndexBuffer_Opt2D = nullptr;
	m_pIndexBuffer_Sprite = nullptr;
	m_pIndexBuffer_SpriteEx = nullptr;

	m_sEngineHacks.Init();

	m_eSolidDrawingMode = SDM_SingleMenuFastGame;
	m_eQueueObjectsMode = QOM_VisQuery;
}

CGlobalManager::~CGlobalManager()
{
	if (m_pTextureMgr != nullptr)
	{
		delete m_pTextureMgr;
		m_pTextureMgr = nullptr;
	}

	if (m_pModelMgr != nullptr)
	{
		delete m_pModelMgr;
		m_pModelMgr = nullptr;
	}

	if (m_pWorldModelMgr != nullptr)
	{
		delete m_pWorldModelMgr;
		m_pWorldModelMgr = nullptr;
	}

	if (m_pSpriteMgr != nullptr)
	{
		delete m_pSpriteMgr;
		m_pSpriteMgr = nullptr;
	}

	if (m_pParticleSystemMgr != nullptr)
	{
		delete m_pParticleSystemMgr;
		m_pParticleSystemMgr = nullptr;
	}

	if (m_pCanvasMgr != nullptr)
	{
		delete m_pCanvasMgr;
		m_pCanvasMgr = nullptr;
	}

	if (m_pPolyGridMgr != nullptr)
	{
		delete m_pPolyGridMgr;
		m_pPolyGridMgr = nullptr;
	}

	if (m_pLineSystemMgr != nullptr)
	{
		delete m_pLineSystemMgr;
		m_pLineSystemMgr = nullptr;
	}

	if (m_pLightMapMgr != nullptr)
	{
		delete m_pLightMapMgr;
		m_pLightMapMgr = nullptr;
	}

	if (m_pOptimizedSurfaceBatchMgr != nullptr)
	{
		delete m_pOptimizedSurfaceBatchMgr;
		m_pOptimizedSurfaceBatchMgr = nullptr;
	}

	if (m_pCanvasBatchMgr != nullptr)
	{
		delete m_pCanvasBatchMgr;
		m_pCanvasBatchMgr = nullptr;
	}

	if (m_pSpriteBatchMgr != nullptr)
	{
		delete m_pSpriteBatchMgr;
		m_pSpriteBatchMgr = nullptr;
	}

#ifdef WORLD_TWEAKS_ENABLED
	if (m_pWorldTweakMgr != nullptr)
	{
		delete m_pWorldTweakMgr;
		m_pWorldTweakMgr = nullptr;
	}
#endif

	FreeVertexBuffers();
	FreeIndexBuffers();
}

bool CGlobalManager::PostInit()
{
	VertexTypes::PositionColor aLineVerts[2] = { };
	m_pVertexBuffer_Line = CreateVertexBuffer(aLineVerts, sizeof(aLineVerts), D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
	if (m_pVertexBuffer_Line == nullptr)
		return false;

	m_pIndexBuffer_Opt2D = CreateIndexBuffer(g_awIndex_Opt2D, sizeof(g_awIndex_Opt2D), D3D11_USAGE_IMMUTABLE, 0);
	if (m_pIndexBuffer_Opt2D == nullptr)
		return false;

	m_pIndexBuffer_Sprite = CreateIndexBuffer(g_awIndex_Sprite, sizeof(g_awIndex_Sprite), D3D11_USAGE_IMMUTABLE, 0);
	if (m_pIndexBuffer_Sprite == nullptr)
		return false;

	m_pIndexBuffer_SpriteEx = CreateIndexBuffer(g_awIndex_SpriteEx, sizeof(g_awIndex_SpriteEx), D3D11_USAGE_IMMUTABLE, 0);
	if (m_pIndexBuffer_SpriteEx == nullptr)
		return false;

	m_pTextureMgr->Init();

	m_pModelMgr->Init();
	m_pWorldModelMgr->Init();
	m_pSpriteMgr->Init();
	m_pParticleSystemMgr->Init();
	m_pCanvasMgr->Init();
	m_pPolyGridMgr->Init();
	m_pLineSystemMgr->Init();

	m_pLightMapMgr->Init();

	m_pOptimizedSurfaceBatchMgr->Init();
	m_pCanvasBatchMgr->Init();
	m_pSpriteBatchMgr->Init();

	HMODULE hLithtechExe = GetModuleHandle("lithtech.exe");
	m_sEngineHacks.w_DoLightLookup = (w_DoLightLookup_Type)((uint32)hLithtechExe + 0x5980);
	m_sEngineHacks.i_IntersectSegment = (i_IntersectSegment_Type)((uint32)hLithtechExe + 0x37C3D);

	m_eSolidDrawingMode = (SolidDrawingMode)g_CV_SolidDrawingMode.m_Val;
	m_eQueueObjectsMode = (QueueObjectsMode)g_CV_QueueObjectsMode.m_Val;

	return true;
}

void CGlobalManager::FreeAll()
{
	if (m_pTextureMgr != nullptr)
		m_pTextureMgr->Term();

	if (m_pModelMgr != nullptr)
		m_pModelMgr->Term();

	if (m_pWorldModelMgr != nullptr)
		m_pWorldModelMgr->Term();

	if (m_pSpriteMgr != nullptr)
		m_pSpriteMgr->Term();

	if (m_pParticleSystemMgr != nullptr)
		m_pParticleSystemMgr->Term();

	if (m_pCanvasMgr != nullptr)
		m_pCanvasMgr->Term();

	if (m_pPolyGridMgr != nullptr)
		m_pPolyGridMgr->Term();

	if (m_pLineSystemMgr != nullptr)
		m_pLineSystemMgr->Term();

	if (m_pLightMapMgr != nullptr)
		m_pLightMapMgr->Term();

	if (m_pOptimizedSurfaceBatchMgr != nullptr)
		m_pOptimizedSurfaceBatchMgr->Term();

	if (m_pCanvasBatchMgr != nullptr)
		m_pCanvasBatchMgr->Term();

	if (m_pSpriteBatchMgr != nullptr)
		m_pSpriteBatchMgr->Term();

	FreeVertexBuffers();
	FreeIndexBuffers();
}

void CGlobalManager::FreeVertexBuffers()
{
	RELEASE_INTERFACE(m_pVertexBuffer_Line, g_szFreeError_VB_LINE);

	for (BankedBuffer& item : m_aVertexBufferBank)
	{
		RELEASE_INTERFACE(item.m_pBuffer, g_szFreeError_VB);
	}

	m_aVertexBufferBank.clear();
}

void CGlobalManager::FreeIndexBuffers()
{
	RELEASE_INTERFACE(m_pIndexBuffer_Opt2D, g_szFreeError_IB_O2D);
	RELEASE_INTERFACE(m_pIndexBuffer_Sprite, g_szFreeError_IB_SPR);
	RELEASE_INTERFACE(m_pIndexBuffer_SpriteEx, g_szFreeError_IB_SPR);

	for (BankedBuffer& item : m_aIndexBufferBank)
	{
		RELEASE_INTERFACE(item.m_pBuffer, g_szFreeError_IB);
	}

	m_aIndexBufferBank.clear();
}

void CGlobalManager::FreeObjectManagersData()
{
	m_pModelMgr->FreeAllData();
	m_pWorldModelMgr->FreeAllData();
	m_pSpriteMgr->FreeAllData();
	m_pParticleSystemMgr->FreeAllData();
	m_pCanvasMgr->FreeAllData();
	m_pPolyGridMgr->FreeAllData();
	m_pLineSystemMgr->FreeAllData();

	m_pLightMapMgr->FreeAllData();
}

void CGlobalManager::Update()
{
	float fCurrTime = g_fLastClientTime;

	m_pModelMgr->Update(fCurrTime, g_CV_ModelVertexBufferTTL.m_Val);
	//m_pWorldModelMgr->Update(fCurrTime, g_CV_WorldModelVertexBufferTTL.m_Val);
	m_pSpriteMgr->Update(fCurrTime, g_CV_SpriteVertexBufferTTL.m_Val);
	m_pParticleSystemMgr->Update(fCurrTime, g_CV_SpriteVertexBufferTTL.m_Val);
	m_pCanvasMgr->Update(fCurrTime, g_CV_CanvasVertexBufferTTL.m_Val);
	m_pPolyGridMgr->Update(fCurrTime, g_CV_PolyGridVertexBufferTTL.m_Val);
	m_pLineSystemMgr->Update(fCurrTime, g_CV_LineSystemVertexBufferTTL.m_Val);
}

bool CGlobalManager::UpdateGenericBuffer(ID3D11Buffer* pBuffer, void* pData, uint32 dwSize, D3D11_MAP eMapType)
{
	D3D11_MAPPED_SUBRESOURCE subResource;
	HRESULT hResult = g_D3DDevice.GetDeviceContext()->Map(pBuffer, 0, eMapType, 0, &subResource);
	if (FAILED(hResult))
		return false;

	memcpy(subResource.pData, pData, dwSize);

	g_D3DDevice.GetDeviceContext()->Unmap(pBuffer, 0);

	return true;
}

bool CGlobalManager::MapGenericBuffer(ID3D11Buffer* pBuffer, D3D11_MAP eMapType, D3D11_MAPPED_SUBRESOURCE* pSubResource)
{
	HRESULT hResult = g_D3DDevice.GetDeviceContext()->Map(pBuffer, 0, eMapType, 0, pSubResource);

	return SUCCEEDED(hResult);
}

void CGlobalManager::UnmapGenericBuffer(ID3D11Buffer* pBuffer)
{
	g_D3DDevice.GetDeviceContext()->Unmap(pBuffer, 0);
}

ID3D11Buffer* CGlobalManager::CreateVertexBuffer(void* pData, uint32 dwSize, D3D11_USAGE eUsage, UINT dwCPUAccessFlags)
{
	D3D11_BUFFER_DESC desc = { };

	desc.Usage = eUsage;
	desc.ByteWidth = dwSize;
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.CPUAccessFlags = dwCPUAccessFlags;
	desc.MiscFlags = 0;
	desc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA vertexData;

	vertexData.pSysMem = pData;
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;

	ID3D11Buffer* pVertexBuffer;
	HRESULT hResult = g_D3DDevice.GetDevice()->CreateBuffer(&desc, &vertexData, &pVertexBuffer);
	if (FAILED(hResult))
		return nullptr;

	return pVertexBuffer;
}

ID3D11Buffer* CGlobalManager::CreateVertexBuffer(uint32 dwSize, D3D11_USAGE eUsage, UINT dwCPUAccessFlags)
{
	D3D11_BUFFER_DESC desc = { };

	desc.Usage = eUsage;
	desc.ByteWidth = dwSize;
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.CPUAccessFlags = dwCPUAccessFlags;
	desc.MiscFlags = 0;
	desc.StructureByteStride = 0;

	ID3D11Buffer* pVertexBuffer;
	HRESULT hResult = g_D3DDevice.GetDevice()->CreateBuffer(&desc, nullptr, &pVertexBuffer);
	if (FAILED(hResult))
		return nullptr;

	return pVertexBuffer;
}

uint32 CGlobalManager::AssignBankedVertexBuffer(void* pData, uint32 dwSize)
{
	uint32 dwIndex = GetBufferBankItem(m_aVertexBufferBank, dwSize);

	if (dwIndex != UINT32_MAX)
	{
		BankedBuffer& item = m_aVertexBufferBank[dwIndex];

		item.m_bInUse = true;

		if (pData != nullptr)
			UpdateGenericBuffer(item.m_pBuffer, pData, dwSize, D3D11_MAP_WRITE_DISCARD);

		return dwIndex;
	}
	else
	{
		ID3D11Buffer* pBuffer;
		
		if (pData != nullptr)
			pBuffer = CreateVertexBuffer(pData, dwSize, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
		else
			pBuffer = CreateVertexBuffer(dwSize, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

		if (pBuffer == nullptr)
			return UINT32_MAX;

		m_aVertexBufferBank.emplace_back(pBuffer, dwSize, true);

		return m_aVertexBufferBank.size() - 1;
	}
}

ID3D11Buffer* CGlobalManager::CreateIndexBuffer(void* pData, uint32 dwSize, D3D11_USAGE eUsage, UINT dwCPUAccessFlags)
{
	D3D11_BUFFER_DESC desc = { };

	desc.Usage = eUsage;
	desc.ByteWidth = dwSize;
	desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	desc.CPUAccessFlags = dwCPUAccessFlags;
	desc.MiscFlags = 0;
	desc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA indexData;

	indexData.pSysMem = pData;
	indexData.SysMemPitch = 0;
	indexData.SysMemSlicePitch = 0;

	ID3D11Buffer* pIndexBuffer;
	HRESULT hResult = g_D3DDevice.GetDevice()->CreateBuffer(&desc, &indexData, &pIndexBuffer);
	if (FAILED(hResult))
		return nullptr;

	return pIndexBuffer;
}

ID3D11Buffer* CGlobalManager::CreateIndexBuffer(uint32 dwSize, D3D11_USAGE eUsage, UINT dwCPUAccessFlags)
{	
	D3D11_BUFFER_DESC desc = { };

	desc.Usage = eUsage;
	desc.ByteWidth = dwSize;
	desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	desc.CPUAccessFlags = dwCPUAccessFlags;
	desc.MiscFlags = 0;
	desc.StructureByteStride = 0;

	ID3D11Buffer* pIndexBuffer;
	HRESULT hResult = g_D3DDevice.GetDevice()->CreateBuffer(&desc, nullptr, &pIndexBuffer);
	if (FAILED(hResult))
		return nullptr;

	return pIndexBuffer;
}

uint32 CGlobalManager::AssignBankedIndexBuffer(void* pData, uint32 dwSize)
{
	uint32 dwIndex = GetBufferBankItem(m_aIndexBufferBank, dwSize);

	if (dwIndex != UINT32_MAX)
	{
		BankedBuffer& item = m_aIndexBufferBank[dwIndex];

		item.m_bInUse = true;

		if (pData != nullptr)
			UpdateGenericBuffer(item.m_pBuffer, pData, dwSize, D3D11_MAP_WRITE_DISCARD);

		return dwIndex;
	}
	else
	{
		ID3D11Buffer* pBuffer; 
		
		if (pData != nullptr)
			pBuffer = CreateIndexBuffer(pData, dwSize, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
		else
			pBuffer = CreateIndexBuffer(dwSize, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

		if (pBuffer == nullptr)
			return UINT32_MAX;

		m_aIndexBufferBank.emplace_back(pBuffer, dwSize, true);

		return m_aIndexBufferBank.size() - 1;
	}
}

ID3D11Buffer* CGlobalManager::CreateConstantBuffer(void* pData, uint32 dwSize, D3D11_USAGE eUsage, UINT dwCPUAccessFlags)
{
	D3D11_BUFFER_DESC desc = { };

	desc.Usage = eUsage;
	desc.ByteWidth = dwSize;
	desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	desc.CPUAccessFlags = dwCPUAccessFlags;
	desc.MiscFlags = 0;
	desc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA indexData;

	indexData.pSysMem = pData;
	indexData.SysMemPitch = 0;
	indexData.SysMemSlicePitch = 0;

	ID3D11Buffer* pConstantBuffer;
	HRESULT hResult = g_D3DDevice.GetDevice()->CreateBuffer(&desc, &indexData, &pConstantBuffer);
	if (FAILED(hResult))
		return nullptr;

	return pConstantBuffer;
}

ID3D11Buffer* CGlobalManager::CreateConstantBuffer(uint32 dwSize, D3D11_USAGE eUsage, UINT dwCPUAccessFlags)
{
	D3D11_BUFFER_DESC desc = { };

	desc.Usage = eUsage;
	desc.ByteWidth = dwSize;
	desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	desc.CPUAccessFlags = dwCPUAccessFlags;
	desc.MiscFlags = 0;
	desc.StructureByteStride = 0;

	ID3D11Buffer* pConstantBuffer;
	HRESULT hResult = g_D3DDevice.GetDevice()->CreateBuffer(&desc, nullptr, &pConstantBuffer);
	if (FAILED(hResult))
		return nullptr;

	return pConstantBuffer;
}

void CWorldTweakManager::LoadWorldTweaks()
{
	Clear();
	
	ILTStream* pStream = g_pSceneDesc->m_pRenderContext->m_pMainWorld->m_pFileStream;
	ILTStream_VTable* pVTable = (ILTStream_VTable*)pStream->m_pVTable;

	uint32 dwNotUsed = 0;
	uint32 dwOldPos = 0;
	uint32 dwSize = 0;
	
	pVTable->ILTStream_GetPos(pStream, &dwNotUsed, &dwOldPos);

	pVTable->ILTStream_SeekTo(pStream, &dwNotUsed, 0);
	pVTable->ILTStream_GetLen(pStream, &dwNotUsed, &dwSize);

	uint8* pData = (uint8*)malloc(dwSize);

	if (pData != nullptr && dwSize)
	{
		pVTable->ILTStream_Read(pStream, &dwNotUsed, pData, dwSize);

		uint32 dwCRC32 = CalcDataCRC(pData, dwSize);

		uint32 dwConfigSize = 0;
		void* pConfigData = nullptr;

#if defined(DEBUG) || defined(_DEBUG)
		if (g_CV_WorldTweaksFile.m_Val)
		{
			FILE* pFile = nullptr;
			fopen_s(&pFile, "world_tweaks.txt", "rb");

			if (pFile != nullptr)
			{
				fseek(pFile, 0, SEEK_END);
				dwConfigSize = ftell(pFile);

				pConfigData = malloc(dwConfigSize);

				if (pConfigData != nullptr)
				{
					fseek(pFile, 0, SEEK_SET);
					fread(pConfigData, sizeof(char), dwConfigSize, pFile);
				}

				fclose(pFile);
			}		
		}
		else
		{
			GetResourcePointer(g_szExtrasResourceTag, IDR_EXTRA_WORLD_TWEAKS, &dwConfigSize, &pConfigData);
		}
#else
		GetResourcePointer(g_szExtrasResourceTag, IDR_EXTRA_WORLD_TWEAKS, &dwConfigSize, &pConfigData);
#endif

		if (pConfigData != nullptr && dwConfigSize)
			LoadWorldTweaks(pConfigData, dwCRC32);

#if defined(DEBUG) || defined(_DEBUG)
		if (g_CV_WorldTweaksFile.m_Val && pConfigData != nullptr)
			free(pConfigData);
#endif

		free(pData);
	}

	pVTable->ILTStream_SeekTo(pStream, &dwNotUsed, dwOldPos);
}

void CWorldTweakManager::CalcRBCutData(uint32 dwBaseIndexCount, uint32 dwBaseIndexPos, 
	uint32& dwIndexCount1, uint32& dwIndexPos1, uint32& dwIndexCount2, uint32& dwIndexPos2)
{
	switch (m_pRBCutData->m_nType)
	{
		case 'F':
		{
			dwIndexCount1 = dwBaseIndexCount - m_pRBCutData->m_dwCount;
			dwIndexPos1 = dwBaseIndexPos + m_pRBCutData->m_dwCount;

			dwIndexCount2 = 0;
			dwIndexPos2 = 0;
			break;
		}

		case 'B':
		{
			dwIndexCount1 = dwBaseIndexCount - m_pRBCutData->m_dwCount;
			dwIndexPos1 = dwBaseIndexPos;

			dwIndexCount2 = 0;
			dwIndexPos2 = 0;
			break;
		}

		case 'M':
		{
			dwIndexCount1 = m_pRBCutData->m_dwFrom;
			dwIndexPos1 = dwBaseIndexPos;


			dwIndexCount2 = dwBaseIndexCount - m_pRBCutData->m_dwFrom - m_pRBCutData->m_dwCount;
			dwIndexPos2 = dwBaseIndexPos + m_pRBCutData->m_dwFrom + m_pRBCutData->m_dwCount;

			break;
		}
	}
}

void CWorldTweakManager::InitFrameParams(DirectX::XMFLOAT3* pPos)
{
	uint32 dwAllFlags = 0;
	DirectX::XMVECTOR vPosTemp = DirectX::XMLoadFloat3(pPos);

	for (WorldTweak& tweak : m_aTweak)
	{
		if (DirectX::XMVector3Greater(vPosTemp, DirectX::XMLoadFloat3(&tweak.m_vBBoxMin)) &&
			DirectX::XMVector3Less(vPosTemp, DirectX::XMLoadFloat3(&tweak.m_vBBoxMax)))
		{
			if (tweak.m_dwFlags & WORLD_TWEAK_RB_DIST_TEST)
			{
				DirectX::XMStoreFloat3(&m_vRB_BBoxMin, vPosTemp - DirectX::XMVectorReplicate(tweak.m_fRBDist));
				DirectX::XMStoreFloat3(&m_vRB_BBoxMax, vPosTemp + DirectX::XMVectorReplicate(tweak.m_fRBDist));
			}

			if (tweak.m_dwFlags & WORLD_TWEAK_OBJ_DIST_TEST)
			{
				DirectX::XMStoreFloat3(&m_vObj_BBoxMin, vPosTemp - DirectX::XMVectorReplicate(tweak.m_fObjDist));
				DirectX::XMStoreFloat3(&m_vObj_BBoxMax, vPosTemp + DirectX::XMVectorReplicate(tweak.m_fObjDist));
			}

			if (tweak.m_dwFlags & WORLD_TWEAK_RB_CUT)
				m_pRBCutData = &tweak.m_sRBCutData;

			dwAllFlags |= tweak.m_dwFlags;
		}
	}

	m_dwAllFlags = dwAllFlags;
}

void CWorldTweakManager::LoadWorldTweaks(void* pConfigData, uint32 dwCRC32)
{
	char szCRC32[16];
	sprintf_s(szCRC32, "%08X", dwCRC32);

	char strLineBuf[256];
	std::string strConfig = (char*)pConfigData;

	int nStart = 0;
	int nEnd = 0;
	bool bFound = false;
	while (true)
	{
		int nEnd = strConfig.find("\r\n", nStart);

		if (nEnd == -1)
			break;

		strncpy_s(strLineBuf, strConfig.c_str() + nStart, nEnd - nStart);

		if (strLineBuf[0] != 0 && strLineBuf[0] != '#' && strLineBuf[0] != '\t' &&
			!strcmp(strLineBuf, szCRC32))
		{
			bFound = true;
			nStart = nEnd + 2;
			break;
		}

		nStart = nEnd + 2;
	}

	if (bFound)
	{
		ConParse parser;

		while (true)
		{
			int nEnd = strConfig.find("\r\n", nStart);

			if (nEnd == -1)
				break;

			strncpy_s(strLineBuf, strConfig.c_str() + nStart, nEnd - nStart);

			if (strLineBuf[0] != 0 && strLineBuf[0] != '#' && strncmp(strLineBuf, "\t#", 2))
			{
				if (strLineBuf[0] != '\t')
					break;

				if (strLineBuf[0] == '\t')
				{				
					parser.Init(strLineBuf);
					parser.Parse();
					
					if (parser.m_nArgs)
					{
						if (!strcmp(parser.m_pArgs[0], "\tBBox"))
						{
							DirectX::XMFLOAT3 vBBoxMin =
							{
								(float)atof(parser.m_pArgs[1]),
								(float)atof(parser.m_pArgs[2]),
								(float)atof(parser.m_pArgs[3]),
							};

							DirectX::XMFLOAT3 vBBoxMax =
							{
								(float)atof(parser.m_pArgs[4]),
								(float)atof(parser.m_pArgs[5]),
								(float)atof(parser.m_pArgs[6]),
							};

							m_aTweak.emplace_back(&vBBoxMin, &vBBoxMax);
						}
						else if (!strcmp(parser.m_pArgs[0], "\t\tHideSky"))
						{
							m_aTweak.back().m_dwFlags |= WORLD_TWEAK_HIDE_SKY;
						}
						else if (!strcmp(parser.m_pArgs[0], "\t\tHideBsp"))
						{
							m_aTweak.back().m_dwFlags |= WORLD_TWEAK_HIDE_BSP;
						}
						else if (!strcmp(parser.m_pArgs[0], "\t\tHideTerrain"))
						{
							m_aTweak.back().m_dwFlags |= WORLD_TWEAK_HIDE_TERRAIN;
						}
						else if (!strcmp(parser.m_pArgs[0], "\t\tDistTest"))
						{
							float fRBDist = (float)atof(parser.m_pArgs[1]);
							float fObjDist = (float)atof(parser.m_pArgs[2]);

							WorldTweak& tweak = m_aTweak.back();

							if (fRBDist > 0.0f)
							{
								tweak.m_dwFlags |= WORLD_TWEAK_RB_DIST_TEST;
								tweak.m_fRBDist = fRBDist;
							}

							if (fObjDist > 0.0f)
							{
								tweak.m_dwFlags |= WORLD_TWEAK_OBJ_DIST_TEST;
								tweak.m_fObjDist = fObjDist;
							}
						}
						else if (!strcmp(parser.m_pArgs[0], "\t\tRBCut"))
						{
							WorldTweak& tweak = m_aTweak.back();

							tweak.m_dwFlags |= WORLD_TWEAK_RB_CUT;
							tweak.m_sRBCutData.m_dwIndex = atoi(parser.m_pArgs[1]);
							tweak.m_sRBCutData.m_nType = parser.m_pArgs[2][0];
							tweak.m_sRBCutData.m_dwFrom = atoi(parser.m_pArgs[3]);
							tweak.m_sRBCutData.m_dwCount = atoi(parser.m_pArgs[4]);
						}
					}
				}
			}

			nStart = nEnd + 2;
		}
	}
}

uint32 CGlobalManager::GetBufferBankItem(Array_BankedBuffer& bank, uint32 dwSize)
{
	uint32 dwBestIndex = UINT32_MAX;
	uint32 dwBestSize = UINT32_MAX;
	
	for (uint32 i = 0; i < bank.size(); i++)
	{
		BankedBuffer& item = bank[i];

		if (item.m_bInUse || dwSize > item.m_dwSize)
			continue;
		
		if (item.m_dwSize < dwBestSize)
		{
			dwBestIndex = i;
			dwBestSize = item.m_dwSize;
		}	
	}

	// TODO - sure?
	return (dwBestSize < dwSize << 3) ? dwBestIndex : UINT32_MAX;
}