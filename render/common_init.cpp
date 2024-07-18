#include "pch.h"

#include "common_init.h"
#include "d3d_shell.h"
#include "d3d_init.h"
#include "d3d_device.h"
#include "rendererconsolevars.h"
#include "d3d_draw.h"
#include "d3d_surface.h"
#include "d3d_optimizedsurface.h"
#include "d3d_texture.h"
#include "d3d_directdraw.h"
#include "tagnodes.h"
#include <mutex>
#include "draw_objects.h"
#include "globalmgr.h"

std::mutex g_RenderMutex;
RMode* g_pModeList;
HMODULE g_hModule = nullptr;

///
/// MUTEX_TESTS - START
///

int	d3d_Init(RenderStructInit* pInit);
void d3d_Term();

int safe_d3d_Init(RenderStructInit* pInit)
{
	std::lock_guard<std::mutex> guard(g_RenderMutex);
	return d3d_Init(pInit);
}

void safe_d3d_Term()
{
	std::lock_guard<std::mutex> guard(g_RenderMutex);
	d3d_Term();
}

void safe_d3d_BindTexture(SharedTexture* pSharedTexture, LTBOOL bTextureChanged)
{
	std::lock_guard<std::mutex> guard(g_RenderMutex);
	d3d_BindTexture(pSharedTexture, bTextureChanged);
}

void safe_d3d_UnbindTexture(SharedTexture* pSharedTexture)
{
	std::lock_guard<std::mutex> guard(g_RenderMutex);
	d3d_UnbindTexture(pSharedTexture);
}

void safe_d3d_RebindLightmaps(RenderContext* pContext)
{
	std::lock_guard<std::mutex> guard(g_RenderMutex);
	d3d_RebindLightmaps(pContext);
}

RenderContext* safe_d3d_CreateContext(RenderContextInit* pInit)
{
	std::lock_guard<std::mutex> guard(g_RenderMutex);
	return d3d_CreateContext(pInit);
}

void safe_d3d_DeleteContext(RenderContext* pContext)
{
	std::lock_guard<std::mutex> guard(g_RenderMutex);
	d3d_DeleteContext(pContext);
}

void safe_d3d_Clear(LTRect* pRect, uint32 dwFlags, LTVector* pClearColor)
{
	std::lock_guard<std::mutex> guard(g_RenderMutex);
	d3d_Clear(pRect, dwFlags, pClearColor);
}

LTBOOL safe_d3d_Start3D()
{
	std::lock_guard<std::mutex> guard(g_RenderMutex);
	return CD3D_Device::Start3D();
}

LTBOOL safe_d3d_End3D()
{
	std::lock_guard<std::mutex> guard(g_RenderMutex);
	return CD3D_Device::End3D();
}

LTBOOL safe_d3d_IsIn3D()
{
	std::lock_guard<std::mutex> guard(g_RenderMutex);
	return CD3D_Device::IsIn3D();
}

LTBOOL safe_d3d_StartOptimized2D()
{
	std::lock_guard<std::mutex> guard(g_RenderMutex);
	return d3d_StartOptimized2D();
}

void safe_d3d_EndOptimized2D()
{
	std::lock_guard<std::mutex> guard(g_RenderMutex);
	d3d_EndOptimized2D();
}
LTBOOL safe_d3d_IsInOptimized2D()
{
	std::lock_guard<std::mutex> guard(g_RenderMutex);
	return d3d_IsInOptimized2D();
}

LTBOOL safe_d3d_SetOptimized2DBlend(LTSurfaceBlend eBlend)
{
	std::lock_guard<std::mutex> guard(g_RenderMutex);
	return d3d_SetOptimized2DBlend(eBlend);
}

LTBOOL safe_d3d_GetOptimized2DBlend(LTSurfaceBlend& eBlend)
{
	std::lock_guard<std::mutex> guard(g_RenderMutex);
	return d3d_GetOptimized2DBlend(eBlend);
}

LTBOOL safe_d3d_SetOptimized2DColor(HLTCOLOR dwColor)
{
	std::lock_guard<std::mutex> guard(g_RenderMutex);
	return d3d_SetOptimized2DColor(dwColor);
}

LTBOOL safe_d3d_GetOptimized2DColor(HLTCOLOR& dwColor)
{
	std::lock_guard<std::mutex> guard(g_RenderMutex);
	return d3d_SetOptimized2DColor(dwColor);
}

int	safe_d3d_RenderScene(SceneDesc* pScene)
{
	std::lock_guard<std::mutex> guard(g_RenderMutex);
	return d3d_RenderScene(pScene);
}

void safe_d3d_RenderCommand(int argc, char** argv)
{
	std::lock_guard<std::mutex> guard(g_RenderMutex);
	d3d_RenderCommand(argc, argv);
}

void* safe_d3d_GetDirectDrawInterface(const char* szQuery)
{
	std::lock_guard<std::mutex> guard(g_RenderMutex);
	return d3d_GetDirectDrawInterface(szQuery);
}

void safe_d3d_SwapBuffers(uint32 dwFlags)
{
	std::lock_guard<std::mutex> guard(g_RenderMutex);
	d3d_SwapBuffers(dwFlags);
}

int	safe_d3d_GetInfoFlags()
{
	std::lock_guard<std::mutex> guard(g_RenderMutex);
	return 0;
}

void safe_d3d_GetScreenFormat(PFormat* pFormat)
{
	std::lock_guard<std::mutex> guard(g_RenderMutex);
	d3d_GetScreenFormat(pFormat);
}

HLTBUFFER safe_d3d_CreateSurface(int nWidth, int nHeight)
{
	std::lock_guard<std::mutex> guard(g_RenderMutex);
	return d3d_CreateSurface(nWidth, nHeight);
}

void safe_d3d_DeleteSurface(HLTBUFFER hSurface)
{
	std::lock_guard<std::mutex> guard(g_RenderMutex);
	d3d_DeleteSurface(hSurface);
}

void safe_d3d_GetSurfaceInfo(HLTBUFFER hSurface, uint32* pWidth, uint32* pHeight, int* pPitch)
{
	std::lock_guard<std::mutex> guard(g_RenderMutex);
	d3d_GetSurfaceInfo(hSurface, pWidth, pHeight, pPitch);
}

void* safe_d3d_LockSurface(HLTBUFFER hSurface)
{
	std::lock_guard<std::mutex> guard(g_RenderMutex);
	return d3d_LockSurface(hSurface);
}

void safe_d3d_UnlockSurface(HLTBUFFER hSurface)
{
	std::lock_guard<std::mutex> guard(g_RenderMutex);
	d3d_UnlockSurface(hSurface);
}

LTBOOL safe_d3d_OptimizeSurface(HLTBUFFER hSurface, GenericColor dwTransparentColor)
{
	std::lock_guard<std::mutex> guard(g_RenderMutex);
	return d3d_OptimizeSurface(hSurface, dwTransparentColor);
}

void safe_d3d_UnoptimizeSurface(HLTBUFFER hSurface)
{
	std::lock_guard<std::mutex> guard(g_RenderMutex);
	return d3d_UnoptimizeSurface(hSurface);
}

LTBOOL safe_d3d_LockScreen(int nLeft, int nTop, int nRight, int nBottom, void** pData, int* pPitch)
{
	std::lock_guard<std::mutex> guard(g_RenderMutex);

	switch (g_GlobalMgr.GetSolidDrawingMode())
	{
		case SDM_DoubleCopy: return d3d_LockScreen(nLeft, nTop, nRight, nBottom, pData, pPitch);
		case SDM_DoubleMenuFastGame: 
			return g_nLastDrawMode == DRAWMODE_NORMAL ? d3d_LockScreen_Fast(nLeft, nTop, nRight, nBottom, pData, pPitch) : 
				d3d_LockScreen(nLeft, nTop, nRight, nBottom, pData, pPitch);

		case SDM_SingleMenuFastGame:
			return g_nLastDrawMode == DRAWMODE_NORMAL ? d3d_LockScreen_Fast(nLeft, nTop, nRight, nBottom, pData, pPitch) :
				d3d_LockScreen_New(nLeft, nTop, nRight, nBottom, pData, pPitch);

		case SDM_Fast:
			return d3d_LockScreen_Fast(nLeft, nTop, nRight, nBottom, pData, pPitch);

		case SDM_SingleCopy: 
		default: return d3d_LockScreen_New(nLeft, nTop, nRight, nBottom, pData, pPitch);
	}
}
void safe_d3d_UnlockScreen()
{
	std::lock_guard<std::mutex> guard(g_RenderMutex);
	
	switch (g_GlobalMgr.GetSolidDrawingMode())
	{
		case SDM_DoubleCopy: d3d_UnlockScreen(); return;
		case SDM_DoubleMenuFastGame: g_nLastDrawMode == DRAWMODE_NORMAL ? d3d_UnlockScreen_Fast() : d3d_UnlockScreen(); return;
		case SDM_SingleMenuFastGame: g_nLastDrawMode == DRAWMODE_NORMAL ? d3d_UnlockScreen_Fast() : d3d_UnlockScreen_New(); return;
		case SDM_Fast: d3d_UnlockScreen_Fast(); return;
		
		case SDM_SingleCopy: 
		default: d3d_UnlockScreen_New(); return;
	}
}

void safe_d3d_BlitToScreen(BlitRequest* pRequest)
{
	std::lock_guard<std::mutex> guard(g_RenderMutex);
	d3d_BlitToScreen(pRequest);
}

LTBOOL safe_d3d_WarpToScreen(BlitRequest* pRequest)
{
	std::lock_guard<std::mutex> guard(g_RenderMutex);
	return d3d_WarpToScreen(pRequest);
}

void safe_d3d_MakeScreenShot(const char* szFilename)
{
	std::lock_guard<std::mutex> guard(g_RenderMutex);
	d3d_MakeScreenShot(szFilename);
}

void safe_d3d_ReadConsoleVariables()
{
	std::lock_guard<std::mutex> guard(g_RenderMutex);
	d3d_ReadConsoleVariables();
}

void safe_d3d_BlitFromScreen(BlitRequest* pRequest)
{
	std::lock_guard<std::mutex> guard(g_RenderMutex);
	d3d_BlitFromScreen(pRequest);
}

/// 
/// MUTEX_TESTS - END
///

typedef LRESULT(__stdcall* WindowHookFn_type)(HWND hWnd, UINT dwMsg, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK NewWindowHookFn(HWND hWnd, UINT dwMsg, WPARAM wParam, LPARAM lParam);

static WindowHookFn_type g_pWindowHookFn = nullptr;

static LRESULT CALLBACK NewWindowHookFn(HWND hWnd, UINT dwMsg, WPARAM wParam, LPARAM lParam)
{
	if (dwMsg == WM_ACTIVATEAPP && (g_CV_LoseFocusNoShutdown.m_Val && g_bRunWindowed))
		return TRUE;
	else
		return g_pWindowHookFn != nullptr ? g_pWindowHookFn(hWnd, dwMsg, wParam, lParam) : TRUE;
}

void d3d_Term()
{
	d3d_TermObjectModules();

	if (g_CV_LoseFocusNoShutdown.m_Val && g_bRunWindowed)
	{
		WindowHookFn_type pWindowHookShellFn = (WindowHookFn_type)GetWindowLong(g_hWnd, GWL_WNDPROC);

		if (pWindowHookShellFn == nullptr || pWindowHookShellFn == NewWindowHookFn)
		{
			SetWindowLong(g_hWnd, GWL_WNDPROC, (LONG)g_pWindowHookFn);
		}
		else if (pWindowHookShellFn != nullptr)
		{
			char szBuffer[64];
			sprintf_s(szBuffer, sizeof(szBuffer) - 1, "D3D11_ShellEngineHook %d", (LONG)g_pWindowHookFn);

			g_pStruct->RunConsoleString(szBuffer);
		}
	}

	g_D3DDevice.FreeAll(); 
	g_D3DShell.FreeAll();
}

int d3d_Init(RenderStructInit* pInit)
{
	pInit->m_nRendererVersion = LTRENDER_VERSION;
	
	d3d_CreateConsoleVariables();
	d3d_ReadConsoleVariables();

	HLTPARAM hWindowed = g_pStruct->GetParameter((char*)"windowed");
	g_bRunWindowed = hWindowed != nullptr ? g_pStruct->GetParameterValueFloat(hWindowed) > 0.0f : false;

	HLTPARAM hAdapterOverride = g_pStruct->GetParameter((char*)"D3D11_Adapter");
	if (hAdapterOverride != nullptr)
		strncpy_s(g_D3DShell.m_szAdapterOverride, g_pStruct->GetParameterValueString(hAdapterOverride), sizeof(g_D3DShell.m_szAdapterOverride));

	HLTPARAM hOutputOverride = g_pStruct->GetParameter((char*)"D3D11_Output");
	if (hOutputOverride != nullptr)
		strncpy_s(g_D3DShell.m_szOutputOverride, g_pStruct->GetParameterValueString(hOutputOverride), sizeof(g_D3DShell.m_szOutputOverride));

	g_hWnd = pInit->m_hWnd;

	if (!g_D3DShell.Create())
		return RENDER_ERROR;

	D3DAdapterInfo* pAdapterInfo = g_D3DShell.PickDefaultAdapter(&pInit->m_Mode);
	D3DOutputInfo* pOutputInfo = nullptr;
	D3DModeInfo* pModeInfo = nullptr;

	if (pAdapterInfo == nullptr)
	{
		d3d_Term();
		AddDebugMessage(0, "Can't find any D3D adapters to use!");
		return RENDER_ERROR;
	}

	g_D3DShell.PickDefaultMode(&pInit->m_Mode, pAdapterInfo, &pOutputInfo, &pModeInfo);

	if (pModeInfo == nullptr)
	{
		d3d_Term();
		AddDebugMessage(0, "Can't find an appropriate D3D mode!");
		return RENDER_ERROR;
	}

	if (!g_D3DDevice.CreateDevice(pAdapterInfo, pOutputInfo, pModeInfo))
	{
		d3d_Term();
		AddDebugMessage(0, "Can't create D3D device!");
		return RENDER_ERROR;
	}

	strncpy_s(pInit->m_Mode.m_szInternalName, pOutputInfo->m_szDeviceName, sizeof(pInit->m_Mode.m_szInternalName));
	strncpy_s(pInit->m_Mode.m_szDescription, pAdapterInfo->m_szDesc, sizeof(pInit->m_Mode.m_szDescription));

	AddDebugMessage(0, "Using D3D device %s (%s)", pAdapterInfo->m_szDesc, pOutputInfo->m_szDeviceName);

	if (g_CV_LoseFocusNoShutdown.m_Val && g_bRunWindowed)
	{
		HLTPARAM hEngineWindowHook = g_pStruct->GetParameter((char*)"D3D11_ShellEngineHook");
		LONG lAddress = hWindowed != nullptr ? (LONG)g_pStruct->GetParameterValueFloat(hEngineWindowHook) : 0;

		if (!lAddress)
		{
			LONG lResult = SetWindowLong(g_hWnd, GWL_WNDPROC, (LONG)NewWindowHookFn);
			if (lResult)
				g_pWindowHookFn = (WindowHookFn_type)lResult;
		}
		else
		{
			g_pWindowHookFn = (WindowHookFn_type)lAddress;
		}
	}

	if (g_bRunWindowed)
	{
		RECT rcWindow, rcScreen;

		//GetWindowRect(GetDesktopWindow(), &rcScreen);
		rcScreen = { 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)};

		rcWindow.left = ((rcScreen.right - rcScreen.left) - pInit->m_Mode.m_dwWidth) >> 1;
		rcWindow.top = ((rcScreen.bottom - rcScreen.top) - pInit->m_Mode.m_dwHeight) >> 1;
		rcWindow.right = rcWindow.left + pInit->m_Mode.m_dwWidth;
		rcWindow.bottom = rcWindow.top + pInit->m_Mode.m_dwHeight;

		AdjustWindowRect(&rcWindow, GetWindowLong(g_hWnd, GWL_STYLE), false);

		if (rcWindow.left < 0)
		{
			rcWindow.right -= rcWindow.left;
			rcWindow.left = 0;
		}
		if (rcWindow.top < 0)
		{
			rcWindow.bottom -= rcWindow.top;
			rcWindow.top = 0;
		}

		SetWindowPos(g_hWnd, 0, rcWindow.left, rcWindow.top, rcWindow.right - rcWindow.left,
			rcWindow.bottom - rcWindow.top, SWP_NOREPOSITION);
	}

	g_D3DShell.GetDXGIFactory()->MakeWindowAssociation(g_hWnd, DXGI_MWA_NO_WINDOW_CHANGES | DXGI_MWA_NO_ALT_ENTER);

	if (!d3d_PostInitializeDevice(pInit))
		return RENDER_ERROR;

	d3d_GetVisibleSet()->Init();

	return RENDER_OK;
}

int d3d_GetInfoFlags()
{
	return 0;
}

void rdll_FreeModeList(RMode* pModes)
{
	RMode* pCur = pModes;
	while (pCur != nullptr)
	{
		RMode* pNext = pCur->m_pNext;
		delete pCur;
		pCur = pNext;
	}
}

RMode* rdll_GetSupportedModes()
{		
	if (g_D3DDevice.GetDevice() == nullptr)
		g_D3DShell.Create();

	g_pModeList = nullptr;
	g_D3DShell.GetSupportedModes(g_pModeList);
	return g_pModeList;
}

void rdll_RenderDLLSetup(RenderStruct* pStruct)
{
	g_pStruct = pStruct;

	g_pStruct->Init = safe_d3d_Init;
	g_pStruct->Term = safe_d3d_Term;

	g_pStruct->BindTexture = d3d_BindTexture;
	g_pStruct->UnbindTexture = d3d_UnbindTexture;

	g_pStruct->RebindLightmaps = safe_d3d_RebindLightmaps;

	g_pStruct->CreateContext = d3d_CreateContext;
	g_pStruct->DeleteContext = d3d_DeleteContext;

	g_pStruct->Clear = safe_d3d_Clear;

	g_pStruct->Start3D = safe_d3d_Start3D;
	g_pStruct->End3D = safe_d3d_End3D;
	g_pStruct->IsIn3D = safe_d3d_IsIn3D;

	g_pStruct->StartOptimized2D = safe_d3d_StartOptimized2D;
	g_pStruct->EndOptimized2D = safe_d3d_EndOptimized2D;
	g_pStruct->IsInOptimized2D = d3d_IsInOptimized2D;

	g_pStruct->SetOptimized2DBlend = d3d_SetOptimized2DBlend;
	g_pStruct->GetOptimized2DBlend = d3d_GetOptimized2DBlend;
	g_pStruct->SetOptimized2DColor = d3d_SetOptimized2DColor;
	g_pStruct->GetOptimized2DColor = d3d_GetOptimized2DColor;

	g_pStruct->RenderScene = safe_d3d_RenderScene;

	g_pStruct->RenderCommand = d3d_RenderCommand;

	g_pStruct->GetDirectDrawInterface = safe_d3d_GetDirectDrawInterface;

	g_pStruct->SwapBuffers = safe_d3d_SwapBuffers;

	g_pStruct->GetInfoFlags = d3d_GetInfoFlags;

	g_pStruct->GetScreenFormat = d3d_GetScreenFormat;

	g_pStruct->CreateSurface = safe_d3d_CreateSurface;
	g_pStruct->DeleteSurface = safe_d3d_DeleteSurface;

	g_pStruct->GetSurfaceInfo = d3d_GetSurfaceInfo;

	g_pStruct->LockSurface = safe_d3d_LockSurface;
	g_pStruct->UnlockSurface = safe_d3d_UnlockSurface;

	g_pStruct->OptimizeSurface = safe_d3d_OptimizeSurface;
	g_pStruct->UnoptimizeSurface = d3d_UnoptimizeSurface;

	g_pStruct->LockScreen = safe_d3d_LockScreen;
	g_pStruct->UnlockScreen = safe_d3d_UnlockScreen;

	g_pStruct->BlitToScreen = safe_d3d_BlitToScreen;
	g_pStruct->WarpToScreen = safe_d3d_WarpToScreen;

	g_pStruct->MakeScreenShot = safe_d3d_MakeScreenShot;

	g_pStruct->ReadConsoleVariables = d3d_ReadConsoleVariables;

	g_pStruct->BlitFromScreen = safe_d3d_BlitFromScreen;
}
