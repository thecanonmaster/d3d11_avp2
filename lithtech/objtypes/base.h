struct Node;

struct Attachment
{	
	LTransform	m_Offset;
	
	uint16	m_wParentID;
	uint16	m_wChildID;	
	uint32	m_dwSocket;
	
    Attachment*	m_pNext;
};

struct ClientData
{
	float	m_fData0; // 0 296
	float	m_fData1; // 4 300

	LTVector	m_vData2; // 8 304
	LTVector	m_vData3; // 20 316
	LTVector	m_vData4; // 32 328

	LTLink	m_Data5; // 44 340
	float	m_fData6; // 56 352
	float	m_fData7; // 60 356
	
	LTRotation	m_rData8; // 64 360

	LTLink	m_Data9; // 80 376
	
	uint32 m_dwData10; // 92 388
	
	uint16	m_nClientFlags; // 96 392
	uint16	m_wData11; // 98 394 ? Padding

	void*	m_pUserData; // 100 396 ?
};

struct ModelInstance;
struct SpriteInstance;
struct WorldModelInstance;
struct DynamicLight;
struct CameraInstance;
struct ParticleSystem;
struct PolyGrid;
struct LineSystem;
struct ContainerInstance;
struct Canvas;

struct LTObject
{
	WorldTreeObj	m_Base;

	void*	m_pObjectMgr; // 92 - ObjectMgr

	LTLink	m_ADLink; // 96 ?
	LTLink	m_BspLink; // 108 ?

	Node*	m_pNode; // 120 ?
	
	LTLink	m_Link; // 124
	
    uint32	m_dwFlags; // 136
    uint32	m_dwFlags2; // 140
    uint32	m_dwUserFlags; // 144
	
    uint8	m_nColorR;	// 148
    uint8	m_nColorG;
    uint8	m_nColorB;
    uint8	m_nColorA;
	
	Attachment*	m_pAttachments; // 152
	
	LTRotation	m_rRot; // 156
	LTVector	m_vScale; // 172
	
	uint16	m_wObjectID; // 184
	uint16	m_wSerializeID; // 186 ?
	
	uint8	m_nObjectType; // 188
	uint8	m_nBPriority; // 189
	uint16	m_wData2; // 190 ? Padding

	void*	m_pUserData; // 192
	
	LTVector	m_vVelocity; // 196
	LTVector	m_vAcceleration; // 208
	
	uint8	m_nData3_0; // 220 - set to 5 on ::Clear
	uint8	m_nData3_1; // 221 ? Padding
	uint16	m_wData3_2; // 222 ? Padding
	
	float	m_fFrictionCoefficient; // 224
	float	m_fMass; // 228
	float	m_fForceIgnoreLimitSqr; // 232
	
	LTObject*	m_pStandingOn; // 236
    Node*		m_pNodeStandingOn; // 240
	
	LTVector	m_vBBoxMin; // 244
	LTVector	m_vBBoxMax; // 256
	
	//uint32	m_adwData4_2[31]; // 268
	LTLink	m_ObjectsStandingOn; // 268 ?
    LTLink	m_StandingOnLink; // 280 ?
	uint32	m_dwInternalFlags; // 292

	ClientData	m_ClientData;
	
	void*	m_pSrvObjData; // 400
	
	LTVector	m_vPos; // 404
	LTVector	m_vDims; // 416
	
	float	m_fRadius; // 428 ?

	inline ModelInstance* ToModel() { return (ModelInstance*)this; }
	inline SpriteInstance* ToSprite() { return (SpriteInstance*)this; }
	inline WorldModelInstance* ToWorldModel() { return (WorldModelInstance*)this; }
	inline DynamicLight* ToDynamicLight() { return (DynamicLight*)this; }
	inline CameraInstance* ToCamera() { return (CameraInstance*)this; }
	inline ParticleSystem* ToParticleSystem() { return (ParticleSystem*)this; }
	inline PolyGrid* ToPolyGrid() { return (PolyGrid*)this; }
	inline LineSystem* ToLineSystem() { return (LineSystem*)this; }
	inline ContainerInstance* ToContainer() { return (ContainerInstance*)this; }
	inline Canvas* ToCanvas() { return (Canvas*)this; }

	inline bool IsWorldModel()
	{
		return m_nObjectType == OT_WORLDMODEL || m_nObjectType == OT_CONTAINER;
	}
};