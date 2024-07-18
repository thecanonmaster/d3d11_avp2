#include "pch.h"

#include "rendermodemgr.h"
#include "globalmgr.h"
#include "rendererconsolevars.h"
#include "common_stuff.h"

CRenderModeMgr g_RenderModeMgr;

static inline void ColorSet_FogColored(DirectX::XMFLOAT3* pOutput, StateChange* pStateChange)
{
	XMFloat3FromRGBA(pOutput, pStateChange->m_RenderList[1].m_dwRenderState);
}

void CRenderModeMgr::Init()
{
	m_bInitialized = true;

	// TODO - decrease amount of copies-moves?
	m_StageChangeData[
	{ 
		{ }, 
		{ { 0, 1, 8 } } // COLOROP(ADDSIGNED)
	}] = { "AddSigned", MODE_ADD_SIGNED, 0, nullptr, BLEND_STATE_Invalid, STENCIL_STATE_Invalid };

	m_StageChangeData[
	{
		{ },
		{ { 0, 1, 5 } } // COLOROP(MODULATE2X)
	}] = { "Modulate2X", MODE_MODULATE2X, 0, nullptr, BLEND_STATE_Invalid, STENCIL_STATE_Invalid };

	m_StageChangeData[
	{ 
		{ }, 
		{ { 0, 1, 2 } } // COLOROP(SELECTARG1)
	}] = { "NoLighting", MODE_NO_LIGHT, 0, nullptr, BLEND_STATE_Invalid, STENCIL_STATE_Invalid };

	m_StageChangeData[
	{ 
		{ { 19, 9 }, { 20, 3 } }, // SRCBLEND(DESTCOLOR), DESTBLEND(SRCCOLOR)
		{ } 
	}] = { "NoFullbright", 0, MODE_FULL_BRITE, nullptr, BLEND_STATE_Invalid, STENCIL_STATE_Invalid }; // TODO - BLEND_STATE_Multiply2?

	m_StageChangeData[
	{
		{ { 19, 4 }, { 20, 4 }, { 27, 1 }, { 28, 0 } }, // SRCBLEND(INVSRCCOLOR), DESTBLEND(INVSRCCOLOR), ALPHABLENDENABLE(1), FOGENABLE(0)
		{ { 0, 1, 2 } } // COLOROP(SELECTARG1)
	}] = { "NavAliens", MODE_NO_LIGHT, MODE_FOG_ENABLED, nullptr, BLEND_STATE_Invert, STENCIL_STATE_Invalid };

	m_StageChangeData[
	{
		{ { 28, 0 } }, // FOGENABLE(0)
		{ { 0, 1, 2 } } // COLOROP(SELECTARG1)
	}] = { "NoFog", MODE_NO_LIGHT, MODE_FOG_ENABLED, nullptr, BLEND_STATE_Invalid, STENCIL_STATE_Invalid };

	m_StageChangeData[
	{
		{ { 28, 1 }, { 34, 0 }, { 35, 3 }, { 36, 0 }, { 37, 0 } }, // FOGENABLE(0), FOGCOLOR(VAR), FOGTABLEMODE(LINEAR), FOGSTART(0), FOGEND(0)
		{ { 0, 1, 2 } } // COLOROP(SELECTARG1)
	}] = { "FogColored", MODE_NO_LIGHT | MODE_FLAT_COLOR, 0, ColorSet_FogColored, BLEND_STATE_Invalid, STENCIL_STATE_Invalid };
}

void CRenderModeMgr::Term()
{
	m_bInitialized = false;

	m_StageChangeData.clear();
}

void CRenderModeMgr::SetupRenderMode_Sprite(SharedTexture* pTexture, uint32 dwFlags, uint32 dwFlags2, 
	BlendState& eBlendState, uint32& dwRenderMode, BlendState eDefaultBlendState)
{
	if (pTexture == nullptr)
		dwRenderMode |= MODE_NO_TEXTURE;

	if (g_CV_FogEnable.m_Val && !(dwFlags & FLAG_FOGDISABLE))
		dwRenderMode |= MODE_FOG_ENABLED;

	if (dwFlags2 & FLAG2_ADDITIVE)
	{
		eBlendState = BLEND_STATE_Add;
		dwRenderMode |= MODE_BLEND_ADDITIVE;
	}
	else if (dwFlags2 & FLAG2_MULTIPLY)
	{
		eBlendState = BLEND_STATE_Multiply;
		// TODO - not used?
		dwRenderMode |= MODE_BLEND_MULTIPLY;
	}
	else
	{
		eBlendState = eDefaultBlendState;
	}
}

void CRenderModeMgr::SetupRenderMode_Model(uint32 dwFlags, uint32 dwFlags2, BlendState& eBlendState,
	uint32& dwRenderMode, BlendState eDefaultBlendState)
{
	if (g_CV_FogEnable.m_Val)
		dwRenderMode |= MODE_FOG_ENABLED;

	if (g_CV_EnvMapEnable.m_Val && (dwFlags & FLAG_ENVIRONMENTMAP))
		dwRenderMode |= MODE_ENV_MAP;

	if (dwFlags2 & FLAG2_ADDITIVE)
	{
		if (dwFlags & FLAG_NOLIGHT)
		{
			eBlendState = BLEND_STATE_Add;
			dwRenderMode |= MODE_BLEND_ADDITIVE;
		}
		else
		{
			eBlendState = eDefaultBlendState;
		}
	}
	else if (dwFlags2 & FLAG2_MULTIPLY)
	{
		if (dwFlags & FLAG_NOLIGHT)
		{
			eBlendState = BLEND_STATE_Multiply;
			dwRenderMode |= MODE_BLEND_MULTIPLY;
		}
		else
		{
			eBlendState = eDefaultBlendState;
		}
	}
	else
	{
		eBlendState = eDefaultBlendState;
	}
}

void CRenderModeMgr::SetupRenderMode_WorldModel(uint32 dwFlags, uint32 dwFlags2, BlendState& eBlendState, 
	uint32& dwRenderMode, BlendState eDefaultBlendState)
{
	if (g_CV_FogEnable.m_Val && g_CV_FogNearZ.m_Val != g_CV_FogFarZ.m_Val)
		dwRenderMode |= MODE_FOG_ENABLED;

	if (dwFlags2 & FLAG2_ADDITIVE)
	{
		eBlendState = BLEND_STATE_Add;
		dwRenderMode |= (MODE_BLEND_ADDITIVE | MODE_NO_LIGHT);
	}
	else if (dwFlags2 & FLAG2_MULTIPLY)
	{
		eBlendState = BLEND_STATE_Multiply;
		dwRenderMode |= MODE_BLEND_MULTIPLY;
	}
	else
	{
		eBlendState = eDefaultBlendState;
	}
}

void CRenderModeMgr::SetupRenderMode_SkyWorldModel(uint32 dwFlags, uint32 dwFlags2, BlendState& eBlendState, 
	uint32& dwRenderMode, BlendState eDefaultBlendState)
{
	if (g_CV_FogEnable.m_Val && g_CV_SkyFogNearZ.m_Val != g_CV_SkyFogFarZ.m_Val)
		dwRenderMode |= MODE_FOG_ENABLED;

	if (dwFlags2 & FLAG2_ADDITIVE)
	{
		eBlendState = BLEND_STATE_Add;
		dwRenderMode |= (MODE_BLEND_ADDITIVE | MODE_NO_LIGHT);
	}
	else if (dwFlags2 & FLAG2_MULTIPLY)
	{
		eBlendState = BLEND_STATE_Multiply;
		dwRenderMode |= MODE_BLEND_MULTIPLY;
	}
	else
	{
		eBlendState = eDefaultBlendState;
	}
}

void CRenderModeMgr::SetupRenderMode_Canvas(SharedTexture* pTexture, uint32 dwFlags, uint32 dwFlags2, 
	uint32& dwRenderMode)
{
	if (pTexture == nullptr)
		dwRenderMode |= MODE_NO_TEXTURE;

	if (g_CV_FogEnable.m_Val && !(dwFlags & FLAG_FOGDISABLE))
		dwRenderMode |= MODE_FOG_ENABLED;
}

void CRenderModeMgr::SetupRenderMode_LineSystem(uint32 dwFlags, uint32& dwRenderMode)
{
	if (g_CV_FogEnable.m_Val)
		dwRenderMode |= MODE_FOG_ENABLED;
}

void CRenderModeMgr::SetupRenderMode_ParticleSystem(SharedTexture* pTexture, uint32 dwFlags, uint32 dwFlags2, 
	BlendState& eBlendState, uint32& dwRenderMode, BlendState eDefaultBlendState)
{
	if (pTexture == nullptr)
		dwRenderMode |= MODE_NO_TEXTURE;

	if (g_CV_FogEnable.m_Val && !(dwFlags & FLAG_FOGDISABLE))
		dwRenderMode |= MODE_FOG_ENABLED;

	if (dwFlags2 & FLAG2_ADDITIVE)
	{
		eBlendState = BLEND_STATE_Add;
		dwRenderMode |= MODE_BLEND_ADDITIVE;
	}
	else if (dwFlags2 & FLAG2_MULTIPLY)
	{
		eBlendState = BLEND_STATE_Multiply;
		dwRenderMode |= MODE_BLEND_MULTIPLY;
	}
	else
	{
		eBlendState = eDefaultBlendState;
	}
}

void CRenderModeMgr::SetupRenderMode_PolyGrid(SharedTexture* pTexture, uint32 dwFlags, uint32 dwFlags2, 
	BlendState& eBlendState, uint32& dwRenderMode, BlendState eDefaultBlendState)
{
	if (pTexture == nullptr)
		dwRenderMode |= MODE_NO_TEXTURE;

	if (g_CV_FogEnable.m_Val)
		dwRenderMode |= MODE_FOG_ENABLED;

	if (dwFlags2 & FLAG2_ADDITIVE)
	{
		eBlendState = BLEND_STATE_Add;
		dwRenderMode |= MODE_BLEND_ADDITIVE;
	}
	else if (dwFlags2 & FLAG2_MULTIPLY)
	{
		eBlendState = BLEND_STATE_Multiply;
		dwRenderMode |= MODE_BLEND_MULTIPLY;
	}
	else
	{
		eBlendState = eDefaultBlendState;
	}
}

void CRenderModeMgr::ApplyStateChange(StateChange* pStateChange, BlendState& eBlendState, StencilState& eStencilState, 
	uint32& dwRenderMode, DirectX::XMFLOAT3* pModeColor)
{
	auto iter = m_StageChangeData.find(*pStateChange);

	if (iter != m_StageChangeData.end())
	{
		CRenderModeData& data = iter->second;

		dwRenderMode |= data.m_dwFlagsToSet;
		dwRenderMode &= ~data.m_dwFlagsToUnset;

		if (data.m_pColorSetFn != nullptr)
			data.m_pColorSetFn(pModeColor, pStateChange);

		eBlendState = data.m_eBlendState;
		eStencilState = data.m_eStencilState;
	} 
	else
	{
		NotImplementedMessage("Unknown state change!");
	}
}
