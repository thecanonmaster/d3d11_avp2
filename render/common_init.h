#ifndef __COMMON_INIT_H__
#define __COMMON_INIT_H__

#include "common_stuff.h"

void rdll_FreeModeList(RMode* pModes);
RMode* rdll_GetSupportedModes();
void rdll_RenderDLLSetup(RenderStruct* pStruct);

extern HMODULE g_hModule;

#endif