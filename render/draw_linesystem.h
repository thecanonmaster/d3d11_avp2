#ifndef __DRAW_LINE_SYSTEM_H__
#define __DRAW_LINE_SYSTEM_H__

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

#define LINE_VERTICES	2
//#define LINE_SYSTEM_DATA_MEM_CMP

#define LINE_SYSTEM_TEMP_BUFFER_LEN	512

typedef VertexTypes::PositionColor InternalLineVertex;

class ViewParams;
class ObjectDrawList;

class CLineSystemData : public CExpirableData
{

public:

	CLineSystemData(LineSystem* pLineSystem, uint32 dwVerts) : CExpirableData()
	{
		m_dwVerts = dwVerts;
		m_pData = new LTLinePt[dwVerts];
		InitData(pLineSystem);
		
		m_dwBankedVBIndex = UINT32_MAX;
	}

	~CLineSystemData() { Term(); }

	void Term();

	CompareResult CompareTo(LineSystem* pLineSystem, uint32 dwVerts);

	bool	CreateVertexBuffer();
	bool	UpdateVertexBuffer(LineSystem* pLineSystem, uint32 dwVerts);

	LTLinePt*	GetData() { return m_pData; }
	uint32		GetVertexCount() { return m_dwVerts; }

	uint32	m_dwBankedVBIndex;

private:

	void	InitData(LineSystem* pLineSystem);

	LTLinePt*	m_pData;
	uint32		m_dwVerts;
};

class CLineSystemManager : public CExpirableDataManager<LineSystem*, CLineSystemData*>
{

public:

	CLineSystemManager() : CExpirableDataManager()
	{
		m_pTempBuffer = nullptr;
	}

	virtual void Init()
	{
		CExpirableDataManager::Init();
		m_pTempBuffer = new InternalLineVertex[LINE_SYSTEM_TEMP_BUFFER_LEN];
	}

	virtual void Term()
	{
		CExpirableDataManager::Term();
		delete[] m_pTempBuffer;
	}

	CLineSystemData*	GetLineSystemData(LineSystem* pLineSystem);

	InternalLineVertex*	GetTempBuffer() { return m_pTempBuffer; }

private:

	InternalLineVertex*	m_pTempBuffer;
};

void d3d_ProcessLineSystem(LTObject* pObject);
void d3d_QueueLineSystems(ViewParams* pParams, ObjectDrawList* pDrawList);
void d3d_DrawLine(DirectX::XMFLOAT3* pSrc, DirectX::XMFLOAT3* pDest, DirectX::XMFLOAT4* pColor1, DirectX::XMFLOAT4* pColor2);
void d3d_DrawLine(DirectX::XMFLOAT3* pSrc, DirectX::XMFLOAT3* pDest, DirectX::XMFLOAT4* pColor);

#endif