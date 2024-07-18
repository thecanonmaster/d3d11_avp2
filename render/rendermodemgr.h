#ifndef __RENDERMODE_MGR_H__
#define __RENDERMODE_MGR_H__

#ifndef __DIRECT_X_MATH_H__
#include <DirectXMath.h>
#define __DIRECT_X_MATH_H__
#endif

#include "renderstatemgr.h"
#include "d3d_draw.h"
#include "rendererconsolevars.h"

#define MODE_REALLY_CLOSE   (1<<0)
#define MODE_FOG_ENABLED    (1<<1)
#define MODE_NO_TEXTURE     (1<<2)
#define MODE_ENV_MAP        (1<<3)
#define MODE_ENV_MAP_ONLY   (1<<4)
#define MODE_BLEND_MULTIPLY	(1<<5)
#define MODE_ADD_SIGNED		(1<<6)
#define MODE_NO_LIGHT		(1<<7)
#define MODE_FLAT_COLOR		(1<<8)
#define MODE_CHROMAKEY		(1<<9)
#define MODE_DETAIL_TEXTURE	(1<<10)
#define MODE_FULL_BRITE		(1<<11)
#define MODE_SURFACE_FX		(1<<12)
#define MODE_SKY_OBJECT		(1<<13)
#define MODE_ENV_MAP_ALPHA	(1<<14)
#define MODE_SKY_PAN		(1<<15)
#define MODE_MODULATE2X		(1<<16)
#define MODE_BLEND_ADDITIVE	(1<<17)

#define BOOST_HASH_CONSTANT	0x9e3779b9

struct CRenderModeData
{
	typedef void (*ColorSetFn)(DirectX::XMFLOAT3* pOutput, StateChange* pStateChange);

	CRenderModeData()
	{
		memset(this, 0, sizeof(CRenderModeData));
	}

	CRenderModeData(const char* szName, uint32 dwFlagsToSet, uint32 dwFlagsToUnset, ColorSetFn pColorSetFn, 
		BlendState eBlendState, StencilState eStencilState)
	{
		Init(szName, dwFlagsToSet, dwFlagsToUnset, pColorSetFn, eBlendState, eStencilState);
	}

	void Init(const char* szName, uint32 dwFlagsToSet, uint32 dwFlagsToUnset, ColorSetFn pColorSetFn, BlendState eBlendState,
		StencilState eStencilState)
	{
		m_szName = szName;

		m_dwFlagsToSet = dwFlagsToSet;
		m_dwFlagsToUnset = dwFlagsToUnset;

		m_pColorSetFn = pColorSetFn;

		m_eBlendState = eBlendState;
		m_eStencilState = eStencilState;
	}
	
	const char*	m_szName;

	uint32	m_dwFlagsToSet;
	uint32	m_dwFlagsToUnset;

	ColorSetFn	m_pColorSetFn;

	BlendState		m_eBlendState;
	StencilState	m_eStencilState;
};

struct StateChangeHash
{
	size_t operator()(const StateChange& input) const
	{
		size_t dwSeed = 0;
		std::hash<uint32> dwordHash;

		for (size_t i = 0; i < input.m_RenderList.size(); i++)
		{
			RenderState renStateCopy = input.m_RenderList[i];
			if (renStateCopy.m_dwRenderStateType == 34)
				renStateCopy.m_dwRenderState = 0;

			dwSeed ^= dwordHash(renStateCopy.m_dwRenderState) + BOOST_HASH_CONSTANT + (dwSeed << 6) + (dwSeed >> 2);
			dwSeed ^= dwordHash(renStateCopy.m_dwRenderStateType) + BOOST_HASH_CONSTANT + (dwSeed << 6) + (dwSeed >> 2);
		}

		for (size_t i = 0; i < input.m_TextureList.size(); i++)
		{
			TextureState& texState = input.m_TextureList[i];

			dwSeed ^= dwordHash(texState.m_dwStage) + BOOST_HASH_CONSTANT + (dwSeed << 6) + (dwSeed >> 2);
			dwSeed ^= dwordHash(texState.m_dwTextureStateType) + BOOST_HASH_CONSTANT + (dwSeed << 6) + (dwSeed >> 2);
			dwSeed ^= dwordHash(texState.m_dwTextureState) + BOOST_HASH_CONSTANT + (dwSeed << 6) + (dwSeed >> 2);
		}

		return dwSeed;
	}
};

struct StateChangeKeyEq
{
	bool operator()(const StateChange& left, const StateChange& right) const
	{
		size_t dwSizeLeftRender = left.m_RenderList.size();
		size_t dwSizeLeftTexture = left.m_TextureList.size();

		size_t dwSizeRightRender = right.m_RenderList.size();
		size_t dwSizeRightTexture = right.m_TextureList.size();

		if (dwSizeLeftRender != dwSizeRightRender || dwSizeLeftTexture != dwSizeRightTexture)
			return false;

		for (size_t i = 0; i < dwSizeLeftRender; i++)
		{
			RenderState renStateCopy = right.m_RenderList[i];
			if (renStateCopy.m_dwRenderStateType == 34)
				renStateCopy.m_dwRenderState = 0;
			
			if (memcmp(&renStateCopy, &right.m_RenderList[i], sizeof(RenderState)))
				return false;
		}

		for (size_t i = 0; i < dwSizeLeftTexture; i++)
		{		
			if (memcmp(&left.m_TextureList[i], &right.m_TextureList[i], sizeof(TextureState)))
				return false;
		}
		
		return true;
	}
};

typedef std::unordered_map<StateChange, CRenderModeData, StateChangeHash, StateChangeKeyEq> Map_StateChangeData;

class CRenderModeMgr
{

public:

	CRenderModeMgr()
	{
		m_bInitialized = false;
	}

	~CRenderModeMgr()
	{
		if (m_bInitialized)
			Term();
	}

	void Init();

	void Term();

	void	SetupRenderMode_Sprite(SharedTexture* pTexture, uint32 dwFlags, uint32 dwFlags2, BlendState& eBlendState,
		uint32& dwRenderMode, BlendState eDefaultBlendState);

	void	SetupRenderMode_Model(uint32 dwFlags, uint32 dwFlags2, BlendState& eBlendState, uint32& dwRenderMode, 
		BlendState eDefaultBlendState);

	void	SetupRenderMode_WorldModel(uint32 dwFlags, uint32 dwFlags2, BlendState& eBlendState, uint32& dwRenderMode, 
		BlendState eDefaultBlendState);

	void	SetupRenderMode_SkyWorldModel(uint32 dwFlags, uint32 dwFlags2, BlendState& eBlendState, uint32& dwRenderMode,
		BlendState eDefaultBlendState);

	void	SetupRenderMode_Canvas(SharedTexture* pTexture, uint32 dwFlags, uint32 dwFlags2, uint32& dwRenderMode);

	void	SetupRenderMode_LineSystem(uint32 dwFlags, uint32& dwRenderMode);

	void	SetupRenderMode_ParticleSystem(SharedTexture* pTexture, uint32 dwFlags, uint32 dwFlags2, 
		BlendState& eBlendState, uint32& dwRenderMode, BlendState eDefaultBlendState);

	void	SetupRenderMode_PolyGrid(SharedTexture* pTexture, uint32 dwFlags, uint32 dwFlags2, BlendState& eBlendState,
		uint32& dwRenderMode, BlendState eDefaultBlendState);

	void	ApplyStateChange(StateChange* pStateChange, BlendState& eBlendState, StencilState& eStencilState,
		uint32& dwRenderMode, DirectX::XMFLOAT3* pModeColor);

private:

	bool	m_bInitialized;

	Map_StateChangeData	m_StageChangeData;
};

extern CRenderModeMgr g_RenderModeMgr;

#endif