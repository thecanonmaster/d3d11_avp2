// Unverified
struct WorldTreeNode;
struct Leaf;
struct UserPortal;

#define NF_IN	1
#define NF_OUT	2

#define MAX_OBJ_NODE_LINKS		5
#define MAX_WTNODE_CHILDREN		4
//#define NUM_NODEOBJ_ARRAYS		3 // not 4
#define FRAMECODE_NOTINTREE		0xFFFFFFFF

struct ObjectMgr
{
	uint32*	m_pVTable;

	uint32	m_dwCurFrameCode;

	LTLink	m_InternalLink;

	StructBank	m_ParticleBank;
	StructBank	m_LineBank;

	StructBank	m_AttachmentBank;

	ObjectBank	m_ObjectBankNormal;
	ObjectBank	m_ObjectBankModel;
	ObjectBank	m_ObjectBankWorldModel;
	ObjectBank	m_ObjectBankSprite;
	ObjectBank	m_ObjectBankLight;
	ObjectBank	m_ObjectBankCamera;
	ObjectBank	m_ObjectBankParticleSystem;
	ObjectBank	m_ObjectBankPolyGrid;
	ObjectBank	m_ObjectBankLineSystem;
	ObjectBank	m_ObjectBankContainer;
	ObjectBank	m_ObjectBankCanvas;

	ObjectBank*	m_ObjectBankPointers[NUM_OBJECT_TYPES];

	LTList	m_ObjectLists[NUM_OBJECT_TYPES];
};

enum NodeObjArray
{
	NOA_Objects = 0,
	NOA_Lights,
	NOA_VisContainers,
	//NOA_TerrainSections, ? separate TerrainSections
	NUM_NODEOBJ_ARRAYS
};

enum WTObjType
{
	WTObj_DObject = 0,
	WTObj_Light
};

struct WTObjLink
{
	LTLink			m_Link;
	WorldTreeNode*	m_pNode;
};

struct WorldTreeObj
{
	uint32*	m_pVTable;

	WTObjLink	m_Links[MAX_OBJ_NODE_LINKS]; // 4

	WTObjType	m_ObjType; // 84
	
	uint32	m_dwWTFrameCode; // 88
};

struct WorldTreeNode
{
	CheapLTLink	m_Objects[NUM_NODEOBJ_ARRAYS];

	LTVector	m_vBBoxMin; // 24
	LTVector	m_vBBoxMax;	
	LTVector	m_vCenter;
	
	float	m_fMinSize; // 60
	float	m_fRadius; // 64
	
	uint32	m_dwObjectsOnOrBelow; // 68 ?
	
	WorldTreeNode*	m_pParent; // 72 ?
	WorldTreeNode*	m_apChildren[MAX_WTNODE_CHILDREN]; // 76
	
	/*union
	{
		WorldTreeNode*	m_pChildren[2][2];		
		WorldTreeNode*	m_pChildrenA[MAX_WTNODE_CHILDREN];
	};*/

	bool	HasChildren() { return !!m_apChildren[0]; }
};

struct WorldTree
{
	uint32*	m_pVTable;

	ObjectMgr*	m_pHelper; // 4

	uint32	m_dwTempFrameCode; // 8	?
	
	WorldTreeNode	m_RootNode; // 12

	uint32	m_nNodes; // 104
	uint32	m_nDepth; // 108

	uint32	m_dwTerrainDepth; // 112 ?
	
	CheapLTLink	m_AlwaysVisObjects; // 116 ?
};

typedef bool (*ISCallback)(WorldTreeObj* pObj, void* pUser);
typedef void (*WTObjCallback)(WorldTreeObj* pObj, void* pData);
typedef void (*AddObjectsFn)(LTLink* pLink, LTObject*** pppBuffer, uint32* pCounter);
typedef void (*IterateLeafCB)(Leaf* pLeaf, void* pData);

typedef LTBOOL (*PortalTestFn)(UserPortal* pPortal);
typedef LTBOOL (*NodeFilterFn)(WorldTreeNode* pNode);

struct FindObjInfo
{
	FindObjInfo()
	{
		m_pTree = nullptr;
		m_eObjArray = NOA_Objects;
		m_vMin = { };
		m_vMax = { };
		m_vViewpoint = { };
		m_CB = nullptr;
		m_pCBUser = nullptr;

		m_pTree = nullptr;
	}

	WorldTree* m_pTree;

	NodeObjArray		m_eObjArray;
	LTVector			m_vMin;
	LTVector			m_vMax;
	LTVector			m_vViewpoint;
	WTObjCallback		m_CB;

	void* m_pCBUser;
};

struct VisQueryRequest
{
	static void		DummyIterateObject(WorldTreeObj* pObj, void* pData) { }
	static void		DummyIterateLeaf(Leaf* pLeaf, void* pData) { }
	static LTBOOL	DummyPortalTest(UserPortal* pPortal) { return LTTRUE; }
	static LTBOOL	DummyNodeTest(WorldTreeNode* pNode) { return LTTRUE; }

	VisQueryRequest()
	{
		m_vViewpoint = { };
		m_fViewRadius = 0.0f;
		m_ObjectCB = DummyIterateObject;
		m_AddObjects = nullptr;
		m_pUserData = nullptr;
		m_LeafCB = DummyIterateLeaf;
		m_eObjArray = NOA_Objects;
		m_PortalTest = DummyPortalTest;
		m_NodeFilterFn = DummyNodeTest;

		m_vMin = { };
		m_vMax = { };
		m_pTree = nullptr;
	}

	NodeObjArray		m_eObjArray; // 0
	LTVector			m_vViewpoint; // 4
	float				m_fViewRadius; // 16
	WTObjCallback		m_ObjectCB; // 20
	AddObjectsFn		m_AddObjects; // 24

	void* m_pUserData; // 28

	IterateLeafCB		m_LeafCB; // 32
	PortalTestFn		m_PortalTest; // 36
	NodeFilterFn		m_NodeFilterFn; // 40

	LTVector			m_vMin;
	LTVector			m_vMax;

	WorldTree* m_pTree;
};