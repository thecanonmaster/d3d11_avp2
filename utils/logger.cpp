#include "pch.h"

FILE* g_pLogFile = NULL;
char g_szLogBuffer[8192];

void Logger_Init()
{
	fopen_s(&g_pLogFile, D3D_LOG, "w");
}

void Logger_Free()
{
	fclose(g_pLogFile);
}

void Logger_WriteBuffer(char* szBuffer)
{
	fputs(szBuffer, g_pLogFile);
	fflush(g_pLogFile);
}

void Logger_Print(const char* szMsg)
{
	fputs(szMsg, g_pLogFile);
	fflush(g_pLogFile);
}

void Logger_TPrintF(const char* szMsg, ...)
{	
	va_list argp;

	va_start(argp, szMsg);
	vsprintf_s(g_szLogBuffer, szMsg, argp);	
	va_end(argp);

	char szTimeBuffer[32];
	sprintf_s(szTimeBuffer, " [%g]\n", Timer_GetTime());
	strcat_s(g_szLogBuffer, szTimeBuffer);
	
	Logger_WriteBuffer(g_szLogBuffer);
}

void Logger_PrintF(const char* szMsg, ...)
{	
	va_list argp;
	
	va_start(argp, szMsg);
	vsprintf_s(g_szLogBuffer, szMsg, argp);	
	va_end(argp);
	
	Logger_WriteBuffer(g_szLogBuffer);
}