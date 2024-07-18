#ifndef __D3D_RENDERWORLD_OCCLUDER_H__
#define __D3D_RENDERWORLD_OCCLUDER_H__

#include "aabb.h"
#include "d3d_mathhelpers.h"

class ViewParams;

typedef std::vector<XMPLANE> Array_XMPLANE;
typedef std::vector<AABBCorner> Array_AABCorner;

class COccluder
{

public:

	COccluder();

	~COccluder();

	COccluder&	operator=(const COccluder& other) 
	{
		m_aEdgePlanes = other.m_aEdgePlanes;
		m_PolyPlane = other.m_PolyPlane;
		m_ePolyPlaneCorner = other.m_ePolyPlaneCorner;

		return *this;
	}

	void	Swap(COccluder& other)
	{
		m_aEdgePlanes.swap(other.m_aEdgePlanes);

		std::swap(m_PolyPlane, other.m_PolyPlane);
		std::swap(m_ePolyPlaneCorner, other.m_ePolyPlaneCorner);
	}

	void	Init() { m_aEdgePlanes.clear(); }

	void	AddPlane(XMPLANE* pScreenPlane);


	Array_XMPLANE	m_aEdgePlanes;
	XMPLANE			m_PolyPlane;
	AABBCorner		m_ePolyPlaneCorner;
};

class COccluder_Frustum : public COccluder
{

public:

	COccluder_Frustum() : COccluder() { }

	~COccluder_Frustum() { }

	COccluder_Frustum&	operator=(const COccluder_Frustum& other) 
	{
		(COccluder&)*this = other;

		m_aWorldEdgePlanes = other.m_aWorldEdgePlanes;
		m_aeEdgeCorners = other.m_aeEdgeCorners;

		return *this;
	}

	void	Swap(COccluder_Frustum& cOther) 
	{
		COccluder::Swap(cOther);

		m_aWorldEdgePlanes.swap(cOther.m_aWorldEdgePlanes);
		m_aeEdgeCorners.swap(cOther.m_aeEdgeCorners);
	}

	void	Init()
	{
		COccluder::Init();

		m_aWorldEdgePlanes.clear();
		m_aeEdgeCorners.clear();
	}

	void	InitFrustum(ViewParams* pParams);

	void	AddPlane(XMPLANE* pScreenPlane, XMPLANE* pWorldPlane);

	PolySide	ClassifyAABB(DirectX::XMFLOAT3* pMin, DirectX::XMFLOAT3* pMax, float fFarZ);

	Array_XMPLANE	m_aWorldEdgePlanes;
	Array_AABCorner	m_aeEdgeCorners;
};

#endif