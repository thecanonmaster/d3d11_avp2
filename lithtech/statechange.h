#ifndef __LT_STATE_CHANGE__
#define __LT_STATE_CHANGE__

template <typename T>
class StdVector
{

public:

	StdVector()
	{
		m_pFirst = nullptr;
		m_pLast = nullptr;
		m_pEnd = nullptr;
	}

	StdVector(const StdVector<T>&& other) noexcept : StdVector()
	{
		size_t dwSize = other.size();

		if (!dwSize)
			return;

		m_pFirst = new T[dwSize];
		m_pLast = m_pFirst + dwSize;
		m_pEnd = m_pLast;

		memcpy(m_pFirst, other.m_pFirst, sizeof(T) * dwSize);
	}

	StdVector(std::initializer_list<T> init) : StdVector()
	{
		size_t dwSize = init.size();

		if (!dwSize)
			return;
		
		m_pFirst = new T[dwSize];

		T* pItem = m_pFirst;
		const T* pInitItem = init.begin();

		while (pInitItem != init.end())
		{
			*pItem = *pInitItem;

			pItem++;
			pInitItem++;
		}

		m_pLast = pItem;
		m_pEnd = pItem;
	};

	~StdVector() 
	{
		if (m_pFirst != nullptr)
			delete [] m_pFirst;
	};

	T&	operator[](size_t dwPos) const { return m_pFirst[dwPos]; }

	size_t	size() const { return m_pLast - m_pFirst; };

private:

	T*	m_pFirst;
	T*	m_pLast;
	T*	m_pEnd;
};

struct RenderState
{
	uint32	m_dwRenderStateType;
	uint32	m_dwRenderState;

	bool CompareTo(RenderState& other) 
	{ 
		return m_dwRenderStateType == other.m_dwRenderStateType && m_dwRenderState == other.m_dwRenderState;
	}
};

struct TextureState
{
	uint32	m_dwStage;
	uint32	m_dwTextureStateType;
	uint32	m_dwTextureState;

	bool CompareTo(TextureState& other)
	{
		return m_dwStage == other.m_dwStage && m_dwTextureStateType == other.m_dwTextureStateType && 
			m_dwTextureState == other.m_dwTextureState;
	}
};

struct StateChange
{
	StateChange(std::initializer_list<RenderState> initRS, std::initializer_list<TextureState> initTS) :
		m_RenderList(initRS), m_TextureList(initTS) { };

	typedef StdVector<RenderState>	RenderContainer;
	typedef StdVector<TextureState>	TextureContainer;
	
	RenderContainer		m_RenderList;
	TextureContainer	m_TextureList;
};

#endif
