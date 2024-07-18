#ifndef __D3D_DRAW_SPRITE_H__
#define __D3D_DRAW_SPRITE_H__

#ifndef __D3D11_H__
#include <d3d11_1.h>
#define __D3D11_H__
#endif

#ifndef __DIRECT_X_MATH_H__
#include <DirectXMath.h>
#define __DIRECT_X_MATH_H__
#endif

#include "common_datamgr.h"

//#define SPRITE_DATA_EXTENDED
//#define SPRITE_DATA_MEM_CMP

#define SPRITE_VERTICES	4
#define CLIPPED_SPRITE_VERTICES	40 + 5
#define SPRITE_INDICES	6
#define BOOST_HASH_CONSTANT	0x9e3779b9

typedef VertexTypes::PositionColorTexture InternalSpriteVertex;

class ViewParams;
class ObjectDrawList;

class CSpriteVertex
{

public:

	DirectX::XMFLOAT3	m_vPos;

	uint32	m_dwColor;

	float	m_fU;
	float	m_fV;

	inline void SetupVertColorAndUVs(uint32 dwColor, float fU, float fV)
	{
		m_dwColor = dwColor;

		m_fU = fU;
		m_fV = fV;
	}

	bool CompareTo(CSpriteVertex* pOther)
	{
		return m_vPos.x == pOther->m_vPos.x && m_vPos.y == pOther->m_vPos.y && m_vPos.z == pOther->m_vPos.z && 
			m_dwColor == pOther->m_dwColor && m_fU == pOther->m_fU && m_fV == pOther->m_fV;
	}

	static void ClipExtra(CSpriteVertex* pPrev, CSpriteVertex* pCur, CSpriteVertex* pOut, float fT)
	{
		pOut->m_fU = pPrev->m_fU + fT * (pCur->m_fU - pPrev->m_fU);
		pOut->m_fV = pPrev->m_fV + fT * (pCur->m_fV - pPrev->m_fV);

		pOut->m_dwColor = pCur->m_dwColor;
	}
};

class CSpriteData
{

public:

	CSpriteData() : m_aData()
	{
		m_pVertexBuffer = nullptr;
		m_fLastTimeDrawn = 0.0f;
	}

	~CSpriteData();

	bool CompareTo(CSpriteVertex* pOthers)
	{
#ifndef SPRITE_DATA_MEM_CMP
		return m_aData[0].CompareTo(pOthers) && m_aData[1].CompareTo(pOthers + 1) &&
			m_aData[2].CompareTo(pOthers + 2) && m_aData[3].CompareTo(pOthers + 3);
#else
		return !std::memcmp(m_aData, pOthers, SPRITE_VERTICES * sizeof(CSpriteVertex));
#endif
	}

	bool	IsExpired();

	bool	CreateVertexBuffer();
	bool	UpdateVertexBuffer(CSpriteVertex* pNewVerts);

	void	SetLastTimeDrawn(float fSet) { m_fLastTimeDrawn = fSet; }

	CSpriteVertex	m_aData[SPRITE_VERTICES];
	ID3D11Buffer*	m_pVertexBuffer;

private:

	float	m_fLastTimeDrawn;
};

#ifdef SPRITE_DATA_EXTENDED

struct CSpriteVerts
{
	CSpriteVertex	m_aData[SPRITE_VERTICES];
};

struct SpriteVertexHash
{
	std::size_t operator()(const CSpriteVerts& input) const
	{
		std::size_t dwSeed = 0;
		std::hash<float> floatHash;
		std::hash<uint32> uint32Hash;

		for (int i = 0; i < SPRITE_VERTICES; i++) 
		{
			dwSeed ^= floatHash(input.m_aData[i].m_vPos.x) + BOOST_HASH_CONSTANT + (dwSeed << 6) + (dwSeed >> 2);
			dwSeed ^= floatHash(input.m_aData[i].m_vPos.y) + BOOST_HASH_CONSTANT + (dwSeed << 6) + (dwSeed >> 2);
			dwSeed ^= floatHash(input.m_aData[i].m_vPos.z) + BOOST_HASH_CONSTANT + (dwSeed << 6) + (dwSeed >> 2);

			dwSeed ^= uint32Hash(input.m_aData[i].m_dwColor) + BOOST_HASH_CONSTANT + (dwSeed << 6) + (dwSeed >> 2);

			dwSeed ^= floatHash(input.m_aData[i].m_fU) + BOOST_HASH_CONSTANT + (dwSeed << 6) + (dwSeed >> 2);
			dwSeed ^= floatHash(input.m_aData[i].m_fV) + BOOST_HASH_CONSTANT + (dwSeed << 6) + (dwSeed >> 2);
		}

		return dwSeed;
	}
};

struct SpriteVertexKeyEq
{
	bool operator()(const CSpriteVerts& left, const CSpriteVerts& right) const
	{
		return !memcmp(&left, &right, sizeof(CSpriteVerts));
	}
};

typedef std::unordered_map<CSpriteVerts, CSpriteData*, SpriteVertexHash, SpriteVertexKeyEq> Map_SpriteData;
#endif

class CSpriteManager : public CExpirableDataManager<SpriteInstance*, CSpriteData*>
{

public:

#ifdef SPRITE_DATA_EXTENDED
	CSpriteData*	GetSpriteData(CSpriteVertex* pVerts);
#else
	CSpriteData*	GetSpriteData(SpriteInstance* pInstance, CSpriteVertex* pVerts);
#endif

#ifdef SPRITE_DATA_EXTENDED
	Map_SpriteData		m_Data;
#endif
};

void d3d_ProcessSprite(LTObject* pObject);
void d3d_QueueTranslucentSprites(ViewParams* pParams, ObjectDrawList* pDrawList);

#endif