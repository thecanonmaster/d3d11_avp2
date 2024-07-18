#ifndef __RENDERSHADER_MGR_H__
#define __RENDERSHADER_MGR_H__

#ifndef __D3D11_H__
#include <d3d11_1.h>
#define __D3D11_H__
#endif

#ifndef __DIRECT_X_MATH_H__
#include <DirectXMath.h>
#define __DIRECT_X_MATH_H__
#endif

#include "d3d_shader_base.h"
#include "draw_light.h"

#define SOURCE_NAME_LENGTH		128
#define MAX_CONST_BUFFER_SLOTS	D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT
#define MAX_SHADER_RESOURCE_SLOTS	18
#define MAX_VERTEX_RESOURCE_SLOTS	2

static const char* g_szShaderResourceTag = "SHADER";
static const char* g_szShaderEntrypointVS = "VSMain";
static const char* g_szShaderEntrypointGS = "GSMain";
static const char* g_szShaderEntrypointPS = "PSMain";
static const char* g_szShaderProfileVS = "vs_5_0";
static const char* g_szShaderProfileGS = "gs_5_0";
static const char* g_szShaderProfilePS = "ps_5_0";

// TODO - separate shaders for every scenario
enum RenderShader
{
	SHADER_Invalid = -1,
	SHADER_OptimizedSurface = 0,
	SHADER_OptimizedSurfaceBatch,
	SHADER_SolidDrawing,
	SHADER_ToneMap,
	SHADER_ScreenFX,
	SHADER_ClearScreen,
	SHADER_FullscreenVideo,
	SHADER_Model,
	SHADER_WorldModel,
	SHADER_SkyWorldModel,
	SHADER_SkyPortal,
	SHADER_Sprite,
	SHADER_SpriteBatch,
	SHADER_ParticleSystem,
	SHADER_Canvas,
	SHADER_CanvasBatch,
	SHADER_PolyGrid,
	SHADER_LineSystem,
	SHADER_PassThrough,
	SHADER_BloomExtract,
	SHADER_BloomBlur,
	SHADER_BloomCombine,
	SHADER_MAX
};

enum ConstBufferSlot_3D_VS
{
	CBS_3D_VS_Invalid = -1,
	CBS_3D_VS_PerFrame = 0,
	CBS_3D_VS_Model,
	CBS_3D_VS_SubModel,
	CBS_3D_VS_Sprite,
	CBS_3D_VS_ParticleSystem,
	CBS_3D_VS_Canvas,
	CBS_3D_VS_PolyGrid,
	CBS_3D_VS_LineSystem,
	CBS_3D_VS_WorldModel,
	CBS_3D_VS_SubWorldModel,
	CBS_3D_VS_DynamicLights
};

enum ConstBufferSlot_3D_GS
{
	CBS_3D_GS_Invalid = -1,
	CBS_3D_GS_PerFrame = 0,
	CBS_3D_GS_Model
};

enum ConstBufferSlot_3D_PS
{
	CBS_3D_PS_Invalid = -1,
	CBS_3D_PS_PerFrame = 0,
	CBS_3D_PS_Model,
	CBS_3D_PS_SubModel,
	CBS_3D_PS_Sprite,
	CBS_3D_PS_ParticleSystem,
	CBS_3D_PS_Canvas,
	CBS_3D_PS_PolyGrid,
	CBS_3D_PS_LineSystem,
	CBS_3D_PS_WorldModel,
	CBS_3D_PS_SubWorldModel,
	CBS_3D_PS_DynamicLights,
	CBS_3D_PS_Bloom
};

enum ConstBufferSlot_Opt2D_VS
{
	CBS_Opt2D_VS_Invalid = -1,
	CBS_Opt2D_VS_PerFrame = 0
};

enum ConstBufferSlot_Opt2D_PS
{
	CBS_Opt2D_PS_Invalid = -1,
	CBS_Opt2D_PS_PerFrame = 0,
	CBS_Opt2D_PS_OptimizedSurface,
	CBS_Opt2D_PS_ClearScreen
};

enum ConstBufferIndex_VS
{
	CBI_VS_Invalid = -1,
	CBI_3D_VS_PerFrame = 0,
	CBI_3D_VS_Model,
	CBI_3D_VS_SubModel,
	CBI_3D_VS_Sprite,
	CBI_3D_VS_ParticleSystem,
	CBI_3D_VS_Canvas,
	CBI_3D_VS_PolyGrid,
	CBI_3D_VS_LineSystem,
	CBI_3D_VS_WorldModel,
	CBI_3D_VS_SubWorldModel,
	CBI_3D_VS_SkyPortal,
	CBI_3D_VS_DynamicLights,
	CBI_Opt2D_VS_PerFrame,
};

enum ConstBufferIndex_GS
{
	CBI_GS_Invalid = -1,
	CBI_3D_GS_PerFrame = 0,
	CBI_3D_GS_Model,
};

enum ConstBufferIndex_PS
{
	CBI_PS_Invalid = -1,
	CBI_3D_PS_PerFrame = 0,
	CBI_3D_PS_Model,
	CBI_3D_PS_SubModel,
	CBI_3D_PS_Sprite,
	CBI_3D_PS_ParticleSystem,
	CBI_3D_PS_Canvas,
	CBI_3D_PS_PolyGrid,
	CBI_3D_PS_LineSystem,
	CBI_3D_PS_WorldModel,
	CBI_3D_PS_SubWorldModel,
	CBI_3D_PS_SkyPortal,
	CBI_3D_PS_DynamicLights,
	CBI_3D_PS_Bloom,
	CBI_Opt2D_PS_PerFrame,
	CBI_Opt2D_PS_OptimizedSurface,
	CBI_Opt2D_PS_ClearScreen,
};

enum ShaderResourceSlot_VS
{
	SRS_VS_Invalid = -1,
	SRS_VS_VertexFXData = 0,
	SRS_VS_LMVertexData
};

enum ShaderResourceSlot_PS
{
	SRS_PS_Invalid = -1,
	SRS_PS_Primary = 0,
	SRS_PS_GlobalEnvMap,
	SRS_PS_GlobalSkyPan,
	SRS_PS_LMVertexData,
	SRS_PS_LMPages,
	SRS_PS_Other
};

enum VertexResourceSlot
{
	VRS_Invalid = -1,
	VRS_Main = 0,
	VRS_Instance
};

class ResourceInclude : public ID3DInclude
{
	virtual HRESULT __stdcall	Open(D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes);
	virtual HRESULT __stdcall	Close(LPCVOID pData);
};

typedef std::vector<CRenderShader_Base*> Array_PCRenderShader;

class CRenderShaderMgr
{

public:

	struct VertexResourceSlotData
	{
		ID3D11Buffer*	m_pBuffer;
		uint32			m_dwStride;
		uint32			m_dwOffset;

		void Init(ID3D11Buffer* pBuffer, uint32 dwStride, uint32 dwOffset)
		{
			m_pBuffer = pBuffer;
			m_dwStride = dwStride;
			m_dwOffset = dwOffset;
		}

		bool CompareTo(ID3D11Buffer* pBuffer, uint32 dwStride, uint32 dwOffset)
		{
			return m_pBuffer == pBuffer && m_dwStride == dwStride && m_dwOffset == dwOffset;
		}
	};

	CRenderShaderMgr();

	~CRenderShaderMgr();

	bool	Init();
	void	FreeAll();

	CRenderShader_Base*	GetRenderShader(RenderShader eRenderShader) { return m_aRenderShader[eRenderShader]; }

	template <class T>
	T* GetRenderShader() { return (T*)m_aRenderShader[T::m_eRenderShader]; }

	RenderShader	GetCurrentInputLayout() { return m_eCurrentInputLayout; }
	RenderShader	GetCurrentShaderVS() { return m_eCurrentShaderVS; }
	RenderShader	GetCurrentShaderGS() { return m_eCurrentShaderGS; }
	RenderShader	GetCurrentShaderPS() { return m_eCurrentShaderPS; }

	ConstBufferIndex_VS	GetCurrentConstBufferSlotVS(uint32 eSlot) { return m_aeCurrentConstBufferSlotsVS[eSlot]; }
	ConstBufferIndex_GS	GetCurrentConstBufferSlotGS(uint32 eSlot) { return m_aeCurrentConstBufferSlotsGS[eSlot]; }
	ConstBufferIndex_PS	GetCurrentConstBufferSlotPS(uint32 eSlot) { return m_aeCurrentConstBufferSlotsPS[eSlot]; }

	bool	ValidateCurrentInputLayout(RenderShader eIndex) { return (m_eCurrentInputLayout == eIndex); }
	bool	ValidateCurrentShaderVS(RenderShader eIndex) { return (m_eCurrentShaderVS == eIndex); }
	bool	ValidateCurrentShaderGS(RenderShader eIndex) { return (m_eCurrentShaderGS == eIndex); }
	bool	ValidateCurrentShaderPS(RenderShader eIndex) { return (m_eCurrentShaderPS == eIndex); }

	void	ResetCurrentData();

	bool	ValidateCurrentConstBufferSlotVS(uint32 eSlot, ConstBufferIndex_VS eIndex)
	{ 
		return (m_aeCurrentConstBufferSlotsVS[eSlot] == eIndex);
	}

	bool	ValidateCurrentConstBufferSlotGS(uint32 eSlot, ConstBufferIndex_GS eIndex)
	{
		return (m_aeCurrentConstBufferSlotsGS[eSlot] == eIndex);
	}

	bool	ValidateCurrentConstBufferSlotPS(uint32 eSlot, ConstBufferIndex_PS eIndex)
	{
		return (m_aeCurrentConstBufferSlotsPS[eSlot] == eIndex);
	}

	void	SetInputLayout(RenderShader eIndex, ID3D11InputLayout* pLayout);
	void	SetShaderVS(RenderShader eIndex, ID3D11VertexShader* pShader);
	void	SetShaderGS(RenderShader eIndex, ID3D11GeometryShader* pShader);
	void	SetShaderPS(RenderShader eIndex, ID3D11PixelShader* pShader);

	void	SetConstantBuffersVS(uint32 eStart, uint32 dwCount, ConstBufferIndex_VS eIndex, ID3D11Buffer** ppConstantBuffers);
	void	SetConstantBuffersGS(uint32 eStart, uint32 dwCount, ConstBufferIndex_GS eIndex, ID3D11Buffer** ppConstantBuffers);
	void	SetConstantBuffersPS(uint32 eStart, uint32 dwCount, ConstBufferIndex_PS eIndex, ID3D11Buffer** ppConstantBuffers);

	void	SetShaderResourceVS(uint32 eSlot, ID3D11ShaderResourceView* pResourceView);
	void	SetShaderResourcesVS(uint32 eStart, uint32 dwCount, ID3D11ShaderResourceView** pResourceViews);
	void	ClearShaderResourcesVS(uint32 dwStart, uint32 dwCount);
	
	void	SetShaderResourcePS(uint32 eSlot, ID3D11ShaderResourceView* pResourceView);
	void	SetShaderResourcesPS(uint32 eStart, uint32 dwCount, ID3D11ShaderResourceView** pResourceViews);
	void	ClearShaderResourcesPS(uint32 eStart, uint32 dwCount);

	void	SetVertexResource(uint32 eSlot, ID3D11Buffer* pBuffer, uint32 dwStride, uint32 dwOffset);
	void	SetVertexResources(uint32 eStart, uint32 dwCount, ID3D11Buffer** ppBuffers, uint32* pStrides, uint32* pOffsets);

	void	SetIndexBuffer16(ID3D11Buffer* pBuffer, uint32 dwOffset);
	void	SetIndexBuffer32(ID3D11Buffer* pBuffer, uint32 dwOffset);

	void	SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY eTopology);

	bool	SetPerFrameParamsVS_Opt2D(uint32 dwScreenWidth, uint32 dwScreenHeight);
	bool	SetPerFrameParamsPS_Opt2D(float fExposure);

	bool	SetPerFrameParamsVS_3D(CRenderShader_Base::VSPerFrameParams_3D* pInitStruct);
	bool	SetPerFrameParamsPS_3D(CRenderShader_Base::PSPerFrameParams_3D* pInitStruct);
	bool	SetDynamicLightsVPS_3D(Array_PLTObject& aLight, Array_ModelShadowLight& aModelShadowLight);

private:

	template <class T>
	bool	CreateRenderShader(uint32 dwVSResource, uint32 dwGSResource, uint32 dwPSResource);

	bool		GetShaderSourceName(void* pSource, uint32 dwSourceSize, char* szName);

	ID3D10Blob*	CompileShader(void* pSource, uint32 dwSourceSize, const char* szEntryPoint, const char* szProfile);
	ID3D10Blob*	CompileShader(uint32 dwResource, const char* szEntryPoint, const char* szProfile);

	RenderShader	m_eCurrentInputLayout;
	RenderShader	m_eCurrentShaderVS;
	RenderShader	m_eCurrentShaderGS;
	RenderShader	m_eCurrentShaderPS;

	ConstBufferIndex_VS	m_aeCurrentConstBufferSlotsVS[MAX_CONST_BUFFER_SLOTS];
	ConstBufferIndex_GS	m_aeCurrentConstBufferSlotsGS[MAX_CONST_BUFFER_SLOTS];
	ConstBufferIndex_PS	m_aeCurrentConstBufferSlotsPS[MAX_CONST_BUFFER_SLOTS];

	ID3D11ShaderResourceView*	m_apCurrentShaderResourcesVS[MAX_SHADER_RESOURCE_SLOTS];
	ID3D11ShaderResourceView*	m_apCurrentShaderResourcesPS[MAX_SHADER_RESOURCE_SLOTS];

	VertexResourceSlotData	m_aCurrentVertexResources[MAX_VERTEX_RESOURCE_SLOTS];

	ID3D11Buffer*	m_pCurrentIndexBuffer;
	uint32			m_dwCurrentIndexBufferOffset;

	D3D11_PRIMITIVE_TOPOLOGY	m_eCurrentPrimitiveTopology;

	Array_PCRenderShader	m_aRenderShader;

	ID3D11Buffer*	m_pVSPerFrameBuffer_Opt2D;
	ID3D11Buffer*	m_pPSPerFrameBuffer_Opt2D;
	ID3D11Buffer*	m_pVSPerFrameBuffer_3D;
	ID3D11Buffer*	m_pPSPerFrameBuffer_3D;
	ID3D11Buffer*	m_pVPSDynamicLights_3D;
};

extern CRenderShaderMgr g_RenderShaderMgr;

#endif