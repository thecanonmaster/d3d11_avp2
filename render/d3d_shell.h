#ifndef __D3D_SHELL_H__
#define __D3D_SHELL_H__

#ifndef __DXGI_H__
#include <dxgi.h>
#define __DXGI_H__
#endif

#include <vector>

#define D3D_SHELL_ADAPTER_DESC_LEN		128
#define D3D_SHELL_OUTPUT_DEVICE_LEN		32

struct D3DAdapterInfo;
struct D3DOutputInfo;
struct D3DModeInfo;

typedef std::vector<D3DAdapterInfo> Array_AdapterInfo;
typedef std::vector<D3DOutputInfo> Array_OutputInfo;
typedef std::vector<D3DModeInfo> Array_ModeInfo;

struct D3DAdapterInfo
{
	D3DAdapterInfo(DXGI_ADAPTER_DESC1* pInfo)
	{	
		m_dwVendorId = pInfo->VendorId;
		m_dwDeviceId = pInfo->DeviceId;

		m_dwDedicatedVideoMemory = pInfo->DedicatedVideoMemory;
		m_dwDedicatedSystemMemory = pInfo->DedicatedSystemMemory;
		m_dwSharedSystemMemory = pInfo->SharedSystemMemory;

		size_t nLen;
		wcstombs_s(&nLen, m_szDesc, D3D_SHELL_ADAPTER_DESC_LEN, pInfo->Description, D3D_SHELL_ADAPTER_DESC_LEN);
	}

	uint32	m_dwVendorId;
	uint32	m_dwDeviceId;

	uint32	m_dwDedicatedVideoMemory;
	uint32	m_dwDedicatedSystemMemory;
	uint32	m_dwSharedSystemMemory;

	char	m_szDesc[D3D_SHELL_ADAPTER_DESC_LEN];
	
	Array_OutputInfo	m_Outputs;
};

struct D3DOutputInfo
{
	D3DOutputInfo(DXGI_OUTPUT_DESC* pInfo)
	{
		size_t nLen;
		wcstombs_s(&nLen, m_szDeviceName, D3D_SHELL_OUTPUT_DEVICE_LEN, pInfo->DeviceName, D3D_SHELL_OUTPUT_DEVICE_LEN);
	}

	char	m_szDeviceName[D3D_SHELL_OUTPUT_DEVICE_LEN];

	Array_ModeInfo	m_Modes;
};

struct D3DModeInfo
{
	D3DModeInfo(DXGI_MODE_DESC* pInfo, DXGI_FORMAT eSCFormat)
	{
		m_dwFormat = pInfo->Format;
		m_dwSCFormat = eSCFormat;
		m_dwWidth = pInfo->Width;
		m_dwHeight = pInfo->Height;
		m_dwNumerator = pInfo->RefreshRate.Numerator;
		m_dwDenominator = pInfo->RefreshRate.Denominator;
		m_dwScaling = pInfo->Scaling;
		m_dwScanlineOrdering = pInfo->ScanlineOrdering;
	}

	static float GetRefreshRate(uint32 dwNumerator, uint32 dwDenominator)
	{
		return (float)dwNumerator / (float)dwDenominator;
	}

	float GetRefreshRate()
	{
		return (float)m_dwNumerator / (float)m_dwDenominator;
	}

	uint32 GetPitch()
	{
		return m_dwWidth << 2;
	}

	uint32	m_dwFormat;
	uint32	m_dwSCFormat;
	uint32	m_dwWidth;
	uint32	m_dwHeight;
	uint32	m_dwNumerator;
	uint32	m_dwDenominator;
	uint32	m_dwScaling;
	uint32	m_dwScanlineOrdering;
};

class CD3D_Shell
{

public:

	CD3D_Shell() { Reset(); }
	~CD3D_Shell() { FreeAll(); }

	bool	Create();
	void	Reset();
	void	FreeAll();

	bool	BuildDeviceList();
	void	ListDevices();
	void	GetSupportedModes(RMode*& pGlobalModeList);

	IDXGIFactory1*	GetDXGIFactory() { return m_pFactory; }
	IDXGIAdapter1*	GetDXGIAdapter(D3DAdapterInfo* pAdapterInfo);
	IDXGIOutput*	GetDXGIOutput(D3DAdapterInfo* pAdapterInfo, D3DOutputInfo* pOutputInfo);

	D3DAdapterInfo*	PickDefaultAdapter(RMode* pMode);

	void	PickDefaultMode(RMode* pMode, D3DAdapterInfo* pAdapterInfo, D3DOutputInfo* pOutputInfo, D3DModeInfo** ppModeInfo);
	void	PickDefaultMode(RMode* pMode, D3DAdapterInfo* pAdapterInfo, D3DOutputInfo** ppOutputInfo, D3DModeInfo** ppModeInfo);

	IDXGIFactory1*	m_pFactory;

	char	m_szAdapterOverride[D3D_SHELL_ADAPTER_DESC_LEN];
	char	m_szOutputOverride[D3D_SHELL_OUTPUT_DEVICE_LEN];

private:

	bool	IsAdapterSupported(DXGI_ADAPTER_DESC1* pDesc);
	bool	IsOutputSupported(DXGI_OUTPUT_DESC* pDesc);
	bool	IsModeSupported(DXGI_MODE_DESC* pDesc);

	Array_AdapterInfo	m_AdapterList;
};

extern CD3D_Shell g_D3DShell;

#endif