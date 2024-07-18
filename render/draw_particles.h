#ifndef __DRAW_PARTICLES_H__
#define __DRAW_PARTICLES_H__

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

//#define PARTICLE_SYSTEM_DATA_MEM_CMP

#define PARTICLE_VERTICES	4
#define PARTICLE_INDICES	6

typedef VertexTypes::Texture InternalParticleVertex;
typedef VertexTypes::PositionColorScale InternalParticleInstance;

class ViewParams;
class ObjectDrawList;

enum PSOffsetCorner
{
	PSOC_UPPER_LEFT = 0,
	PSOC_UPPER_RIGHT,
	PSOC_BOTTOM_RIGHT,
	PSOC_BOTTOM_LEFT,
	PSOC_MAX
};

class CParticleSystemData : public CExpirableData
{

public:

	CParticleSystemData(ParticleSystem* pParticleSystem, InternalParticleVertex* pVerts) : 
		CExpirableData(), m_aVertexData()
	{
		m_nInstances = pParticleSystem->m_nParticles;
		m_pInstanceData = new InternalParticleInstance[m_nInstances];
		InitVertexData(pParticleSystem, pVerts);
		InitInstanceData(pParticleSystem);

		m_dwBankedVBIndex = UINT32_MAX;
		m_dwBankedISIndex = UINT32_MAX;
	}

	~CParticleSystemData()
	{
		Term_VertexRelated();
		Term_InstanceRelated();
	}

	void Term_VertexRelated();
	void Term_InstanceRelated();

	bool CompareTo_VertexData(InternalParticleVertex* pOthers)
	{
#ifndef PARTICLE_SYSTEM_DATA_MEM_CMP
		return m_aVertexData[0].CompareTo(pOthers) && m_aVertexData[1].CompareTo(pOthers + 1) &&
			m_aVertexData[2].CompareTo(pOthers + 2) && m_aVertexData[3].CompareTo(pOthers + 3);
#else
		return !memcmp(m_aVertexData, pOthers, PARTICLE_VERTICES * sizeof(InternalParticleVertex));
#endif
	}

	CompareResult	CompareTo_InstanceData(ParticleSystem* pParticleSystem);

	bool	CreateVertexBuffer();
	bool	CreateInstanceBuffer();
	bool	UpdateBuffers(ParticleSystem* pParticleSystem, InternalParticleVertex* pVerts);
	bool	UpdateVertexBuffer(ParticleSystem* pParticleSystem, InternalParticleVertex* pVerts);
	bool	UpdateInstanceBuffer(ParticleSystem* pParticleSystem, CompareResult eResult);

	InternalParticleVertex*		GetVertexData() { return m_aVertexData; }
	InternalParticleInstance*	GetInstanceData() { return m_pInstanceData; }
	int							GetInstanceCount() { return m_nInstances; }

	uint32	m_dwBankedVBIndex;
	uint32	m_dwBankedISIndex;

private:

	void	InitVertexData(ParticleSystem* pParticleSystem, InternalParticleVertex* pVerts);
	void	InitInstanceData(ParticleSystem* pParticleSystem);

	InternalParticleVertex	m_aVertexData[PARTICLE_VERTICES];

	InternalParticleInstance*	m_pInstanceData;
	int							m_nInstances;
};

class CParticleSystemManager : public CExpirableDataManager<Sprite*, CParticleSystemData*>
{

public:

	CParticleSystemData* GetParticleSystemData(ParticleSystem* pParticleSystem, InternalParticleVertex* pVerts);
};

void d3d_ProcessParticles(LTObject* pObject);
void d3d_QueueTranslucentParticles(ViewParams* pParams, ObjectDrawList* pDrawList);

#endif