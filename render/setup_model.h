#ifndef __SETUP_MODEL_H__
#define __SETUP_MODEL_H__

#ifndef __DIRECT_X_MATH_H__
#include <DirectXMath.h>
#define __DIRECT_X_MATH_H__
#endif

#include "d3d_viewparams.h"
#include "d3d_utils.h"
#include "d3d_shader_model.h"

typedef float (_fastcall* ModelInstance__GetRadius_Type)(ModelInstance* pInstance);
typedef LTMatrix* (_fastcall* ModelInstance__GetNodeTransforms_Type)(ModelInstance* pInstance); // 18
typedef LTMatrix* (_fastcall* ModelInstance__GetNodeTransform_Type)(ModelInstance* pInstance, void* pNotUsed, uint32 dwIndex);
typedef BOOL (_fastcall* ModelInstance__HasValidNodeTransforms_Type)(ModelInstance* pInstance);

struct ModelInstance_VTable
{
	void (_fastcall* ModelInstance__Destructor)();
	void (_fastcall* LTObject__InsertSpecial)();
	void (_fastcall* LTObject__RemoveFromWorldTree)();
	void (_fastcall* LTObject__GetBBox)();
	void (_fastcall* WorldTreeObj__IsVisContainer)();
	void (_fastcall* WorldTreeObj__GetVisContainerLink)();
	void (_fastcall* NullFunc1)();
	void (_fastcall* NullFunc2)();
	void (_fastcall* LTObject__Init)();
	void (_fastcall* LTObject__GetCSType)();
	void (_fastcall* LTObject__SetupTransform)();
	void (_fastcall* ModelInstance__GetVisRadius)();
	void (_fastcall* LTObject__IsMoveable)();
	void (_fastcall* WorldTreeObj__IsMainWorldModel)();
	void (_fastcall* ModelInstance__SetupTransformMaker)();
	void (_fastcall* ModelInstance__SetupGVPTime)();
	void (_fastcall* ModelInstance__FindTracker)();

	float (_fastcall* ModelInstance__GetRadius)(ModelInstance* pInstance);
	LTMatrix* (_fastcall* ModelInstance__GetNodeTransforms)(ModelInstance* pInstance);
	LTMatrix* (_fastcall* ModelInstance__GetNodeTransform)(ModelInstance* pInstance, void* pNotUsed, uint32 dwIndex);
	BOOL (_fastcall* ModelInstance__HasValidNodeTransforms)(ModelInstance* pInstance);
};

struct InternalModelHookData
{
	uint32	m_dwHookFlags;
	uint32	m_dwObjectFlags;

	LTVector	m_vLightAdd;
	LTVector	m_vObjectColor;
};

struct QueuedModelInfo
{
	QueuedModelInfo(ModelInstance* pInstance)
	{
		Init(pInstance);
	}

	void Init(ModelInstance* pInstance)
	{
		m_pInstance = pInstance;
	}

	ModelInstance*	m_pInstance;

	InternalModelHookData	m_ModelHookData;

	XMFloat4x4Trinity	m_sTransforms;

	DirectX::XMFLOAT3	m_vAmbientLight;

	DirectX::XMFLOAT3	m_vDirLightColor;

	uint32	m_dwDynamicLightCount;
	uint32	m_aDynamicLightIndex[MAX_DYNAMIC_LIGHTS_PER_MODEL];

	uint32			m_dwStaticLightCount;
	StaticLight*	m_apStaticLight[MAX_STATIC_LIGHTS_PER_MODEL];
};

struct QueuedPieceInfo
{

	ModelInstance*	m_pInstance;
	Model*			m_pModel;
	uint32			m_dwPieceIndex;
	uint32			m_dwLODIndex;
	SharedTexture*	m_pTexture;
	float			m_fDistanceSqr;
	
	QueuedModelInfo*	m_pModelInfo;

	bool	m_bReallyClose;

	bool	operator==(const QueuedPieceInfo& other) const;
	bool	operator<(const QueuedPieceInfo& other) const;

	static bool	LessThan_Translucent(const QueuedPieceInfo& left, const QueuedPieceInfo& right);
};

typedef std::unordered_map<Model*, Array_UInt32> Map_ModelPrimaryNodes;
typedef std::vector<QueuedModelInfo> Array_QueuedModelInfo;
typedef std::unordered_map<Model*, Array_QueuedModelInfo> Map_Array_QueuedModelInfo;
typedef std::vector<QueuedPieceInfo> Array_QueuedPieceInfo;

class ModelSetup
{

public:

	ModelSetup() { };
	~ModelSetup() 
	{
		m_ModelPrimaryNodes.clear();
		m_QueuedModelInfo.clear();
		m_aQueuedPieceInfo.clear();
	};

	void				QueueModelInfo(ModelInstance* pInstance);
	QueuedModelInfo*	FindQueuedModelInfo(ModelInstance* pInstance);
	QueuedModelInfo*	FindQueuedModelInfo(Array_QueuedModelInfo& aInfo, ModelInstance* pInstance);

	float	GetDirLightAmount(ModelInstance* pInstance, DirectX::XMFLOAT3* pInstancePos, DirectX::XMFLOAT4X4* pTransform);

	void	SetupModelLight(ModelInstance* pInstance, QueuedModelInfo& modelInfo);
	void	FillModelInfo(QueuedModelInfo* pInfo, ModelInstance* pInstance);
	void	QueueAllModelInfo();

	void	QueueModel(ModelInstance* pInstance);
	void	QueueModel(ModelInstance* pInstance, QueuedModelInfo* pInfo);

	void	FreeQueuedData();

	Map_Array_QueuedModelInfo&	GetQueuedModelInfo() { return m_QueuedModelInfo; }
	Array_QueuedPieceInfo&	GetQueuedPieceInfo() { return m_aQueuedPieceInfo; }

private:

	float	CalcModelPieceDistSqr(QueuedPieceInfo* pieceInfo, XMFloat4x4Trinity* pTransforms, 
		Array_UInt32& aModelPrimaryNodes);

	void	QueueModelPieces(ModelInstance* pInstance, QueuedModelInfo* pModelInfo);
	void	QueueModelPiece(ModelInstance* pInstance, Model* pModel, uint32 dwPieceIndex, uint32 dwLODIndex, bool bTexture,
		QueuedModelInfo* pModelInfo, float fDistToModel, Array_UInt32& aModelPrimaryNodes);

	void	CacheModelInstanceTransforms(ModelInstance* pInstance);

	void	CallModelInstanceHook(ModelInstance* pInstance, ModelInstanceHookData* pHookData);
	void	CallModelHook(ModelInstance* pInstance, InternalModelHookData* pHookData);

	static void	StaticLightCB(WorldTreeObj* pObj, void* pUser);

	Map_ModelPrimaryNodes	m_ModelPrimaryNodes;

	Map_Array_QueuedModelInfo	m_QueuedModelInfo;
	Array_QueuedPieceInfo		m_aQueuedPieceInfo;
};

extern ModelSetup g_ModelSetup;

#endif
