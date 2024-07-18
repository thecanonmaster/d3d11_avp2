#ifndef __D3D_INIT_H__
#define __D3D_INIT_H__

extern bool g_bInOptimized2D;

void d3d_ReadExtraConsoleVariables();
bool d3d_PostInitializeDevice(RenderStructInit* pInit);
void d3d_RebindLightmaps(RenderContext* pContext);
RenderContext* d3d_CreateContext(RenderContextInit* pInit);
void d3d_DeleteContext(RenderContext* pContext);
void d3d_RenderCommand(int argc, char** argv);

#endif