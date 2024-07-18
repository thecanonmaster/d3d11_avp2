#ifndef __UTILS_LOGGER_H__
#define __UTILS_LOGGER_H__

#define D3D_LOG					".\\d3d.log"

void Logger_Init();
void Logger_Free();

void Logger_Print(const char* szMsg);
void Logger_TPrintF(const char* szMsg, ...);
void Logger_PrintF(const char* szMsg, ...);

#endif