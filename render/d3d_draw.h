#ifndef __D3D_DRAW_H__
#define __D3D_DRAW_H__

#include "renderstatemgr.h"

void d3d_UploadDynamicLights();
void d3d_Clear(LTRect* pRect, uint32 dwFlags, LTVector* pClearColor);
int d3d_RenderScene(SceneDesc* pDesc);

void d3d_SetTranslucentObjectStates(bool bAdditive);
void d3d_UnsetTranslucentObjectStates(bool bChangeZ);

extern int g_nLastDrawMode;
extern float g_fLastFrameTime;
extern float g_fLastClientTime;

#endif