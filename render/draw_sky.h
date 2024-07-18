#ifndef __D3D_DRAW_SKY_H__
#define __D3D_DRAW_SKY_H__

#ifndef __DIRECT_X_MATH_H__
#include <DirectXMath.h>
#define __DIRECT_X_MATH_H__
#endif

class ViewParams;

//void d3d_ExtendSkyBounds(ViewParams* pParams, float& fMinX, float& fMinY, float& fMaxX, float& fMaxY);
void d3d_DrawSkyExtents(ViewParams* pParams, float fMinX, float fMinY, float fMaxX, float fMaxY);

#endif
