#ifndef __DRAW_MODEL_H__
#define __DRAW_MODEL_H__

#ifndef __D3D11_H__
#include <d3d11_1.h>
#define __D3D11_H__
#endif

#include "common_datamgr.h"
#include "d3d_vertextypes.h"
#include "renderstatemgr.h"

class ViewParams;
class ObjectDrawList;
struct QueuedModelInfo;

#define MODEL_PIECELESS_RENDERING

#define MODEL_VERTEX_PER_TRI			3
#define MODEL_MAX_WEIGHTS_PER_VERTEX	4

struct IndexToUVPair
{
	IndexToUVPair(uint32 dwIndex, UVPair* pUVPair)
	{
		Init(dwIndex, pUVPair);
	}

	void Init(uint32 dwIndex, UVPair* pUVPair)
	{
		m_dwIndex = dwIndex;
		m_pUVPair = pUVPair;
	}

	uint32	m_dwIndex;
	UVPair*	m_pUVPair;

	bool operator==(const IndexToUVPair& other) const
	{
		return m_pUVPair->m_fU == other.m_pUVPair->m_fU && 
			m_pUVPair->m_fV == other.m_pUVPair->m_fV;
	};

	bool operator==(const UVPair& other)
	{
		return m_pUVPair->m_fU == other.m_fU && m_pUVPair->m_fV == other.m_fV;
	};
};

typedef VertexTypes::PositionNormalTextureSkinnedIndex InternalModelVertex;
typedef std::vector<InternalModelVertex> Array_InternalModelVertex;
typedef std::vector<IndexToUVPair> Array_IndexToUVPair;
typedef std::vector<Array_IndexToUVPair> Array2D_IndexToUVPair;
typedef std::unordered_map<uint32, Array_IndexToUVPair> Map_Array_IndexToUVPair;

class CModelData : public CExpirableData
{

public:

	struct BufferInfo
	{
		BufferInfo(uint32 dwVertexPos, uint32 dwIndexPos, uint32 dwIndexCount)
		{
			Init(dwVertexPos, dwIndexPos, dwIndexCount);
		}

		void Init(uint32 dwVertexPos, uint32 dwIndexPos, uint32 dwIndexCount)
		{
			m_dwVertexPos = dwVertexPos;
			m_dwIndexPos = dwIndexPos;
			m_dwIndexCount = dwIndexCount;
		}

		uint32	m_dwVertexPos;
		uint32	m_dwIndexPos;
		uint32	m_dwIndexCount;
	};

	typedef std::vector<BufferInfo> Array_BufferInfo;
	typedef std::vector<Array_BufferInfo> Array2D_BufferInfo;

	CModelData() : CExpirableData()
	{
		m_pVertexBuffer = nullptr;
		m_pIndexBuffer = nullptr;
		m_dwVertexCount = 0;
		m_dwIndexCount = 0;
		m_dwTextureIndicesInUse = 0;
#if defined(DEBUG) || defined(_DEBUG)
		m_szFilename[0] = 0;
#endif
	}

	~CModelData();

	bool	Init(Model* pModel);

	ID3D11Buffer* m_pVertexBuffer;
	ID3D11Buffer* m_pIndexBuffer;

	Array2D_BufferInfo	m_aBufferInfo;

#if defined(DEBUG) || defined(_DEBUG)
	char	m_szFilename[256];
#endif

	uint32	GetVertexCount() { return m_dwVertexCount; }
	uint32	GetIndexCount() { return m_dwIndexCount; }

	uint32	GetTextureIndicesInUse() { return m_dwTextureIndicesInUse; }

private:

	void	PieceToLODArrays_Reserve(Model* pModel, uint32& dwFullVertexSize, uint32& dwFullIndexSize);

	bool	CreateVertexBuffer(InternalModelVertex* pData, uint32 dwSize);
	bool	CreateIndexBuffer(uint16* pData, uint32 dwSize);

	uint32	m_dwVertexCount;
	uint32	m_dwIndexCount;

	uint32	m_dwTextureIndicesInUse;
};

class CModelManager : public CExpirableDataManager<Model*, CModelData*>
{

public:

	CModelData*	GetModelData(Model* pModel);
};

void d3d_GetFinalModelTransform(DirectX::XMFLOAT4X4* pTransform, DirectX::XMFLOAT4X4* pTransformNoProj,
	DirectX::XMFLOAT4X4* pModelTransform, DirectX::XMFLOAT4X4* pReallyCloseProj);
void d3d_DrawSolidModels(ViewParams* pParams);
void d3d_DrawChromakeyModels(ViewParams* pParams);
void d3d_QueueTranslucentModels(ViewParams* pParams, ObjectDrawList* pDrawList);
void d3d_QueueModel(ViewParams* pParams, LTObject* pObject, QueuedModelInfo* pInfo);
bool d3d_IsTranslucentModel(LTObject* pObject);
void d3d_DrawModelPieceList(BlendState eDefaultBlendState, uint32 dwExtraModeFlags);
void d3d_ProcessModel(LTObject* pObject);
bool d3d_CacheModel(LTObject* pObject);
void d3d_ModelPreFrame();

#endif
