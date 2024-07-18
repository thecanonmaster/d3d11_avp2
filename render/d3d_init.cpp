#include "pch.h"

#include "d3d_init.h"
#include "d3d_shell.h"
#include "rendererconsolevars.h"
#include "d3d_texture.h"
#include "globalmgr.h"
#include "common_stuff.h"
#include "draw_objects.h"
#include "common_draw.h"
#include "setup_model.h"

bool g_bInOptimized2D = false;

void d3d_ReadExtraConsoleVariables()
{
	if (g_CV_D3D11_Format.m_Val != -1 &&
		g_CV_D3D11_Format.m_Val != DXGI_FORMAT_B8G8R8A8_UNORM && g_CV_D3D11_Format.m_Val != DXGI_FORMAT_B8G8R8A8_UNORM_SRGB)
		g_CV_D3D11_Format.m_Val = DXGI_FORMAT_B8G8R8A8_UNORM;

	if (g_CV_NoLockOnFlip.m_Val)
		g_pStruct->RunConsoleString((char*)"LockOnFlip 0");

	if (g_CV_TraceConsole.m_Val)
		g_pStruct->RunConsoleString((char*)"TraceConsole 1");
}

void d3d_RebindLightmaps(RenderContext* pContext)
{
	g_GlobalMgr.GetLightMapMgr()->FreeAllData();

	g_GlobalMgr.GetLightMapMgr()->CreateLightmapPages(pContext->m_pMainWorld->m_LMAnims.m_pArray,
		pContext->m_pMainWorld->GetLMAnimCount());
}

RenderContext* d3d_CreateContext(RenderContextInit* pInit)
{
	RenderContext* pContext = (RenderContext*)calloc(1, sizeof(RenderContext));
	
	if (pContext == nullptr) 
		return nullptr;

	pContext->m_pMainWorld = pInit->m_pMainWorld;
	pContext->m_wCurFrameCode = UINT16_MAX;

	g_bNewRenderContext = true;

	return pContext;
}

void d3d_DeleteContext(RenderContext* pContext)
{
	if (pContext != nullptr)
		free(pContext);

	g_ModelSetup.FreeQueuedData();
	g_GlobalMgr.FreeObjectManagersData();
}

bool d3d_PostInitializeDevice(RenderStructInit* pInit)
{
	if (pInit == nullptr)
		return false;

	if (!g_GlobalMgr.PostInit())
		return false;

	d3d_InitObjectModules();

	return true;
}

void d3d_RenderCommand(int argc, char** argv)
{
	if (argc > 0)
	{
		if (_stricmp(argv[0], "LISTDEVICES") == 0)
		{
			g_D3DShell.ListDevices();
		}
		else if (_stricmp(argv[0], "LISTTEXTUREFORMATS") == 0)
		{
			NotImplementedMessage("d3d_RenderCommand - LISTTEXTUREFORMATS");
		}
		else if (_stricmp(argv[0], "LISTDEVICECAPS") == 0)
		{
			NotImplementedMessage("d3d_RenderCommand - LISTDEVICECAPS");
		}
		else if (_stricmp(argv[0], "FREETEXTURES") == 0)
		{
			g_GlobalMgr.GetTextureManager()->FreeAllTextures();
		}
	}
}
