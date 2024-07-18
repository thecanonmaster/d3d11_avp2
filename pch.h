#ifndef PCH_H
#define PCH_H

//#define _XM_SSE4_INTRINSICS_
#define _XM_NO_INTRINSICS_

#pragma warning(push)
//#pragma warning(disable:26812)

#pragma warning(pop)

#include "framework.h"

#include "lithtech\codes.h"
#include "lithtech\basetypes.h"
#include "lithtech\statechange.h"
#include "lithtech\structbank.h"

#include "lithtech\objtypes\worldtree.h"
#include "lithtech\objtypes\base.h"
#include "lithtech\objtypes\sprite.h"
#include "lithtech\objtypes\model.h"
#include "lithtech\objtypes\light.h"
#include "lithtech\objtypes\camera.h"
#include "lithtech\objtypes\linesystem.h"
#include "lithtech\objtypes\particlesystem.h"
#include "lithtech\objtypes\canvas.h"
#include "lithtech\objtypes\polygrid.h"
#include "lithtech\objtypes\staticlight.h"
#include "lithtech\objtypes\worldmodel.h"
#include "lithtech\objtypes\terrainsection.h"
#include "lithtech\objtypes\container.h"

#include "lithtech\textypes.h"
#include "lithtech\surftypes.h"
#include "lithtech\mainworld.h"
#include "lithtech\rendertypes.h"
#include "lithtech\types.h"

#include "utils\common.h"
#include "utils\logger.h"

typedef std::vector<uint16> Array_UInt16;
typedef std::vector<uint32> Array_UInt32;

typedef std::vector<LTObject*> Array_PLTObject;

#endif
