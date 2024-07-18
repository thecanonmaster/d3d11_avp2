#ifndef __COMMON_STUFF_H__
#define __COMMON_STUFF_H__

extern bool g_bRunWindowed;

extern RenderStruct* g_pStruct;
extern HWND g_hWnd;

#define RELEASE_INTERFACE(res, ...) \
	if (res != nullptr) { \
		uint32 ref_count = res->Release(); \
		if (ref_count) AddDebugMessage(0, __VA_ARGS__, ref_count);\
		res = nullptr; } \

bool GetResourcePointer(const char* szType, uint32 dwResource, uint32* pSize, void** ppData);

void NotImplementedMessage(const char* szMsg);
void AddDebugMessage(uint32 dwDebugLevel, const char* szMsg, ...);
void AddObjectInfoMessage(LTObject* pObject);
void AddConsoleMessage(const char* szMsg, ...);

void d3d_CreateConsoleVariables();
void d3d_ReadConsoleVariables();

#endif