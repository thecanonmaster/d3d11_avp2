#ifndef __D3D_CONVAR_H__
#define __D3D_CONVAR_H__

typedef void* HLTPARAM;

class BaseConVar;
extern BaseConVar* g_pConVars;

inline int RoundFloatToInt(float fVal)
{
	int nResult;

	__asm
	{
		fld fVal
		fistp nResult
	}

	return nResult;
}

class BaseConVar
{

public:

	BaseConVar(const char* szName, float fDefaultVal)
	{
		m_DefaultVal = fDefaultVal;
		m_szName = szName;
		m_hParam = nullptr;

		m_pNext = g_pConVars;
		g_pConVars = this;
	}

	virtual void	SetFloat(float fVal) = 0;
	virtual float	GetFloat() const = 0;

	float	m_DefaultVal;

	const char*	m_szName;

	HLTPARAM	m_hParam;

	BaseConVar*	m_pNext;
};

template <class T>
class ConVar : public BaseConVar
{

public:

	ConVar(const char* szName, T Val) : BaseConVar(szName, (float)Val) { m_Val = Val; }

	const T& operator=(const T& rhs) { m_Val = rhs; return rhs; }
	operator T() const { return m_Val; }

	virtual void	SetFloat(float fVal) { m_Val = RoundFloatToInt(fVal); }
	virtual float	GetFloat() const { return (float)m_Val; }

	T	m_Val;
};

template <>
class ConVar <float> : public BaseConVar
{

public:

	ConVar(const char* szName, float Val) : BaseConVar(szName, (float)Val) { m_Val = Val; }

	const float& operator=(const float& rhs) { m_Val = rhs; return rhs; }
	operator float() const { return m_Val; }

	virtual void	SetFloat(float fVal) { m_Val = fVal; }
	virtual float	GetFloat() const { return m_Val; }

	float	m_Val;
};

#endif