#ifndef __D3D_TEXTURE_H__
#define __D3D_TEXTURE_H__

#ifndef __D3D11_H__
#include <d3d11_1.h>
#define __D3D11_H__
#endif

#define RT_FULLBRITE	(1<<1)

static const char* g_szFreeError_SRV = "Failed to free D3D resource view (count = %d)";

class RTexture;
typedef std::list<RTexture*> List_PRTexture;

void d3d_BindTexture(SharedTexture* pSharedTexture, LTBOOL bTextureChanged);
void d3d_UnbindTexture(SharedTexture* pSharedTexture);

class RTexture
{

public:

	RTexture() : m_anColorKey()
	{
		m_dwFormat = 0;
		m_pResourceView = nullptr;
		m_dwBaseWidth = 0;
		m_dwBaseHeight = 0;
		m_dwScaledWidth = 0;
		m_dwScaledHeight = 0;
		m_pSharedTexture = nullptr;
		m_fDetailTextureScale = 0.0f;
		m_fDetailTextureAngleC = 0.0f;
		m_fDetailTextureAngleS = 0.0f;
		m_dwFlags = 0;
		m_fAlphaRef = 0.0f;
	}

	~RTexture() { }

	bool	IsFullbrite() const { return (m_dwFlags & RT_FULLBRITE); }

	uint32	m_dwFormat;

	ID3D11ShaderResourceView*	m_pResourceView;

	uint32	m_dwBaseWidth;
	uint32	m_dwBaseHeight;
	uint32	m_dwScaledWidth;
	uint32	m_dwScaledHeight;

	SharedTexture*	m_pSharedTexture;

	float	m_fDetailTextureScale;
	float	m_fDetailTextureAngleC;
	float	m_fDetailTextureAngleS;

	uint32	m_dwFlags;

	float	m_fAlphaRef;

	uint8	m_anColorKey[3];

	List_PRTexture::iterator	m_GlobalIter;
};

class CTextureManager
{

public:

	CTextureManager()
	{
		m_bInitialized = false;
	}

	~CTextureManager()
	{
		if (m_bInitialized) 
			Term();
	}

	void	Init();
	void	Term();

	RTexture*	CreateRTexture(SharedTexture* pSharedTexture, TextureData* pTextureData);
	bool		UploadRTexture(RTexture* pTexture, TextureData* pTextureData);

	void	FreeTexture(RTexture* pTexture);
	void	FreeAllTextures();

	static uint32	QueryDXFormat(uint32 dwBPP, uint32 dwFlags);

	List_PRTexture m_Textures;

private:

	bool	m_bInitialized;
};

#endif