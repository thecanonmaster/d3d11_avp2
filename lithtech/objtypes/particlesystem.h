// Unverified
struct StructBank;

#define PS_BOUNCE	(1<<0)
#define PS_NEVERDIE	(1<<2)
#define PS_DUMB		(1<<3)

struct LTParticle
{
	LTVector	m_vVel; // 0
	
	LTVector	m_vColor; // 12
	float		m_fAlpha; // 24
	
	float	m_fSize; // 28

    LTParticle*	m_pNext; // 32 ?

	LTVector	m_vPos; // 36

    float	m_fLifetime; // 48
    float	m_fTotalLifetime; // 52

    LTParticle*	m_pPrev; //	56 ?
};

struct ParticleSystem
{
	LTObject	m_Base;

	LTParticle	m_ParticleHead; // 432

    StructBank*		m_pParticleBank; // 492 ?
    SharedTexture*	m_pCurTexture; // 496 ?

	uint8	m_nSoftwareR; // 500
    uint8	m_nSoftwareG;
    uint8	m_nSoftwareB;
    uint8	m_nPadding; // Alpha?
	
	Sprite*			m_pSprite; // 504 ?
    SpriteTracker	m_SpriteTracker; // 508 ?

	LTVector	m_vSystemCenter; // 528 ?
    float		m_fSystemRadius; // 540 ?	
    LTVector	m_vOldCenter; // 544 ?
    float		m_fOldRadius; // 556 ?

	int	m_nParticles; // 560 ?
    int	m_nChangedParticles; // 564 ?
	
    LTVector	m_vMinPos; // 568 ?
	LTVector	m_vMaxPos; // 580 ?

	float	m_fGravityAccel; // 592 ?

	float	m_fParticleRadius; // 596 ?

	int	m_nSrcBlend; // 600
    int	m_nDestBlend; // 604

	uint32	m_dwSystemFlags; // 608 ?
};