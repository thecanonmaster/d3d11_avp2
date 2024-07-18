#include "pch.h"

#include "common_stuff.h"
#include "rendererconsolevars.h"
#include "d3d_init.h"
#include "common_init.h"
#include "resource.h"

#define CONSOLE_MSG_BUFFER_LEN	512

RenderStruct* g_pStruct = nullptr;
HWND g_hWnd = nullptr;
bool g_bRunWindowed = false;

BaseConVar* g_pConVars = nullptr;

bool GetResourcePointer(const char* szType, uint32 dwResource, uint32* pSize, void** ppData)
{
	HRSRC hResource = FindResource(g_hModule, MAKEINTRESOURCE(dwResource), szType);

	if (hResource == nullptr)
		return false;

	HGLOBAL hMemory = LoadResource(g_hModule, hResource);

	if (hMemory == nullptr)
		return false;

	*pSize = SizeofResource(g_hModule, hResource);
	*ppData = LockResource(hMemory);

	return true;
}

void NotImplementedMessage(const char* szMsg)
{
	char szBuffer[CONSOLE_MSG_BUFFER_LEN] = "Not implemented: ";

	strcat_s(szBuffer, szMsg);
	
	g_pStruct->ConsolePrint(szBuffer);

	size_t nLen = strlen(szBuffer);
	if (szBuffer[nLen - 1] != '\n')
	{
		szBuffer[nLen] = '\n';
		szBuffer[nLen + 1] = '\0';
	}

	if (!g_CV_TraceConsole.m_Val)
		OutputDebugString(szBuffer);

#if defined(DEBUG) || defined(_DEBUG)
	if (g_CV_RenderDebugFile.m_Val)
		Logger_Print(szBuffer);
#endif
}

void AddDebugMessage(uint32 dwDebugLevel, const char* szMsg, ...)
{
	if (dwDebugLevel <= (uint32)g_CV_RenderDebug.m_Val)
	{
		va_list marker;
		va_start(marker, szMsg);

		char szBuffer[CONSOLE_MSG_BUFFER_LEN];
		vsprintf_s(szBuffer, szMsg, marker);
		va_end(marker);

		g_pStruct->ConsolePrint(szBuffer);

		size_t nLen = strlen(szBuffer);
		if (szBuffer[nLen - 1] != '\n')
		{
			szBuffer[nLen] = '\n';
			szBuffer[nLen + 1] = '\0';
		}

		if (!g_CV_TraceConsole.m_Val)
			OutputDebugString(szBuffer);

#if defined(DEBUG) || defined(_DEBUG)
		if (g_CV_RenderDebugFile.m_Val)
			Logger_Print(szBuffer);
#endif
	}
}

void AddObjectInfoMessage(LTObject* pObject)
{
	char szBuffer[CONSOLE_MSG_BUFFER_LEN];

	switch (pObject->m_nObjectType)
	{
		case OT_SPRITE:
		{
			sprintf_s(szBuffer, "SPRITE [%08x, RC: %d]: %s", (uint32)pObject, !!(pObject->m_dwFlags & FLAG_REALLYCLOSE),
				pObject->ToSprite()->m_SpriteTracker.m_pCurFrame->m_pTexture->m_pFile->m_szFilename);
			break;
		}

		case OT_MODEL:
		{
			sprintf_s(szBuffer, "MODEL [%08x, RC: %d]: %s", (uint32)pObject, !!(pObject->m_dwFlags & FLAG_REALLYCLOSE),
				pObject->ToModel()->GetModel()->m_szFilename);
			break;
		}
		
		default: return;
	}

	/*size_t nLen = strlen(szBuffer);
	if (szBuffer[nLen - 1] != '\n')
	{
		szBuffer[nLen] = '\n';
		szBuffer[nLen + 1] = '\0';
	}*/

	g_pStruct->ConsolePrint(szBuffer);

	if (!g_CV_TraceConsole.m_Val)
		OutputDebugString(szBuffer);
}

void AddConsoleMessage(const char* szMsg, ...)
{
	va_list marker;
	va_start(marker, szMsg);

	char szBuffer[CONSOLE_MSG_BUFFER_LEN];
	vsprintf_s(szBuffer, szMsg, marker);
	va_end(marker);

	g_pStruct->ConsolePrint(szBuffer);

	size_t nLen = strlen(szBuffer);
	if (szBuffer[nLen - 1] != '\n')
	{
		szBuffer[nLen] = '\n';
		szBuffer[nLen + 1] = '\0';
	}
}

static HLTPARAM d3d_MaybeCreateCVar(const char* szName, float fDefaultVal)
{
	HLTPARAM hRet;

	hRet = g_pStruct->GetParameter((char*)szName);
	if (hRet != nullptr)
	{
		return hRet;
	}
	else
	{
		char szBuffer[256];

		sprintf_s(szBuffer, "%s %f", szName, fDefaultVal);
		g_pStruct->RunConsoleString(szBuffer);
		return g_pStruct->GetParameter((char*)szName);
	}
}

void d3d_CreateConsoleVariables()
{
	BaseConVar* pCur;

	for (pCur = g_pConVars; pCur != nullptr; pCur = pCur->m_pNext)
		pCur->m_hParam = d3d_MaybeCreateCVar(pCur->m_szName, pCur->m_DefaultVal);
}

void d3d_ReadConsoleVariables()
{
	BaseConVar* pCur;

	for (pCur = g_pConVars; pCur != nullptr; pCur = pCur->m_pNext)
		pCur->SetFloat(g_pStruct->GetParameterValueFloat(pCur->m_hParam));

	d3d_ReadExtraConsoleVariables();
}
