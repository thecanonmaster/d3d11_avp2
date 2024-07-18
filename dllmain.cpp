#include "pch.h"

#include "render/common_init.h"

/*extern "C"
{
	__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}*/

BOOL APIENTRY DllMain(HANDLE hModule, DWORD dwReason, LPVOID pReserved)
{
	switch (dwReason)
	{
		case DLL_PROCESS_ATTACH:
		{
			ParseExtraCommandLine(GetCommandLine());
			Timer_Init();
#if defined(DEBUG) || defined(_DEBUG)
			Logger_Init();
#endif

			g_hModule = (HMODULE)hModule;
		}
		break;

		case DLL_PROCESS_DETACH:
		{
#if defined(DEBUG) || defined(_DEBUG)
			Logger_Free();
#endif
			Timer_Free();
		}
		break;
	}

	return TRUE;
}
