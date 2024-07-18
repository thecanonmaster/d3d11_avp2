#include "pch.h"

#include "rendershadermgr.h"
#include "common_stuff.h"
#include "d3d_device.h"
#include <d3dcompiler.h>
#include "resource.h"
#include "d3d_shader_optimizedsurface.h"
#include "d3d_shader_soliddrawing.h"
#include "d3d_shader_tonemap.h"
#include "d3d_shader_screenfx.h"
#include "d3d_shader_clearscreen.h"
#include "d3d_shader_fullscreenvideo.h"
#include "d3d_shader_model.h"
#include "d3d_shader_worldmodel.h"
#include "d3d_shader_sprite.h"
#include "d3d_shader_particles.h"
#include "d3d_shader_canvas.h"
#include "d3d_shader_polygrid.h"
#include "d3d_shader_linesystem.h"
#include "d3d_shader_passthrough.h"
#include "d3d_shader_bloom.h"
#include "globalmgr.h"

CRenderShaderMgr g_RenderShaderMgr;

HRESULT ResourceInclude::Open(D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes)
{
	if (!GetResourcePointer(g_szShaderResourceTag, IDR_SHADER_GLOBAL_HEADER, (uint32*)pBytes, (void**)ppData))
		return S_FALSE;
	
	return S_OK;
}

HRESULT ResourceInclude::Close(LPCVOID pData)
{
	return S_OK;
}

CRenderShaderMgr::CRenderShaderMgr()
{
	ResetCurrentData();

	m_pPSPerFrameBuffer_Opt2D = nullptr;
	m_pVSPerFrameBuffer_Opt2D = nullptr;
	m_pPSPerFrameBuffer_3D = nullptr;
	m_pVSPerFrameBuffer_3D = nullptr;
	m_pVPSDynamicLights_3D = nullptr;
};

CRenderShaderMgr::~CRenderShaderMgr()
{
	FreeAll();
}

bool CRenderShaderMgr::Init()
{
	m_pVSPerFrameBuffer_Opt2D = g_GlobalMgr.CreateConstantBuffer(sizeof(CRenderShader_Base::VSInputPerFrame_Opt2D), 
		D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
	if (m_pVSPerFrameBuffer_Opt2D == nullptr)
		return false;

	m_pPSPerFrameBuffer_Opt2D = g_GlobalMgr.CreateConstantBuffer(sizeof(CRenderShader_Base::PSInputPerFrame_Opt2D),
		D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
	if (m_pPSPerFrameBuffer_Opt2D == nullptr)
		return false;

	m_pVSPerFrameBuffer_3D = g_GlobalMgr.CreateConstantBuffer(sizeof(CRenderShader_Base::VSInputPerFrame_3D),
		D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
	if (m_pVSPerFrameBuffer_3D == nullptr)
		return false;

	m_pPSPerFrameBuffer_3D = g_GlobalMgr.CreateConstantBuffer(sizeof(CRenderShader_Base::PSInputPerFrame_3D),
		D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
	if (m_pPSPerFrameBuffer_3D == nullptr)
		return false;

	m_pVPSDynamicLights_3D = g_GlobalMgr.CreateConstantBuffer(sizeof(CRenderShader_Base::VPSInputDynamicLights_3D),
		D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
	if (m_pVPSDynamicLights_3D == nullptr)
		return false;

	bool bRet = true;

	bRet &= CreateRenderShader<CRenderShader_OptimizedSurface>(IDR_SHADER_OPTIMIZED_SURFACE_VS, 0, IDR_SHADER_OPTIMIZED_SURFACE_PS);
	bRet &= CreateRenderShader<CRenderShader_OptimizedSurfaceBatch>(IDR_SHADER_OPTIMIZED_SURFACE_BAT_VS, 0, IDR_SHADER_OPTIMIZED_SURFACE_BAT_PS);
	bRet &= CreateRenderShader<CRenderShader_SolidDrawing>(IDR_SHADER_SOLIDDRAWING_VS, 0, IDR_SHADER_SOLIDDRAWING_PS);
	bRet &= CreateRenderShader<CRenderShader_ToneMap>(IDR_SHADER_TONEMAP_VS, 0, IDR_SHADER_TONEMAP_PS);
	bRet &= CreateRenderShader<CRenderShader_ScreenFX>(IDR_SHADER_SCREENFX_VS, 0, IDR_SHADER_SCREENFX_PS);
	bRet &= CreateRenderShader<CRenderShader_ClearScreen>(IDR_SHADER_CLEARSCREEN_VS, 0, IDR_SHADER_CLEARSCREEN_PS);
	bRet &= CreateRenderShader<CRenderShader_FullScreenVideo>(IDR_SHADER_FULLSCREEN_VIDEO_VS, 0, IDR_SHADER_FULLSCREEN_VIDEO_PS);
	bRet &= CreateRenderShader<CRenderShader_Model>(IDR_SHADER_MODEL_VS, IDR_SHADER_MODEL_GS, IDR_SHADER_MODEL_PS);
	bRet &= CreateRenderShader<CRenderShader_WorldModel>(IDR_SHADER_WORLD_MODEL_VS, 0, IDR_SHADER_WORLD_MODEL_PS);
	bRet &= CreateRenderShader<CRenderShader_SkyWorldModel>(IDR_SHADER_SKY_WORLD_MODEL_VS, 0, IDR_SHADER_SKY_WORLD_MODEL_PS);
	bRet &= CreateRenderShader<CRenderShader_SkyPortal>(IDR_SHADER_SKY_PORTAL_VS, 0, IDR_SHADER_SKY_PORTAL_PS);
	bRet &= CreateRenderShader<CRenderShader_Sprite>(IDR_SHADER_SPRITE_VS, 0, IDR_SHADER_SPRITE_PS);
	bRet &= CreateRenderShader<CRenderShader_SpriteBatch>(IDR_SHADER_SPRITE_BAT_VS, 0, IDR_SHADER_SPRITE_BAT_PS);
	bRet &= CreateRenderShader<CRenderShader_ParticleSystem>(IDR_SHADER_PARTICLE_SYSTEM_VS, 0, IDR_SHADER_PARTICLE_SYSTEM_PS);
	bRet &= CreateRenderShader<CRenderShader_Canvas>(IDR_SHADER_CANVAS_VS, 0, IDR_SHADER_CANVAS_PS);
	bRet &= CreateRenderShader<CRenderShader_CanvasBatch>(IDR_SHADER_CANVAS_BAT_VS, 0, IDR_SHADER_CANVAS_BAT_PS);
	bRet &= CreateRenderShader<CRenderShader_PolyGrid>(IDR_SHADER_POLYGRID_VS, 0, IDR_SHADER_POLYGRID_PS);
	bRet &= CreateRenderShader<CRenderShader_LineSystem>(IDR_SHADER_LINE_SYSTEM_VS, 0, IDR_SHADER_LINE_SYSTEM_PS);
	bRet &= CreateRenderShader<CRenderShader_PassThrough>(IDR_SHADER_PASSTHROUGH_VS, 0, IDR_SHADER_PASSTHROUGH_PS);
	bRet &= CreateRenderShader<CRenderShader_BloomExtract>(IDR_SHADER_BLOOM_EXTRACT_VS, 0, IDR_SHADER_BLOOM_EXTRACT_PS);
	bRet &= CreateRenderShader<CRenderShader_BloomBlur>(IDR_SHADER_BLOOM_BLUR_VS, 0, IDR_SHADER_BLOOM_BLUR_PS);
	bRet &= CreateRenderShader<CRenderShader_BloomCombine>(IDR_SHADER_BLOOM_COMBINE_VS, 0, IDR_SHADER_BLOOM_COMBINE_PS);

	return bRet;
}

void CRenderShaderMgr::FreeAll()
{
	for (const CRenderShader_Base* pShader : m_aRenderShader)
		delete pShader;

	m_aRenderShader.clear();

	RELEASE_INTERFACE(m_pVSPerFrameBuffer_Opt2D, g_szFreeError_VS_PF_CB, nullptr);
	RELEASE_INTERFACE(m_pPSPerFrameBuffer_Opt2D, g_szFreeError_PS_PF_CB, nullptr);
	RELEASE_INTERFACE(m_pVSPerFrameBuffer_3D, g_szFreeError_VS_PF_CB, nullptr);
	RELEASE_INTERFACE(m_pPSPerFrameBuffer_3D, g_szFreeError_PS_PF_CB, nullptr);
	RELEASE_INTERFACE(m_pVPSDynamicLights_3D, g_szFreeError_VPS_DL_CB, nullptr);
}

bool CRenderShaderMgr::GetShaderSourceName(void* pSource, uint32 dwSourceSize, char* szName)
{
	char* szPos = (char*)pSource;
	char* szEnd = szPos + dwSourceSize;
	
	if (dwSourceSize < 3 || szPos[0] != '/' || szPos[1] != '/')
		return false;

	szPos += 2;

	while (*szPos == ' ')
	{
		szPos++;

		if (szPos == szEnd)
			return false;
	}

	char* szFrom = szPos;

	while (*szPos != '\r' && *szPos != '\n' && szPos != szEnd)
		szPos++;

	uint32 dwNameLen = szPos - szFrom;
	strncpy_s(szName, SOURCE_NAME_LENGTH - 1, szFrom, dwNameLen);
	szName[dwNameLen] = 0;

	return true;
}

ID3D10Blob* CRenderShaderMgr::CompileShader(void* pSource, uint32 dwSourceSize, const char* szEntryPoint, const char* szProfile)
{
	uint32 dwFlags = D3DCOMPILE_ENABLE_STRICTNESS;

#if defined(DEBUG) || defined(_DEBUG)
	dwFlags |= (D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_WARNINGS_ARE_ERRORS);
#endif

	const D3D_SHADER_MACRO defines[] = { "EXAMPLE_DEFINE", "1", nullptr, nullptr };

	ID3DBlob* pShaderBlob = nullptr;
	ID3DBlob* pErrorBlob = nullptr;
	char szSourceName[128] = { 0 };
	bool bHasSourceName = GetShaderSourceName(pSource, dwSourceSize, szSourceName);
	ResourceInclude include;

	HRESULT hResult = D3DCompile(pSource, dwSourceSize, bHasSourceName ? szSourceName : nullptr, defines,
		&include, szEntryPoint, szProfile, dwFlags, 0, &pShaderBlob, &pErrorBlob);

	if (FAILED(hResult))
	{
		if (pErrorBlob != nullptr)
		{
			const char* szError = (const char*)(pErrorBlob->GetBufferPointer());
			AddDebugMessage(0, szError);
			pErrorBlob->Release();
		}

		if (pShaderBlob != nullptr)
			pShaderBlob->Release();
	
		return nullptr;
	}

	return pShaderBlob;
}

ID3D10Blob* CRenderShaderMgr::CompileShader(uint32 dwResource, const char* szEntryPoint, const char* szProfile)
{
	void* pSource = nullptr;
	uint32 dwSourceSize = 0;

	if (!GetResourcePointer(g_szShaderResourceTag, dwResource, &dwSourceSize, &pSource))
		return nullptr;

	ID3D10Blob* pShaderBlob = CompileShader(pSource, dwSourceSize, szEntryPoint, szProfile);
	if (pShaderBlob == nullptr)
		return nullptr;

	return pShaderBlob;
}

void CRenderShaderMgr::ResetCurrentData()
{
	m_eCurrentInputLayout = SHADER_Invalid;
	m_eCurrentShaderVS = SHADER_Invalid;
	m_eCurrentShaderGS = SHADER_Invalid;
	m_eCurrentShaderPS = SHADER_Invalid;

	for (uint32 i = 0; i < MAX_CONST_BUFFER_SLOTS; i++)
	{
		m_aeCurrentConstBufferSlotsVS[i] = CBI_VS_Invalid;
		m_aeCurrentConstBufferSlotsGS[i] = CBI_GS_Invalid;
		m_aeCurrentConstBufferSlotsPS[i] = CBI_PS_Invalid;
	}

	for (uint32 i = 0; i < MAX_SHADER_RESOURCE_SLOTS; i++)
	{
		m_apCurrentShaderResourcesVS[i] = nullptr;
		m_apCurrentShaderResourcesPS[i] = nullptr;
	}

	for (uint32 i = 0; i < MAX_VERTEX_RESOURCE_SLOTS; i++)
		m_aCurrentVertexResources[i].Init(nullptr, 0, 0);

	m_pCurrentIndexBuffer = nullptr;
	m_dwCurrentIndexBufferOffset = 0;
	m_eCurrentPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
}

void CRenderShaderMgr::SetInputLayout(RenderShader eIndex, ID3D11InputLayout* pLayout)
{
	if (m_eCurrentInputLayout == eIndex)
		return;

	m_eCurrentInputLayout = eIndex;
	g_D3DDevice.GetDeviceContext()->IASetInputLayout(pLayout);
}

void CRenderShaderMgr::SetShaderVS(RenderShader eIndex, ID3D11VertexShader* pShader)
{
	if (m_eCurrentShaderVS == eIndex)
		return;

	m_eCurrentShaderVS = eIndex;
	g_D3DDevice.GetDeviceContext()->VSSetShader(pShader, nullptr, 0);
}

void CRenderShaderMgr::SetShaderGS(RenderShader eIndex, ID3D11GeometryShader* pShader)
{
	if (m_eCurrentShaderGS == eIndex)
		return;

	m_eCurrentShaderGS = eIndex;
	g_D3DDevice.GetDeviceContext()->GSSetShader(pShader, nullptr, 0);
}

void CRenderShaderMgr::SetShaderPS(RenderShader eIndex, ID3D11PixelShader* pShader)
{
	if (m_eCurrentShaderPS == eIndex)
		return;

	m_eCurrentShaderPS = eIndex;
	g_D3DDevice.GetDeviceContext()->PSSetShader(pShader, nullptr, 0);
}

void CRenderShaderMgr::SetConstantBuffersVS(uint32 eStart, uint32 dwCount, ConstBufferIndex_VS eIndex, ID3D11Buffer** ppConstantBuffers)
{
	for (uint32 i = eStart; i < eStart + dwCount; i++)
		m_aeCurrentConstBufferSlotsVS[i] = eIndex;
	
	g_D3DDevice.GetDeviceContext()->VSSetConstantBuffers(eStart, dwCount, ppConstantBuffers);
}

void CRenderShaderMgr::SetConstantBuffersGS(uint32 eStart, uint32 dwCount, ConstBufferIndex_GS eIndex, ID3D11Buffer** ppConstantBuffers)
{
	for (uint32 i = eStart; i < eStart + dwCount; i++)
		m_aeCurrentConstBufferSlotsGS[i] = eIndex;

	g_D3DDevice.GetDeviceContext()->GSSetConstantBuffers(eStart, dwCount, ppConstantBuffers);
}

void CRenderShaderMgr::SetConstantBuffersPS(uint32 eStart, uint32 dwCount, ConstBufferIndex_PS eIndex, ID3D11Buffer** ppConstantBuffers)
{
	for (uint32 i = eStart; i < eStart + dwCount; i++)
		m_aeCurrentConstBufferSlotsPS[i] = eIndex;

	g_D3DDevice.GetDeviceContext()->PSSetConstantBuffers(eStart, dwCount, ppConstantBuffers);
}

void CRenderShaderMgr::SetShaderResourceVS(uint32 eSlot, ID3D11ShaderResourceView* pResourceView)
{
	if (m_apCurrentShaderResourcesVS[eSlot] == pResourceView)
		return;

	m_apCurrentShaderResourcesVS[eSlot] = pResourceView;
	g_D3DDevice.GetDeviceContext()->VSSetShaderResources(eSlot, 1, &pResourceView);
}

void CRenderShaderMgr::SetShaderResourcesVS(uint32 eStart, uint32 dwCount, ID3D11ShaderResourceView** pResourceViews)
{
	uint32 dwSlotEnd = eStart + dwCount;

	for (uint32 i = eStart; i < dwSlotEnd; i++)
	{
		if (m_apCurrentShaderResourcesVS[i] != pResourceViews[i - eStart])
			goto LABEL_SetShaderResourcesVS_continue;
	}

	return;

LABEL_SetShaderResourcesVS_continue:

	for (uint32 i = eStart; i < dwSlotEnd; i++)
		m_apCurrentShaderResourcesVS[i] = pResourceViews[i - eStart];

	g_D3DDevice.GetDeviceContext()->VSSetShaderResources(eStart, dwCount, pResourceViews);
}

void CRenderShaderMgr::ClearShaderResourcesVS(uint32 dwStart, uint32 dwCount)
{
	ID3D11ShaderResourceView* apNullSRV[MAX_SHADER_RESOURCE_SLOTS] = { };
	g_D3DDevice.GetDeviceContext()->VSSetShaderResources(dwStart, dwCount, apNullSRV);
}

void CRenderShaderMgr::SetShaderResourcePS(uint32 eSlot, ID3D11ShaderResourceView* pResourceView)
{
	if (m_apCurrentShaderResourcesPS[eSlot] == pResourceView)
		return;

	m_apCurrentShaderResourcesPS[eSlot] = pResourceView;
	g_D3DDevice.GetDeviceContext()->PSSetShaderResources(eSlot, 1, &pResourceView);
}

void CRenderShaderMgr::SetShaderResourcesPS(uint32 eStart, uint32 dwCount, ID3D11ShaderResourceView** pResourceViews)
{
	uint32 dwSlotEnd = eStart + dwCount;

	for (uint32 i = eStart; i < dwSlotEnd; i++)
	{
		if (m_apCurrentShaderResourcesPS[i] != pResourceViews[i - eStart])
			goto LABEL_SetShaderResourcesPS_continue;
	}

	return;

	LABEL_SetShaderResourcesPS_continue:

	for (uint32 i = eStart; i < dwSlotEnd; i++)
		m_apCurrentShaderResourcesPS[i] = pResourceViews[i - eStart];

	g_D3DDevice.GetDeviceContext()->PSSetShaderResources(eStart, dwCount, pResourceViews);
}

void CRenderShaderMgr::ClearShaderResourcesPS(uint32 eStart, uint32 dwCount)
{
	for (uint32 i = eStart; i < eStart + dwCount; i++)
		m_apCurrentShaderResourcesPS[i] = nullptr;

	ID3D11ShaderResourceView* apNullSRV[MAX_SHADER_RESOURCE_SLOTS] = { };
	g_D3DDevice.GetDeviceContext()->PSSetShaderResources(eStart, dwCount, apNullSRV);
}

void CRenderShaderMgr::SetVertexResource(uint32 eSlot, ID3D11Buffer* pBuffer, uint32 dwStride, uint32 dwOffset)
{
	VertexResourceSlotData& data = m_aCurrentVertexResources[eSlot];

	if (data.CompareTo(pBuffer, dwStride, dwOffset))
		return;

	data.Init(pBuffer, dwStride, dwOffset);
	g_D3DDevice.GetDeviceContext()->IASetVertexBuffers(eSlot, 1, &pBuffer, &dwStride, &dwOffset);
}

void CRenderShaderMgr::SetVertexResources(uint32 eStart, uint32 dwCount, ID3D11Buffer** ppBuffers, uint32* pStrides, 
	uint32* pOffsets)
{
	uint32 dwSlotEnd = eStart + dwCount;

	for (uint32 i = eStart; i < dwSlotEnd; i++)
	{
		uint32 dwSourceSlot = i - eStart;

		if (!m_aCurrentVertexResources[i].CompareTo(ppBuffers[dwSourceSlot], pStrides[dwSourceSlot], 
			pOffsets[dwSourceSlot]))
		{
			goto LABEL_SetVertexResources_continue;
		}
	}

	return;

	LABEL_SetVertexResources_continue:

	for (uint32 i = eStart; i < dwSlotEnd; i++)
	{
		uint32 dwSourceSlot = i - eStart;
		
		m_aCurrentVertexResources[i].Init(ppBuffers[dwSourceSlot], pStrides[dwSourceSlot], 
			pOffsets[dwSourceSlot]);
	}

	g_D3DDevice.GetDeviceContext()->IASetVertexBuffers(eStart, dwCount, ppBuffers, pStrides, pOffsets);
}

void CRenderShaderMgr::SetIndexBuffer16(ID3D11Buffer* pBuffer, uint32 dwOffset)
{
	if (m_pCurrentIndexBuffer == pBuffer && m_dwCurrentIndexBufferOffset == dwOffset)
		return;

	m_pCurrentIndexBuffer = pBuffer;
	m_dwCurrentIndexBufferOffset = dwOffset;
	g_D3DDevice.GetDeviceContext()->IASetIndexBuffer(pBuffer, DXGI_FORMAT_R16_UINT, dwOffset);
}

void CRenderShaderMgr::SetIndexBuffer32(ID3D11Buffer* pBuffer, uint32 dwOffset)
{
	if (m_pCurrentIndexBuffer == pBuffer && m_dwCurrentIndexBufferOffset == dwOffset)
		return;

	m_pCurrentIndexBuffer = pBuffer;
	m_dwCurrentIndexBufferOffset = dwOffset;
	g_D3DDevice.GetDeviceContext()->IASetIndexBuffer(pBuffer, DXGI_FORMAT_R32_UINT, dwOffset);
}

void CRenderShaderMgr::SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY eTopology)
{
	if (m_eCurrentPrimitiveTopology == eTopology)
		return;

	m_eCurrentPrimitiveTopology = eTopology;
	g_D3DDevice.GetDeviceContext()->IASetPrimitiveTopology(eTopology);
}

bool CRenderShaderMgr::SetPerFrameParamsVS_Opt2D(uint32 dwScreenWidth, uint32 dwScreenHeight)
{
	if (m_aeCurrentConstBufferSlotsVS[CBS_Opt2D_VS_PerFrame] == CBI_Opt2D_VS_PerFrame)
		return true;

	auto lambdaUpdate = [dwScreenWidth, dwScreenHeight](D3D11_MAPPED_SUBRESOURCE* pSubResource)
		{
			CRenderShader_Base::VSInputPerFrame_Opt2D* pVSData = (CRenderShader_Base::VSInputPerFrame_Opt2D*)pSubResource->pData;
			pVSData->Init(dwScreenWidth, dwScreenHeight);
		};

	if (!g_GlobalMgr.UpdateGenericBufferEx(m_pVSPerFrameBuffer_Opt2D, D3D11_MAP_WRITE_DISCARD, lambdaUpdate))
		return false;

	SetConstantBuffersVS(CBS_Opt2D_VS_PerFrame, 1, CBI_Opt2D_VS_PerFrame, &m_pVSPerFrameBuffer_Opt2D);

	return true;
}

bool CRenderShaderMgr::SetPerFrameParamsPS_Opt2D(float fExposure)
{
	if (m_aeCurrentConstBufferSlotsPS[CBS_Opt2D_PS_PerFrame] == CBI_Opt2D_PS_PerFrame)
		return true;

	auto lambdaUpdate = [fExposure](D3D11_MAPPED_SUBRESOURCE* pSubResource)
		{
			CRenderShader_Base::PSInputPerFrame_Opt2D* pData = (CRenderShader_Base::PSInputPerFrame_Opt2D*)pSubResource->pData;
			pData->Init(fExposure);
		};

	if (!g_GlobalMgr.UpdateGenericBufferEx(m_pPSPerFrameBuffer_Opt2D, D3D11_MAP_WRITE_DISCARD, lambdaUpdate))
		return false;

	SetConstantBuffersPS(CBS_Opt2D_PS_PerFrame, 1, CBI_Opt2D_PS_PerFrame, &m_pPSPerFrameBuffer_Opt2D);

	return true;
}

bool CRenderShaderMgr::SetPerFrameParamsVS_3D(CRenderShader_Base::VSPerFrameParams_3D* pInitStruct)
{
	if (m_aeCurrentConstBufferSlotsVS[CBS_3D_VS_PerFrame] == CBI_3D_VS_PerFrame)
		return true;

	auto lambdaUpdate = [pInitStruct](D3D11_MAPPED_SUBRESOURCE* pSubResource)
		{
			CRenderShader_Base::VSInputPerFrame_3D* pData = (CRenderShader_Base::VSInputPerFrame_3D*)pSubResource->pData;
			pData->Init(pInitStruct);
		};

	if (!g_GlobalMgr.UpdateGenericBufferEx(m_pVSPerFrameBuffer_3D, D3D11_MAP_WRITE_DISCARD, lambdaUpdate))
		return false;

	SetConstantBuffersVS(CBS_3D_VS_PerFrame, 1, CBI_3D_VS_PerFrame, &m_pVSPerFrameBuffer_3D);
	
	return true;
}

bool CRenderShaderMgr::SetPerFrameParamsPS_3D(CRenderShader_Base::PSPerFrameParams_3D* pInitStruct)
{
	if (m_aeCurrentConstBufferSlotsPS[CBS_3D_PS_PerFrame] == CBI_3D_PS_PerFrame)
		return true;

	auto lambdaUpdate = [pInitStruct](D3D11_MAPPED_SUBRESOURCE* pSubResource)
		{
			CRenderShader_Base::PSInputPerFrame_3D* pData = (CRenderShader_Base::PSInputPerFrame_3D*)pSubResource->pData;
			pData->Init(pInitStruct);
		};

	if (!g_GlobalMgr.UpdateGenericBufferEx(m_pPSPerFrameBuffer_3D, D3D11_MAP_WRITE_DISCARD, lambdaUpdate))
		return false;

	SetConstantBuffersPS(CBS_3D_PS_PerFrame, 1, CBI_3D_PS_PerFrame, &m_pPSPerFrameBuffer_3D);
	
	SetShaderResourcePS(SRS_PS_GlobalEnvMap, pInitStruct->m_pGlobalEnvMapSRV);
	SetShaderResourcePS(SRS_PS_GlobalSkyPan, pInitStruct->m_pGlobalPanSRV);

	SetShaderResourcePS(SRS_PS_LMPages, pInitStruct->m_pLMPages);

	return true;
}

bool CRenderShaderMgr::SetDynamicLightsVPS_3D(Array_PLTObject& aLight, Array_ModelShadowLight& aModelShadowLight)
{
	if (m_aeCurrentConstBufferSlotsVS[CBS_3D_VS_DynamicLights] == CBI_3D_VS_DynamicLights &&
		m_aeCurrentConstBufferSlotsPS[CBS_3D_PS_DynamicLights] == CBI_3D_PS_DynamicLights)
	{
		return true;
	}

	auto lambdaUpdate = [aLight, aModelShadowLight](D3D11_MAPPED_SUBRESOURCE* pSubResource)
		{
			CRenderShader_Base::VPSInputDynamicLights_3D* pData = 
				(CRenderShader_Base::VPSInputDynamicLights_3D*)pSubResource->pData;

			pData->Init(aLight, aModelShadowLight, false);
		};
	
	if (!g_GlobalMgr.UpdateGenericBufferEx(m_pVPSDynamicLights_3D, D3D11_MAP_WRITE_DISCARD, lambdaUpdate))
		return false;
	
	SetConstantBuffersVS(CBS_3D_VS_DynamicLights, 1, CBI_3D_VS_DynamicLights, &m_pVPSDynamicLights_3D);
	SetConstantBuffersPS(CBS_3D_PS_DynamicLights, 1, CBI_3D_PS_DynamicLights, &m_pVPSDynamicLights_3D);

	return true;
}

template <class T>
bool CRenderShaderMgr::CreateRenderShader(uint32 dwVSResource, uint32 dwGSResource, uint32 dwPSResource)
{
	ID3D10Blob* pVSBlob = CompileShader(dwVSResource, g_szShaderEntrypointVS, g_szShaderProfileVS);
	if (pVSBlob == nullptr)
		return false;

	ID3D10Blob* pGSBlob = nullptr;
	if (dwGSResource)
	{
		pGSBlob = CompileShader(dwGSResource, g_szShaderEntrypointGS, g_szShaderProfileGS);
		if (pGSBlob == nullptr)
		{
			pVSBlob->Release();
			return false;
		}
	}

	ID3D10Blob* pPSBlob = CompileShader(dwPSResource, g_szShaderEntrypointPS, g_szShaderProfilePS);
	if (pPSBlob == nullptr)
	{
		pVSBlob->Release();

		if (dwGSResource)
			pGSBlob->Release();

		return false;
	}

	ID3D11VertexShader* pVertexShader;
	HRESULT hResult = g_D3DDevice.GetDevice()->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &pVertexShader);
	if (FAILED(hResult))
	{
		pVSBlob->Release();
		pPSBlob->Release();
		return false;
	}

	ID3D11GeometryShader* pGeometryShader = nullptr;
	if (dwGSResource)
	{
		hResult = g_D3DDevice.GetDevice()->CreateGeometryShader(pGSBlob->GetBufferPointer(), pGSBlob->GetBufferSize(), nullptr, &pGeometryShader);
		if (FAILED(hResult))
		{
			pVSBlob->Release();
			pGSBlob->Release();
			pPSBlob->Release();
			pVertexShader->Release();
			return false;
		}
	}

	ID3D11PixelShader* pPixelShader;
	hResult = g_D3DDevice.GetDevice()->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &pPixelShader);
	if (FAILED(hResult))
	{
		pVSBlob->Release();

		if (dwGSResource)
		{
			pGSBlob->Release();
			pGeometryShader->Release();
		}

		pPSBlob->Release();
		pVertexShader->Release();
		return false;
	}

	T* pRenderShader = new T();

	if (!pRenderShader->Init(pVertexShader, pPixelShader, pVSBlob) || !pRenderShader->ExtraInit(pGeometryShader))
	{
		pVSBlob->Release();

		if (dwGSResource)
			pGSBlob->Release();

		pPSBlob->Release();
		delete pRenderShader;
		return false;
	}

	m_aRenderShader.push_back(pRenderShader);

	return (T::m_eRenderShader == m_aRenderShader.size() - 1);
}
