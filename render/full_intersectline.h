#ifndef __FULL_INTERSECT_LINE_H__
#define __FULL_INTERSECT_LINE_H__

#ifndef __DIRECT_X_MATH_H__
#include <DirectXMath.h>
#define __DIRECT_X_MATH_H__
#endif

class XMIntersectQuery;
struct XMIntersectInfo;
struct WorldTree;

bool i_IntersectSegment(XMIntersectQuery* pQuery, XMIntersectInfo* pInfo, WorldTree* pWorldTree);

#endif
