#ifndef __UTILS_COMMON_H__
#define __UTILS_COMMON_H__

#define EXP_FREE_MODE_LIST			"FreeModeList"
#define EXP_GET_SUPPORTED_MODES		"GetSupportedModes"
#define EXP_RENDER_DLL_SETUP		"RenderDLLSetup"

#define EXCMD_LAUNCHER_DXGI_MODE_SCALING	"+Launcher_D3D11_ModeScaling="
#define EXCMD_LAUNCHER_DXGI_FORMAT			"+Launcher_D3D11_Format="
extern int g_nLauncher_D3D11_Format;
extern int g_nLauncher_D3D11_ModeScaling;

void Timer_Init();
void Timer_Free();
float Timer_GetTime();

double Timer_GetTimeMM();
void Timer_MeasurementStartMM();
double Timer_MeasurementEndMM();

void ParseExtraCommandLine(char* szCommandLine);

#endif