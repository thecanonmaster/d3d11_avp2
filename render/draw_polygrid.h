#ifndef __DRAW_POLY_GRID_H__
#define __DRAW_POLY_GRID_H__

#ifndef __D3D11_H__
#include <d3d11_1.h>
#define __D3D11_H__
#endif

#ifndef __DIRECT_X_MATH_H__
#include <DirectXMath.h>
#define __DIRECT_X_MATH_H__
#endif

#include "common_datamgr.h"
#include "d3d_vertextypes.h"

//#define POLY_GRID_DATA_MEM_CMP
#define MIN_POLYGRID_SIZE	2

typedef VertexTypes::PositionNormalColorTexture InternalPolyGridVertex;

class ViewParams;
class ObjectDrawList;

class CPolyGridData : public CExpirableData
{

public:

	typedef std::pair<CompareResult, CompareResult> CompareResultPair;

	struct VertexDataEssentials
	{
		float fCurrX, fYScale, fCurrZ, fCurrU, fCurrV, fXStart, fXInc, fUInc, fStartU, fZInc, fVInc;
	};

	struct PGInitialValues
	{
		LTVector	m_vDims;
		LTVector	m_vScale;

		uint32	m_dwWidth;
		uint32	m_dwHeight;
		float	m_fPanX;
		float	m_fPanY;
		float	m_fScaleX;
		float	m_fScaleY;

		PGColor	m_aColorTable[POLY_GRID_COLOR_TABLE_SIZE];

		inline void Init(PolyGrid* pPolyGrid)
		{
			m_vDims = pPolyGrid->m_Base.m_vDims;
			m_vScale = pPolyGrid->m_Base.m_vScale;
			m_dwHeight = pPolyGrid->m_dwHeight;
			m_dwWidth = pPolyGrid->m_dwWidth;
			m_fPanX = pPolyGrid->m_fPanX;
			m_fPanY = pPolyGrid->m_fPanY;
			m_fScaleX = pPolyGrid->m_fScaleX;
			m_fScaleY = pPolyGrid->m_fScaleY;

			memcpy(m_aColorTable, pPolyGrid->m_aColorTable, sizeof(m_aColorTable));
		}

		bool CompareTo(PolyGrid* pPolyGrid)
		{
			return m_vDims.x == pPolyGrid->m_Base.m_vDims.x && m_vDims.y == pPolyGrid->m_Base.m_vDims.y && m_vDims.z == pPolyGrid->m_Base.m_vDims.z &&
				m_vScale.x == pPolyGrid->m_Base.m_vScale.x && m_vScale.y == pPolyGrid->m_Base.m_vScale.y && m_vScale.z == pPolyGrid->m_Base.m_vScale.z &&
				m_dwWidth == pPolyGrid->m_dwWidth && m_dwHeight == pPolyGrid->m_dwHeight && m_fPanX == pPolyGrid->m_fPanX && m_fPanY == pPolyGrid->m_fPanY &&
				m_fScaleX == pPolyGrid->m_fScaleX && m_fScaleY == pPolyGrid->m_fScaleY && 
				!memcmp(m_aColorTable, pPolyGrid->m_aColorTable, sizeof(m_aColorTable));
		}
	};

	CPolyGridData(PolyGrid* pPolyGrid, uint32 dwVerts, uint32 dwIndices);

	~CPolyGridData() 
	{ 
		Term_VertexRelated();
		Term_IndexRelated();
	}

	void Term_VertexRelated();
	void Term_IndexRelated();

	CompareResultPair CompareTo(PolyGrid* pPolyGrid, uint32 dwVerts, uint32 dwIndices);

	bool	CreateVertexBuffer();
	bool	CreateIndexBuffer();
	bool	UpdateBuffers(PolyGrid* pPolyGrid, uint32 dwVerts, uint32 dwIndices);
	bool	UpdateVertexBuffer(PolyGrid* pPolyGrid, uint32 dwVerts, CompareResult eResult);
	bool	UpdateIndexBuffer(PolyGrid* pPolyGrid, uint32 dwIndices, CompareResult eResult);

	uint32	GetVertexCount() { return m_dwVerts; }
	uint16*	GetIndexData() { return m_pIndexData; }
	uint32	GetIndexCount() { return m_dwIndices; }
	
	uint32	m_dwBankedVBIndex;
	uint32	m_dwBankedIBIndex;

private:

	inline void	CalcVertexDataEssentials(PolyGrid* pPolyGrid, VertexDataEssentials* pParams);

	void	InitVertexData(PolyGrid* pPolyGrid);
	void	InitIndexData(PolyGrid* pPolyGrid);

	PGInitialValues	m_InitialValues;

	uint32	m_dwVerts;

	uint16*	m_pIndexData;
	uint32	m_dwIndices;

	InternalPolyGridVertex* m_pTempBuffer;
};

class CPolyGridManager : public CExpirableDataManager<PolyGrid*, CPolyGridData*>
{

public:

	CPolyGridData* GetPolyGridData(PolyGrid* pPolyGrid);
};

void d3d_DrawSolidPolyGrids(ViewParams* pParams);
void d3d_DrawPolyGrid(ViewParams* pParams, LTObject* pObj);
void d3d_QueueTranslucentPolyGrids(ViewParams* pParams, ObjectDrawList* pDrawList);
bool d3d_IsTranslucentPolyGrid(LTObject* pObject);
void d3d_ProcessPolyGrid(LTObject* pObject);

#endif