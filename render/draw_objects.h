#ifndef __DRAWOBJECTS_H__
#define __DRAWOBJECTS_H__

#include "tagnodes.h"
#include <queue>

#ifndef __DIRECT_X_MATH_H__
#include <DirectXMath.h>
#define __DIRECT_X_MATH_H__
#endif

class ViewParams;

struct ObjectHandler
{
	uint8	m_nType;

	void	(*m_pModuleInit)();
	void	(*m_pModuleTerm)();
	bool	m_bModuleInitted;

	void	(*m_pPreFrameFn)();

	void	(*m_pProcessObjectFn)(LTObject* pObject);

	bool	m_bCheckWorldVisibility;

	DirectX::XMFLOAT3	(*m_pGetDims)(LTObject* pObject);
};

extern ObjectHandler g_ObjectHandlers[NUM_OBJECT_TYPES];

void d3d_InitObjectModules();
void d3d_TermObjectModules();

void d3d_InitObjectQueues();
void d3d_FlushObjectQueues(ViewParams* pParams);

class ObjectDrawer
{

public:

	ObjectDrawer() 
	{
		m_pObject = nullptr;
		m_pDrawFn = nullptr;
		m_pDrawExFn = nullptr;
		m_fDistance = 0.0f;
	};

	ObjectDrawer(ObjectDrawer* pOther)
	{
		m_pObject = pOther->m_pObject;
		m_pDrawFn = pOther->m_pDrawFn;
		m_pDrawExFn = pOther->m_pDrawExFn;
		m_fDistance = pOther->m_fDistance;
	}

	ObjectDrawer(LTObject* pObject, DrawObjectFn pDrawFn, float fDistance)
	{
		m_pObject = pObject;
		m_pDrawFn = pDrawFn;
		m_pDrawExFn = nullptr;
		m_fDistance = fDistance;
	}

	ObjectDrawer(LTObject* pObject, DrawObjectFn pDrawFn, DrawObjectExFn pDrawExFn, float fDistance)
	{
		m_pObject = pObject;
		m_pDrawFn = pDrawFn;
		m_pDrawExFn = pDrawExFn;
		m_fDistance = fDistance;
	}

	inline void Draw(ViewParams* pParams) const { m_pDrawFn(pParams, m_pObject); };
	inline void DrawEx(ViewParams* pParams, LTObject* pPrevObject) const { m_pDrawExFn(pParams, m_pObject, pPrevObject); };

	inline bool operator<(const ObjectDrawer& other) const
	{
		if ((m_pObject->m_dwFlags & FLAG_REALLYCLOSE) == (other.m_pObject->m_dwFlags & FLAG_REALLYCLOSE))
		{
			if (m_fDistance != other.m_fDistance)
				return m_fDistance < other.m_fDistance;

			// TODO - needs better approach?
			if (m_pObject->m_fRadius != other.m_pObject->m_fRadius)
				return m_pObject->m_fRadius > other.m_pObject->m_fRadius;

			uint32 dwTest = m_pObject->m_dwFlags2 & (FLAG2_ADDITIVE | FLAG2_MULTIPLY);
			uint32 dwOtherTest = other.m_pObject->m_dwFlags2 & (FLAG2_ADDITIVE | FLAG2_MULTIPLY);
			if (dwTest != dwOtherTest)
				return dwTest > dwOtherTest;

			return false;
		}
		else
		{
			return (m_pObject->m_dwFlags & FLAG_REALLYCLOSE) > (other.m_pObject->m_dwFlags & FLAG_REALLYCLOSE);
		}
	}

	LTObject*	GetLTObject() const { return m_pObject; }

	DrawObjectFn	GetDrawFn() const { return m_pDrawFn; }
	DrawObjectExFn	GetDrawExFn() const { return m_pDrawExFn; }

	float	GetDistance() const { return m_fDistance; }
	
private:

	LTObject*	m_pObject;

	DrawObjectFn	m_pDrawFn;
	DrawObjectExFn	m_pDrawExFn;

	float	m_fDistance;
};

class ObjectDrawList
{

public:

	ObjectDrawList() { };

	void Add(ViewParams* pParams, LTObject* pObject, DrawObjectFn pDrawFn);
	void AddEx(ViewParams* pParams, LTObject* pObject, DrawObjectFn pDrawFn, DrawObjectExFn pDrawExFn);

	void Draw(ViewParams* pParams);

private:

	static float CalcDistance(LTObject* pObject, ViewParams* pParams);

	typedef std::priority_queue<ObjectDrawer> Queue_ObjectDrawers;
	Queue_ObjectDrawers	m_ObjectDrawers;
};


#endif