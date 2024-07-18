#ifndef __TAGNODES_H__
#define __TAGNODES_H__

#ifndef __DIRECT_X_MATH_H__
#include <DirectXMath.h>
#define __DIRECT_X_MATH_H__
#endif

class ViewParams;
class AllocSet;
class ObjectDrawList;

typedef void (_fastcall* WorldTree__FindObjectsInBox_Type)(WorldTree* pTree, void* notUsed, 
	LTVector* pMin, LTVector* pMax, WTObjCallback cb, void* pCBUser, NodeObjArray eArray);

typedef void (_fastcall* WorldTree__FindObjectsInBox2_Type)(WorldTree* pTree, void* notUsed, FindObjInfo* pInfo);
typedef void (_fastcall* WorldTree__DoVisQuery_Type)(WorldTree* pTree, void* notUsed, VisQueryRequest* pRequest);

struct WorldTree_VTable
{
	void (_fastcall* WorldTree_Func0)();
	void (_fastcall* WorldTree__FindObjectsInBox)(WorldTree* pTree, void* notUsed,
		LTVector* pMin, LTVector* pMax, WTObjCallback cb, void* pCBUser, NodeObjArray eArray);

	void (_fastcall* WorldTree__FindObjectsInBox2)(WorldTree* pTree, void* notUsed, FindObjInfo* pInfo);
	void (_fastcall* WorldTree_Func3)();
	void (_fastcall* WorldTree_IntersectSegment)(WorldTree* pTree, void* notUsed, LTVector* pPoint1, 
		LTVector* vPoint2, ISCallback cb, void* pCBUser, NodeObjArray eArray);

	void (_fastcall* WorldTree__DoVisQuery)(WorldTree* pTree, void* notUsed, VisQueryRequest* pRequest);
};

typedef void (*DrawObjectFn)(ViewParams* pParams, LTObject* pObject);
typedef void (*DrawObjectExFn)(ViewParams* pParams, LTObject* pObject, LTObject* pPrevObject);
typedef std::list<AllocSet*> List_PAllocSet;
typedef std::unordered_map<LTObject*, bool> Map_PObjectFlag;

#define MAX_VISIBLE_MODELS				128
#define MAX_VISIBLE_SPRITES				128
#define MAX_VISIBLE_WORLDMODELS			64
#define MAX_VISIBLE_LIGHTS				64
#define MAX_VISIBLE_POLYGRIDS			32
#define MAX_VISIBLE_LINESYSTEMS			32
#define MAX_VISIBLE_PARTICLESYSTEMS		64
#define MAX_VISIBLE_CANVASES			32

class AllocSet
{

public:

	AllocSet();
	~AllocSet();

	void	Init(char* szSetName, uint32 defaultMax);
	void	Term();

	Array_PLTObject&	GetObjects() { return m_Objects; }
	LTObject*			GetObjectByIndex(int nIndex) { return m_Objects[nIndex]; }
	uint32				GetObjectCount() { return m_Objects.size(); }

	void	ClearSet();

	void	Add(LTObject* pObject);
	void	Draw(ViewParams* pParams, DrawObjectFn DrawFn);
	void	Queue(ObjectDrawList* pDrawList, ViewParams* pParams, DrawObjectFn DrawFn);
	void	Queue(ObjectDrawList* pDrawList, ViewParams* pParams, DrawObjectFn pDrawFn, DrawObjectExFn DrawFn);

private:

	char*	m_szSetName;

	Array_PLTObject	m_Objects;
	uint32			m_dwMaxObjects;

	//Map_PObjectFlag	m_ObjectFlags;
};

class VisibleSet
{

public:

	VisibleSet();

	void	Init();
	void	Term();

	void	ClearSet();

	AllocSet* GetModels() { return &m_SolidModels; }
	AllocSet* GetTranslucentModels() { return &m_TranslucentModels; }
	AllocSet* GetChromakeyModels() { return &m_ChromakeyModels; }

	AllocSet* GetSprites() { return &m_TranslucentSprites; }
	AllocSet* GetNoZSprites() { return &m_NoZSprites; }

	AllocSet* GetWorldModels() { return &m_SolidWorldModels; }
	AllocSet* GetTranslucentWorldModels() { return &m_TranslucentWorldModels; }
	AllocSet* GetChromakeyWorldModels() { return &m_ChromakeyWorldModels; }

	AllocSet* GetLights() { return &m_Lights; }

	AllocSet* GetPolyGrids() { return &m_SolidPolyGrids; }
	AllocSet* GetTranslucentPolyGrids() { return &m_TranslucentPolyGrids; }

	AllocSet* GetLineSystems() { return &m_LineSystems; }

	AllocSet* GetParticleSystems() { return &m_ParticleSystems; }

	AllocSet* GetCanvases() { return &m_SolidCanvases; }
	AllocSet* GetTranslucentCanvases() { return &m_TranslucentCanvases; }

private:

	List_PAllocSet	m_Sets;

	AllocSet	m_SolidModels;
	AllocSet	m_TranslucentModels;
	AllocSet	m_ChromakeyModels;

	AllocSet	m_TranslucentSprites;
	AllocSet	m_NoZSprites;

	AllocSet	m_SolidWorldModels;
	AllocSet	m_TranslucentWorldModels;
	AllocSet	m_ChromakeyWorldModels;

	AllocSet	m_Lights;

	AllocSet	m_SolidPolyGrids;
	AllocSet	m_TranslucentPolyGrids;

	AllocSet	m_LineSystems;

	AllocSet	m_ParticleSystems;

	AllocSet	m_SolidCanvases;
	AllocSet	m_TranslucentCanvases;
};

VisibleSet* d3d_GetVisibleSet();

void d3d_TagVisibleLeaves(ViewParams* pParams);
void d3d_CacheObjects();

extern DirectX::XMFLOAT3 g_vCameraPosVisBoxMin;
extern DirectX::XMFLOAT3 g_vCameraPosVisBoxMax;

#endif