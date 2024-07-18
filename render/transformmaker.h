#ifndef __TRANSFORMMAKER_H__
#define __TRANSFORMMAKER_H__

#ifndef __DIRECT_X_MATH_H__
#include <DirectXMath.h>
#define __DIRECT_X_MATH_H__
#endif

#define PIECELOD_BASE		0
#define MAX_GVP_ANIMS		8
#define MAX_MODELPATH_LEN	64

class GVPStruct
{

public:

	GVPStruct();

	AnimTimeRef	m_Anims[MAX_GVP_ANIMS];
	uint32		m_dwAnims;

	void*	m_pVertices;
	uint32	m_dwVertexStride;

	DirectX::XMFLOAT4X4	m_mBaseTransform;

	uint32	m_dwLOD;
	float	m_fCurrentLODDist;
};


class TransformMaker
{

public:

	enum NodeEvalState
	{
		NES_EVAL,
		NES_DONE,
		NES_IGNORE
	};

	TransformMaker()
	{
		memset(this, 0, sizeof(TransformMaker));
	}

	void	SetupFromGVPStruct(GVPStruct* pStruct);

	bool	IsValid();
	void	CopyTimes(TransformMaker& other);

	bool	SetupTransforms();
	bool	GetNodeTransform(uint32 dwNode);

	AnimTimeRef		m_Anims[MAX_GVP_ANIMS];
	uint32			m_dwAnims;

	DirectX::XMFLOAT4X4*	m_pStartMatrix;
	DirectX::XMFLOAT4X4*	m_pOutput;

	NodeControlFn	m_pNCFunc;
	void*			m_pNCUser;
	LTObject*		m_pNCObject;

	uint32*	m_pRecursePath;
	uint32	m_dwCurPath;

protected:

	bool	SetupCall();

	void	InitTransform(uint32 dwAnim, uint32 dwNode, DirectX::XMFLOAT4& rOutQuat, DirectX::XMFLOAT3& vOutVec);
	void	InitTransformAdditive(uint32 dwAnim, uint32 dwNode, DirectX::XMFLOAT4& rOutQuat, DirectX::XMFLOAT3& vOutVec);

	void	BlendTransform(uint32 dwAnim, uint32 dwNode);

	void	Recurse(uint32 dwNode, DirectX::XMFLOAT4X4* pParentT, NodeEvalState* pNodeEvalStates);

	CIRelation*							m_pCurRelation;
	DirectX::XMFLOAT4X4					m_mCurRelation;
	CMoArray<CIRelation, DefaultCache>*	m_pNodeRelation;

	DirectX::XMFLOAT4X4	m_mTemp;

	DirectX::XMFLOAT4	m_rQuaternion;
	DirectX::XMFLOAT3	m_vTranslation;

	Model*	m_pModel;

	DirectX::XMFLOAT4X4	m_mIdentity;
};

#endif
