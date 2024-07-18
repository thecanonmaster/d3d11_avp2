#include "pch.h"

#include "d3d_shell.h"
#include "common_stuff.h"
#include "rendererconsolevars.h"
#include "d3d_mathhelpers.h"
#include <d3d11sdklayers.h>

CD3D_Shell g_D3DShell;

bool CD3D_Shell::Create()
{
	FreeAll();

	HRESULT hResult = CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&m_pFactory);

	if (FAILED(hResult))
		return false;

	if (!BuildDeviceList()) 
	{
		FreeAll(); 
		return false;
	}

	return true;
}

void CD3D_Shell::Reset()
{
	m_pFactory = nullptr;
	
	*m_szAdapterOverride = 0;
	*m_szOutputOverride = 0;
	
	m_AdapterList.clear();
}

void CD3D_Shell::FreeAll()
{
	if (m_pFactory != nullptr)
	{
		uint32 dwRefCount = m_pFactory->Release();
		if (dwRefCount)
		{
			AddDebugMessage(0, "Failed to free DXGI factory (count = %d)", dwRefCount);
		}
	}
	
	Reset();
}

bool CD3D_Shell::IsAdapterSupported(DXGI_ADAPTER_DESC1* pDesc)
{
	return pDesc->Flags != DXGI_ADAPTER_FLAG_SOFTWARE;
}

bool CD3D_Shell::IsOutputSupported(DXGI_OUTPUT_DESC* pDesc)
{
	return pDesc->AttachedToDesktop;
}

bool CD3D_Shell::IsModeSupported(DXGI_MODE_DESC* pDesc)
{
	int nScalingOverride = g_nLauncher_D3D11_ModeScaling != 1 ? g_nLauncher_D3D11_ModeScaling : -1;
	nScalingOverride = g_CV_D3D11_ModeScaling.m_Val != 1 ? g_CV_D3D11_ModeScaling.m_Val : nScalingOverride;
	return nScalingOverride != -1 ? pDesc->Scaling == nScalingOverride : pDesc->Scaling == DXGI_MODE_SCALING_UNSPECIFIED;
}

bool CD3D_Shell::BuildDeviceList()
{
	if (m_pFactory == nullptr)
		return false;
	
	m_AdapterList.clear();
	
	HRESULT hResult;

	IDXGIAdapter1* pAdapter;

	for (uint32 dwAdapter = 0; m_pFactory->EnumAdapters1(dwAdapter, &pAdapter) != DXGI_ERROR_NOT_FOUND; dwAdapter++)
	{
		DXGI_ADAPTER_DESC1 adapterDesc;
		hResult = pAdapter->GetDesc1(&adapterDesc);

		D3DAdapterInfo adapterInfo(&adapterDesc);

		if (FAILED(hResult))
			continue;

		if (!IsAdapterSupported(&adapterDesc))
		{
			pAdapter->Release();
			continue;
		}

		IDXGIOutput* pOutput;

		for (uint32 dwOutput = 0; pAdapter->EnumOutputs(dwOutput, &pOutput) != DXGI_ERROR_NOT_FOUND; dwOutput++)
		{
			DXGI_OUTPUT_DESC outputDesc;
			hResult = pOutput->GetDesc(&outputDesc);
			
			D3DOutputInfo outputInfo(&outputDesc);
			
			if (FAILED(hResult))
				continue;

			if (!IsOutputSupported(&outputDesc))
			{
				pOutput->Release();
				continue;
			}

			uint32 dwModeCount = 0;

			int nFormatOverride = g_nLauncher_D3D11_Format != 1 ? g_nLauncher_D3D11_Format : -1;
			nFormatOverride = g_CV_D3D11_Format.m_Val != 1 ? g_CV_D3D11_Format.m_Val : nFormatOverride;
			DXGI_FORMAT eFormat = nFormatOverride != -1 ? (DXGI_FORMAT)nFormatOverride : DXGI_FORMAT_B8G8R8A8_UNORM;
			pOutput->GetDisplayModeList(eFormat, DXGI_ENUM_MODES_INTERLACED | DXGI_ENUM_MODES_SCALING, &dwModeCount, nullptr);

			DXGI_MODE_DESC* pModeList = new DXGI_MODE_DESC[dwModeCount];
			pOutput->GetDisplayModeList(eFormat, DXGI_ENUM_MODES_INTERLACED | DXGI_ENUM_MODES_SCALING, &dwModeCount, pModeList);

			for (uint32 dwMode = 0; dwMode < dwModeCount; dwMode++)
			{
				if (!IsModeSupported(&pModeList[dwMode]))
					continue;
				
				outputInfo.m_Modes.emplace_back(&pModeList[dwMode], DXGI_FORMAT_B8G8R8A8_UNORM);
			}

			delete[] pModeList;

			adapterInfo.m_Outputs.push_back(outputInfo);
			pOutput->Release();
		}

		m_AdapterList.push_back(adapterInfo);
		pAdapter->Release();
	}

	if (!m_AdapterList.size())
		return false;

	return true;
}

void CD3D_Shell::ListDevices()
{
	for (const auto& adapterInfo : m_AdapterList)
	{
		for (const auto& outputInfo : adapterInfo.m_Outputs)
			AddConsoleMessage("Device: %s (%s)", adapterInfo.m_szDesc, outputInfo.m_szDeviceName);
	}
}

void CD3D_Shell::GetSupportedModes(RMode*& pGlobalModeList)
{
	for (const auto& adapterInfo : m_AdapterList)
	{
		for (const auto& outputInfo : adapterInfo.m_Outputs)
		{
			Array_ModeInfo usedModes;
			
			for (const auto& modeInfo : outputInfo.m_Modes)
			{	
				auto lambdaFindMode = [modeInfo](D3DModeInfo& mode) { return mode.m_dwWidth == modeInfo.m_dwWidth && mode.m_dwHeight == modeInfo.m_dwHeight; };			
				if (std::find_if(usedModes.begin(), usedModes.end(), lambdaFindMode) != usedModes.end())
					continue;
				
				RMode* pMode = new RMode();

				pMode->m_dwWidth = modeInfo.m_dwWidth;
				pMode->m_dwHeight = modeInfo.m_dwHeight;
				pMode->m_dwBitDepth = 32;

				strncpy_s(pMode->m_szInternalName, outputInfo.m_szDeviceName, sizeof(pMode->m_szInternalName));
				strncpy_s(pMode->m_szDescription, adapterInfo.m_szDesc, sizeof(pMode->m_szDescription));

				pMode->m_bHardware = TRUE;

				pMode->m_pNext = pGlobalModeList;
				pGlobalModeList = pMode;

				usedModes.push_back(modeInfo);
			}
		}
	}
}

IDXGIAdapter1* CD3D_Shell::GetDXGIAdapter(D3DAdapterInfo* pAdapterInfo)
{
	IDXGIAdapter1* pAdapter;

	for (uint32 dwAdapter = 0; m_pFactory->EnumAdapters1(dwAdapter, &pAdapter) != DXGI_ERROR_NOT_FOUND; dwAdapter++)
	{
		DXGI_ADAPTER_DESC1 adapterDesc;
		HRESULT hResult = pAdapter->GetDesc1(&adapterDesc);

		if (FAILED(hResult))
			continue;

		size_t nLen;
		char szDesc[D3D_SHELL_ADAPTER_DESC_LEN];
		wcstombs_s(&nLen, szDesc, D3D_SHELL_ADAPTER_DESC_LEN, adapterDesc.Description, D3D_SHELL_ADAPTER_DESC_LEN);

		if (!strcmp(szDesc, pAdapterInfo->m_szDesc))
			return pAdapter;

		pAdapter->Release();
	}

	return nullptr;
}

IDXGIOutput* CD3D_Shell::GetDXGIOutput(D3DAdapterInfo* pAdapterInfo, D3DOutputInfo* pOutputInfo)
{
	IDXGIAdapter1* pAdapter = GetDXGIAdapter(pAdapterInfo);

	if (pAdapter != nullptr)
	{
		IDXGIOutput* pOutput;

		for (uint32 dwOutput = 0; pAdapter->EnumOutputs(dwOutput, &pOutput) != DXGI_ERROR_NOT_FOUND; dwOutput++)
		{
			DXGI_OUTPUT_DESC outputDesc;
			HRESULT hResult = pOutput->GetDesc(&outputDesc);

			if (FAILED(hResult))
				continue;

			size_t nLen;
			char szDevice[D3D_SHELL_OUTPUT_DEVICE_LEN];
			wcstombs_s(&nLen, szDevice, D3D_SHELL_OUTPUT_DEVICE_LEN, outputDesc.DeviceName, D3D_SHELL_OUTPUT_DEVICE_LEN);

			if (!strcmp(szDevice, pOutputInfo->m_szDeviceName))
			{
				pAdapter->Release();
				return pOutput;
			}

			pOutput->Release();
		}
	}

	pAdapter->Release();
	return nullptr;
}

D3DAdapterInfo* CD3D_Shell::PickDefaultAdapter(RMode* pMode)
{	
	if (!*m_szAdapterOverride)
	{
		auto lambdaFindAdapter = [pMode](D3DAdapterInfo& adapter) { return !strcmp(adapter.m_szDesc, pMode->m_szInternalName); };
		auto iter = std::find_if(m_AdapterList.begin(), m_AdapterList.end(), lambdaFindAdapter);

		if (iter != m_AdapterList.end())
			return &(*iter);

		return &m_AdapterList.front();
	}
	else
	{
		auto lambdaFindAdapter = [this](D3DAdapterInfo& adapter) { return !strcmp(adapter.m_szDesc, m_szAdapterOverride); };
		auto iter = std::find_if(m_AdapterList.begin(), m_AdapterList.end(), lambdaFindAdapter);

		return iter != m_AdapterList.end() ? &(*iter) : nullptr;
	}
}

void CD3D_Shell::PickDefaultMode(RMode* pMode, D3DAdapterInfo* pAdapterInfo, D3DOutputInfo* pOutputInfo, D3DModeInfo** ppModeInfo)
{
	for (auto& modeInfo : pOutputInfo->m_Modes)
	{
		float fRefreshRate = D3DModeInfo::GetRefreshRate(modeInfo.m_dwNumerator, modeInfo.m_dwDenominator);

		if (pMode->m_dwWidth == modeInfo.m_dwWidth &&
			pMode->m_dwHeight == modeInfo.m_dwHeight &&
			Float_NearlyEquals(g_CV_D3D11_RefreshRate.m_Val, fRefreshRate, 0.1f))
		{
			*ppModeInfo = &modeInfo;
			return;
		}
	}
}

void CD3D_Shell::PickDefaultMode(RMode* pMode, D3DAdapterInfo* pAdapterInfo, D3DOutputInfo** ppOutputInfo, D3DModeInfo** ppModeInfo)
{	
	if (*m_szOutputOverride)
	{
		auto lambdaFindOutput = [pMode, this](D3DOutputInfo& output) { return !strcmp(output.m_szDeviceName, m_szOutputOverride); };
		auto iter = std::find_if(pAdapterInfo->m_Outputs.begin(), pAdapterInfo->m_Outputs.end(), lambdaFindOutput);

		if (iter != pAdapterInfo->m_Outputs.end())
		{
			D3DOutputInfo* pOutputInfo = &(*iter);
			PickDefaultMode(pMode, pAdapterInfo, pOutputInfo, ppModeInfo);

			if (*ppModeInfo != nullptr)
			{
				*ppOutputInfo = pOutputInfo;
				return;
			}
		}
	}

	for (auto& outputInfo : pAdapterInfo->m_Outputs)
	{
		if (!strcmp(outputInfo.m_szDeviceName, pMode->m_szInternalName) || !*pMode->m_szInternalName)
		{
			PickDefaultMode(pMode, pAdapterInfo, &outputInfo, ppModeInfo);
			if (*ppModeInfo != nullptr)
			{
				*ppOutputInfo = &outputInfo;
				return;
			}
		}
	}
}