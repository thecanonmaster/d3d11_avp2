#include "pch.h"
#include "common.h"
#include <string>

uint32 g_dwStartTimeMS = 0;

int g_nLauncher_D3D11_Format = -1;
int g_nLauncher_D3D11_ModeScaling = -1;

LARGE_INTEGER g_liStartTime;
LARGE_INTEGER g_liFrequency;

double g_fSavedTime = 0.0;

void Timer_Init()
{
	timeBeginPeriod(1);

	QueryPerformanceFrequency(&g_liFrequency);
	QueryPerformanceCounter(&g_liStartTime);

	g_dwStartTimeMS = timeGetTime();
}

void Timer_Free()
{
	timeEndPeriod(1);
}

float Timer_GetTime()
{
	return (float)(timeGetTime() - g_dwStartTimeMS) / 1000.0f;
}

double Timer_GetTimeMM()
{
	LARGE_INTEGER liCurrent;

	QueryPerformanceCounter(&liCurrent);

	LONGLONG ddwTimeDiff = liCurrent.QuadPart - g_liStartTime.QuadPart;

	return (double)ddwTimeDiff / (double)g_liFrequency.QuadPart;
}

void Timer_MeasurementStartMM()
{
	g_fSavedTime = Timer_GetTimeMM();
}

double Timer_MeasurementEndMM()
{
	return Timer_GetTimeMM() - g_fSavedTime;
}

uint32 ParseExtraInt(char* szStart)
{
	int nValueLen = 0;
	while (*(szStart + nValueLen) != ' ' && *(szStart + nValueLen) != 0)
		nValueLen++;

	char szBuffer[32];
	strncpy_s(szBuffer, szStart, nValueLen);
	szBuffer[nValueLen] = 0;

	return atoi(szBuffer);
}

void ParseExtraCommandLine(char* szCommandLine)
{
	char* szStart = strstr(szCommandLine, EXCMD_LAUNCHER_DXGI_FORMAT);
	if (szStart != nullptr)
	{
		constexpr int nCommandLen = sizeof(EXCMD_LAUNCHER_DXGI_FORMAT);
		szStart += nCommandLen - 1;

		g_nLauncher_D3D11_Format = ParseExtraInt(szStart);
	}

	szStart = strstr(szCommandLine, EXCMD_LAUNCHER_DXGI_MODE_SCALING);
	if (szStart != nullptr)
	{
		constexpr int nCommandLen = sizeof(EXCMD_LAUNCHER_DXGI_MODE_SCALING);
		szStart += nCommandLen - 1;

		g_nLauncher_D3D11_ModeScaling = ParseExtraInt(szStart);
	}
}
