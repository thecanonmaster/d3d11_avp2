#ifndef __LT_RENDER_TYPES__
#define __LT_RENDER_TYPES__

struct RMode
{
	LTBOOL	m_bHardware;
	
	char	m_szRenderDLL[256];
	char	m_szInternalName[128];
	char	m_szDescription[128];
	
	uint32	m_dwWidth;
	uint32	m_dwHeight;
	uint32	m_dwBitDepth;

	RMode*	m_pNext;
};

struct RenderStructInit
{
    int		m_nRendererVersion; 
    RMode	m_Mode;
    HWND	m_hWnd; // void*
};

struct RenderContext
{
	uint32	m_dwData_0; // 0 - LMData?
	uint32	m_dwData_4; // 4 - LM memory?

	MainWorld*	m_pMainWorld; // 8

	uint16	m_wCurFrameCode; // 12
	uint16	m_wPadding;
};

struct RenderContextInit
{
	MainWorld*	m_pMainWorld;
};

struct SceneDesc
{
    int	m_nDrawMode;
	
	uint32*	m_pObjectTicks;
	uint32*	m_pModelTicks;
	uint32*	m_pSpriteTicks;
	uint32*	m_pWorldModelTicks;
	uint32*	m_pParticleSystemTicks;
	uint32*	m_pLightProcessTicks;
	
	// m_vVoidColor

	LTVector	m_vModelLightAdd;
	LTVector	m_vModelLightAddDir;
	
    RenderContext*	m_pRenderContext; // 52 (13 x 4)
		    
	LTVector	m_vGlobalLightColor; // ? (14 x 4)
	LTVector	m_vGlobalLightDir; // ? (17 x 4)

	LTVector	m_vGlobalLightScale; 
	LTVector	m_vGlobalVertexTint; // 92 (23 x 4)
	LTVector	m_vGlobalWMAmbient;
	LTVector	m_vGlobalLightAdd; // 116 (29 x 4)
	
    float	m_fFrameTime;
	
    SkyDef		m_SkyDef;
    LTObject**	m_pSkyObjects;
    int			m_nSkyObjects;
	
    LTRect		m_rcViewport;
	
    float		m_fFovX;
	float		m_fFovY;
	
	float		m_fFarZ;
	
    LTVector	m_vPos;
    LTRotation	m_rRotation;
	
	LTObject**	m_pObjectList;
	int			m_nObjectListSize;
	
	void		(*m_ModelHookFn)(ModelHookData* pData, void* pUser);
    void*		m_pModelHookUser;
};

struct RenderStruct
{
	LTObject*	(*ProcessAttachment)(LTObject* pParent, Attachment* pAttachment);

	SharedTexture*	(*GetSharedTexture)(const char* szFilename);
	TextureData*	(*GetTexture)(SharedTexture* pSharedTexture);
	void			(*FreeTexture)(SharedTexture* pSharedTexture);

	void	(*RunConsoleString)(char* pString);
	void	(*ConsolePrint)(char* pMsg, ...);

	HLTPARAM	(*GetParameter)(char* szName);
	float		(*GetParameterValueFloat)(HLTPARAM hParam);
    char*		(*GetParameterValueString)(HLTPARAM hParam);

	void	(*RendererPing)();

	uint32	(*IncObjectFrameCode)();
    uint32	(*GetObjectFrameCode)();

	uint16	(*IncCurTextureFrameCode)();

	void*	(*Alloc)(uint32 dwSize);
	void	(*Free)(void* pData);

	uint32	m_dwWidth;
	uint32	m_dwHeight;
    LTBOOL	m_bInitted;

	uint32	m_dwTextureMem;
	uint32	m_dwLightmapsMem;
	uint32	m_dwRendererTextureMem;
	uint32	m_dwMPSavedMem;

	uint32	m_dwTicks_TVL; // ?
	uint32	m_dwTicks_FOQ; // ?
	uint32	m_dwTicks_Models; // ?
	uint32	m_dwTicks_WM; // ?
	uint32	m_dwTicks_TWM; // ?
	uint32	m_dwTicks_Sky; // ?

    int		(*Init)(RenderStructInit* pInit);
    void	(*Term)();

	void	(*BindTexture)(SharedTexture* pSharedTexture, LTBOOL bTextureChanged);
    void	(*UnbindTexture)(SharedTexture* pSharedTexture);

	void	(*RebindLightmaps)(RenderContext* pContext);

	RenderContext*	(*CreateContext)(RenderContextInit* pInit);
	void			(*DeleteContext)(RenderContext* pContext);

	void	(*Clear)(LTRect* pRect, uint32 dwFlags, LTVector* pClearColor);

	LTBOOL	(*Start3D)();
	LTBOOL	(*End3D)();
	LTBOOL	(*IsIn3D)();

    LTBOOL	(*StartOptimized2D)();
    void	(*EndOptimized2D)();
	LTBOOL	(*IsInOptimized2D)();

    LTBOOL	(*SetOptimized2DBlend)(LTSurfaceBlend eBlend);
    LTBOOL	(*GetOptimized2DBlend)(LTSurfaceBlend& eBlend);
    LTBOOL	(*SetOptimized2DColor)(HLTCOLOR dwColor);
    LTBOOL	(*GetOptimized2DColor)(HLTCOLOR& dwColor);

	int		(*RenderScene)(SceneDesc* pScene);

	void	(*RenderCommand)(int argc, char **argv);

	void*	(*GetDirectDrawInterface)(const char* szQuery);

	void	(*SwapBuffers)(uint32 dwFlags);

	int		(*GetInfoFlags)();

	void	(*GetScreenFormat)(PFormat* pFormat);

	HLTBUFFER	(*CreateSurface)(int nWidth, int nHeight);
	void		(*DeleteSurface)(HLTBUFFER hSurface);

	void	(*GetSurfaceInfo)(HLTBUFFER hSurface, uint32* pWidth, uint32* pHeight, int* pPitch);

	void*	(*LockSurface)(HLTBUFFER hSurface);
    void	(*UnlockSurface)(HLTBUFFER hSurface);

	LTBOOL	(*OptimizeSurface)(HLTBUFFER hSurface, GenericColor dwTransparentColor);
    void	(*UnoptimizeSurface)(HLTBUFFER hSurface);

	LTBOOL	(*LockScreen)(int nLeft, int nTop, int nRight, int nBottom, void** pData, int* pPitch);
    void	(*UnlockScreen)();

	void	(*BlitToScreen)(BlitRequest* pRequest);
    LTBOOL	(*WarpToScreen)(BlitRequest* pRequest);

	void	(*MakeScreenShot)(const char* szFilename);

	void	(*ReadConsoleVariables)();

	void	(*BlitFromScreen)(BlitRequest* pRequest);

	uint32	m_dwData_264; // 264 ?

	SharedTexture* m_pEnvMapTexture;

	GlobalPanInfo m_GlobalPanInfo[NUM_GLOBALPAN_TYPES];

	LTVector m_vGlobalLightDir; // 312
	LTVector m_vGlobalLightColor; // 324

	float m_fAmbientLight; // 336

	uint32 m_dwCWO_Flags; // 340 - AddObjectToClientWorld
};

#endif
