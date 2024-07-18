struct AnimKeyFrame;
struct LTAnimTracker;
struct SharedTexture;
struct Model;
struct ModelNode;
struct AnimNode;
struct ModelAnim;
struct SharedTexture;
struct Sprite;

struct NodeKeyFrame
{
	LTVector	m_vTranslation;
	LTRotation	m_rQuaternion;
};

struct AnimNode
{
	uint32* m_pVTable;

	ModelNode*	m_pNode; // 4

	CMoArray<NodeKeyFrame, NoCache>	m_KeyFrames; // 8

	AnimNode*	m_pParentNode; // 24

	CMoArray<AnimNode*, NoCache>	m_Children;	// 28

	ModelAnim*	m_pAnim; // 44

	uint32			GetKeyFrameCount() { return m_KeyFrames.m_dwElements; }
	NodeKeyFrame*	GetKeyFrame(uint32 dwIndex) { return &m_KeyFrames.m_pArray[dwIndex]; }

	uint32		GetChildCount() { return m_Children.m_dwElements; }
	AnimNode*	GetChild(uint32 dwIndex) { return m_Children.m_pArray[dwIndex]; }
};

typedef void (*KeyCallback)(void *pTracker, AnimKeyFrame *pFrame);

struct AnimKeyFrame
{	
	uint32	m_dwTime;
	
	char*	m_szString;
	
	uint8		m_nKeyType;
	uint8		m_nData_9; // ? padding
	uint16		m_dwData_10; // ? padding

	KeyCallback	m_pCallback;
	
	void*	m_pUser1;
};

struct ModelAnim
{
	uint32*	m_pVTable;

	AnimNode**	m_pAnimNodes;
	
	CMoArray<AnimKeyFrame, NoCache>	m_KeyFrames;
	
	uint32	m_dwModelEditWeightSet;
	
	uint32	m_dwInterpolationMS;
	
	Model*	m_pModel;
	
	char*	m_szName;
	
	AnimNode	m_DefaultRootNode;
	AnimNode*	m_pRootNode;

	uint32	GetKeyFrameCount() { return m_KeyFrames.m_dwElements; }
};

struct CIRelation
{
	LTVector	m_vPos;
	LTRotation	m_rRot;
};

struct ChildInfo
{
	CMoArray<CIRelation, DefaultCache>	m_Relation;

	BOOL	m_bBoundRadiusValid; // ?
	
	uint32	m_dwAnimOffset;
	
	char*	m_szFilename;
	
	uint32	m_dwSaveIndex;
	
	Model*	m_pParentModel;
	Model*	m_pModel;	
	BOOL	m_bTreeValid;
};

struct AnimInfo
{
	ModelAnim*	m_pAnim;

	ChildInfo*	m_pChildInfo;

	LTVector	m_vDims;
	LTVector	m_vTranslation;
};

struct ModelString
{
	uint32			m_dwAllocSize;
	ModelString*	m_pNext;
	char			m_szString[1];
};

struct ModelStringList
{
	ModelString*	m_pStringList;
	void*			m_pAlloc;
};

struct ModelSocket
{
	LTVector	m_vPos;
	LTRotation	m_rRot;

	char	m_szName[MAX_SOCKETNAME_LEN];

	uint32	m_dwNode;

	Model	*m_pAttachment;
};

struct WeightSet
{
	char	m_szName[MAX_WEIGHTSETNAME_LEN];

	CMoArray<float, NoCache>	m_Weights;

	Model*	m_pModel;

	uint32	GetWeightCount() { return m_Weights.m_dwElements; }
	float	GetWeight(uint32 dwIndex) { return m_Weights.m_pArray[dwIndex]; }
};

struct NewVertexWeight
{
	float	m_afVec[4];
	uint32	m_dwNode;
};

struct UVPair
{
	float m_fU;
	float m_fV;

	bool operator==(const UVPair& other)
	{
		return m_fU == other.m_fU && m_fV == other.m_fV;
	};
};

struct ModelTri
{
	uint16	m_awIndices[3];
	UVPair	m_UVs[3];
};

struct ModelVert
{
	NewVertexWeight*	m_pWeights;
	uint16				m_wWeights;
	uint16				m_wReplacement;
	
	LTVector	m_vVec;
	LTVector	m_vNormal;
};

struct PieceLOD
{
	CMoArray<ModelVert, NoCache>	m_Verts;
	CMoArray<ModelTri, NoCache>		m_Tris;

	Model*	m_pModel;

	ModelVert*	GetVert(uint32 dwIndex) { return &m_Verts.m_pArray[dwIndex]; }
	uint32		GetVertCount() { return m_Verts.m_dwElements; }

	ModelTri*	GetTri(uint32 dwIndex) { return &m_Tris.m_pArray[dwIndex]; }
	uint32		GetTriCount() { return m_Tris.m_dwElements; }
};

struct ModelPiece
{
	CMoArray<ModelVert, NoCache> m_Verts; // 0
	CMoArray<ModelTri, NoCache> m_Tris; // 16

	uint32	m_adwData0; // 32

	uint16	m_wTextureIndex; // 36
	uint16	m_wData_38; // 38 ? padding
	uint32	m_dwData_40; // 40 ?

	float	m_fSpecularPower; // 44
	float	m_fSpecularScale; // 48

	CMoArray<PieceLOD, NoCache>	m_LODs; // 52

	float	m_fLODWeight; // 68

	char	m_szName[MAX_PIECENAME_LEN]; // 72

	Model*	m_pModel; // 104

	ModelVert*	GetVert(uint32 dwIndex) { return &m_Verts.m_pArray[dwIndex]; }
	uint32		GetVertCount() { return m_Verts.m_dwElements; }

	ModelTri*	GetTri(uint32 dwIndex) { return &m_Tris.m_pArray[dwIndex]; }
	uint32		GetTriCount() { return m_Tris.m_dwElements; }

	uint32		GetLODCount() { return m_LODs.m_dwElements; }
	PieceLOD*	GetLOD(uint32 dwIndex) { return &m_LODs.m_pArray[dwIndex]; }
	uint32		GetLODIndexFromDist(int nBias, float fDist);
};

struct ModelNode
{
	uint32*	m_pVTable;

	LTVector	m_vOffsetFromParent;
	
	uint16	m_wNodeIndex;
	uint8	m_nFlags;
	uint8	m_nData_19; // 19 ? padding
	uint32	m_dwData_20; // 20 ? m_ScratchData
	
	CMoArray<ModelNode*, NoCache>	m_Children; // 24
	
	uint32	m_dwParentNode; // 40
	
	LTMatrix	m_mGlobalTransform; // 44
	LTMatrix	m_mFromParentTransform; // 108
	
	Model*		m_pModel; // 172
	const char*	m_szName; // 176

	uint32		GetChildCount() { return m_Children.m_dwElements; }
	ModelNode*	GetChild(uint32 dwIndex) { return m_Children.m_pArray[dwIndex]; }
};

struct LODInfo
{
	float	m_fDist;
};

struct Model
{
	uint32*	m_pVTable;

	char*	m_szFilename;

	LTLink	m_Link;

	Nexus	m_Nexus; // 20

	uint32	m_dwFileID; // 28
	uint32	m_dwFlags; // 32

	CMoArray<ModelNode*, NoCache>		m_Nodes; // 36
	CMoArray<ModelPiece*, NoCache>		m_Pieces; // 52
	CMoArray<WeightSet*, NoCache>		m_WeightSets; // 68
	CMoArray<NewVertexWeight, NoCache>	m_NewVertexWeights; // 84

	uint32	m_nTotalVerts; // 100
	uint32	m_nTotalTris;
	uint32	m_nDWordNodes; // ?
	
	CMoArray<LTMatrix, NoCache>	m_Transforms; // 112

	const char*	m_szCommandString; // 128
	
	ModelStringList	m_StringList; // 132

	CMoArray<LODInfo, DefaultCache> m_LODInfos; // 140

	uint32	m_dwData_160; // 160 ?

	float	m_fGlobalRadius; // 164
	float	m_fVisRadius; // 168

	LTBOOL	m_bNoAnimation; // 172

	float	m_fFadeRangeMin; // 176
	float	m_fFadeRangeMinSqr; // 180
	float	m_fFadeRangeMax; // 184
	float	m_fFadeRangeMaxSqr; // 188

	SharedTexture*	m_pFadeSpriteTex; // 192 ?

	float	m_fFadeSpriteSizeX; // 196
	float	m_fFadeSpriteSizeY; // 200

	float	m_fAmbientLight; // 204 ?
	float	m_fDirLight; // 208 ?

	LTBOOL	m_bShadowEnable; // 212

	float	m_fShadowProjectLength; // 216 ?
	float	m_fShadowLightDist; // 220 ?
	float	m_fShadowSizeX; // 224 ?
	float	m_fShadowSizeY; // 228 ?

	LTVector	m_vShadowCenterOffset; // 232

	uint32		m_dwNormalRefNode; // 244
	uint32		m_dwNormalRefAnim; // 248
	LTMatrix	m_mNormalRef; // 252
	LTBOOL		m_bNormalRef; // 316

	LTBOOL	m_bFOVOffset; // 320
	float	m_fFOVOffsetX; // 324
	float	m_fFOVOffsetY; // 328

	LTBOOL	m_bSpecularEnable; // 332

	LTBOOL	m_bRigid; // 336

	CMoArray<ModelSocket*, NoCache>	m_Sockets; // 340
	CMoArray<AnimInfo, NoCache>		m_Anims; // 356
	
	void*	m_pAlloc; // 372
	void*	m_pDefAlloc; // 376
	uint32	m_BlockAlloc[4]; // 380

	uint32	m_dwModelMemory; // 396
	LTBOOL	m_bLoaded; // 400 ?

	ChildInfo*	m_pChildModels[MAX_CHILD_MODELS];
	uint32		m_nChildModels;
	ChildInfo	m_SelfChildModel;

	uint32	m_dwFileVersion; // 520

	uint32	m_dwRefCount; // 524 ?

	ModelNode	m_DefaultRootNode;
	ModelNode*	m_pRootNode; // 708

	uint32		GetLODInfoCount() { return m_LODInfos.m_dwElements; }
	LODInfo*	GetLODInfo(uint32 dwIndex) { return &m_LODInfos.m_pArray[dwIndex]; }

	uint32		GetNodeCount() { return m_Nodes.m_dwElements; }
	ModelNode*	GetNode(uint32 dwIndex) { return m_Nodes.m_pArray[dwIndex]; }

	uint32		GetPieceCount() { return m_Pieces.m_dwElements; }
	ModelPiece* GetPiece(uint32 dwIndex) { return m_Pieces.m_pArray[dwIndex]; }

	uint32		GetWeightSetCount() { return m_WeightSets.m_dwElements; }
	WeightSet*	GetWeightSet(uint32 dwIndex) { return m_WeightSets.m_pArray[dwIndex]; }

	uint32				GetNewVertexWeightCount() { return m_NewVertexWeights.m_dwElements; }
	NewVertexWeight*	GetNewVertexWeight(uint32 dwIndex) { return &m_NewVertexWeights.m_pArray[dwIndex]; }

	uint32		GetTransformCount() { return m_Transforms.m_dwElements; }
	LTMatrix*	GetTransform(uint32 dwIndex) { return &m_Transforms.m_pArray[dwIndex]; }

	uint32		GetAnimCount() { return m_Anims.m_dwElements; }
	ModelAnim*	GetAnim(uint32 dwIndex) { return m_Anims.m_pArray[dwIndex].m_pAnim; }
	AnimInfo*	GetAnimInfo(uint32 dwIndex) { return &m_Anims.m_pArray[dwIndex]; }
};

typedef void (*ModelHookFn)(ModelHookData* pData, void* pUser);

typedef void (*StringKeyCallback)(LTAnimTracker* pTracker, AnimKeyFrame* pFrame, char* szAppend, LTBOOL bForceReplace);

struct FrameLocator
{
	uint16	m_wAnim;
	uint16	m_wFrame;
	uint32	m_dwTime;
	uint32	m_dwWeightSet;
};

struct AnimTimeRef
{
	Model*	m_pModel;
	
	FrameLocator	m_Prev;
	FrameLocator	m_Cur;
	
	float	m_fPercent;	

	LTBOOL	m_bNormalize;

	bool IsValid();
};

struct LTAnimTracker
{
	LTLink	m_Link;
	
	StringKeyCallback	m_pStringKeyCallback;
	void*				m_pUserData;

	uint32	m_dwCurKey;
	
	uint16	m_wFlags;
	uint16	m_wInterpolationMS;

	uint32	m_dwTimeScaleDenom;		
	uint32	m_dwTimeScaleNum;
	
	LTBOOL	m_bAllowInterpolation;
	
	AnimTimeRef	m_TimeRef;
	
	int	m_nIndex;

	inline bool	IsValid() { return m_TimeRef.IsValid(); }
	Model*	GetModel() { return m_TimeRef.m_pModel; }
};

typedef void (*ModelInstanceHookFn)(LTObject* pObj, ModelInstanceHookData *pData, uint32 dwFlags);
typedef void (*NodeControlFn)(LTObject* pObj, uint32 hNode, LTMatrix* pGlobalMat, void* pUserData);

struct ModelInstance
{
	LTObject	m_Base;

	uint32	m_dwData_432; // 432 ?
	FileIdentifier*	m_pFileIdentifier; // 436
	uint32	m_dwData_440; // 440 ?

	ModelInstanceHookFn	m_pInstanceHookFn; // 444

	SharedTexture*	m_pSkins[MAX_MODEL_TEXTURES]; // 448

	LTAnimTracker	m_AnimTracker; // 464
    LTAnimTracker*	m_pAnimTrackers; // 544
	
	CMoArray<FrameLocator, NoCache>	m_CachedLocators; // 548
	CMoArray<LTMatrix, NoCache>		m_CachedTransforms; // 564

	NodeControlFn	m_pNCFunc; // 580 ?
	void*			m_pNCUserData; // 584 ?
	
	uint32	m_adwHiddenPiece[MAX_PIECES_PER_MODEL / 32]; // 588 

	Sprite*			m_pSprites[MAX_MODEL_TEXTURES]; // 592
    SpriteTracker	m_SpriteTrackers[MAX_MODEL_TEXTURES];

	LTVector	m_vLastDirLightPos; // 688 ?
	float		m_fLastDirLightAmount; // 700 ?

	LTVector	m_vLastLightColor; // 704 ?

	LTBOOL	m_bValidTransformsFlag; // 716 ?

	bool	IsPieceHidden(uint32 dwIndex);

	Model*	GetModel() { return m_AnimTracker.GetModel(); }

	uint32			GetCachedLocatorsCount() { return m_CachedLocators.m_dwElements; }
	FrameLocator*	GetCachedLocators(uint32 dwIndex) { return &m_CachedLocators.m_pArray[dwIndex]; }

	uint32		GetCachedTransformCount() { return m_CachedTransforms.m_dwElements; }
	LTMatrix*	GetCachedTransform(uint32 dwIndex) { return &m_CachedTransforms.m_pArray[dwIndex]; }
};
