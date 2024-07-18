#ifndef __COMMON_DATA_MGR_H__
#define __COMMON_DATA_MGR_H__

class CExpirableData
{

public:

	enum CompareResult
	{
		COMP_R_Unset = -1,
		COMP_R_Equal = 0,
		COMP_R_SizeDiff,
		COMP_R_SizeDiffMore,
		COMP_R_ContentDiff,
		COMP_R_Max
	};

	CExpirableData() { m_fLastUpdate = 0.0f; };

	bool	IsExpired(float fCurrTime, float fLifeTime)
	{
		return (fCurrTime - m_fLastUpdate > fLifeTime);
	}

	void	SetLastUpdate(float fSet) { m_fLastUpdate = fSet; }

private:

	float			m_fLastUpdate;
};

class CBaseDataManager
{

public:

	CBaseDataManager()
	{
		m_bInitialized = false;
	}

	virtual ~CBaseDataManager()
	{
		if (m_bInitialized)
			Term();
	}

	virtual void Init()
	{
		m_bInitialized = true;
	}

	virtual void Term()
	{
		FreeAllData();

		m_bInitialized = false;
	}

	virtual void FreeAllData() = 0;

private:

	bool	m_bInitialized;
};

template <class V>
class CExpirableDataManager2 : public CBaseDataManager
{

public:

	virtual void FreeAllData()
	{
		for (auto item : m_Data)
		{
			if (item != nullptr)
				delete item;
		}

		m_Data.clear();
	}

	void Update(float fCurrTime, float fLifeTime)
	{
		for (uint32 i = 0; i < m_Data.size(); i++)
		{
			if (m_Data[i] != nullptr && m_Data[i]->IsExpired(fCurrTime, fLifeTime))
			{
				delete m_Data[i];
				m_Data[i] = nullptr;
			}
		}
	}

	std::vector<V>	m_Data;
};

template <class K, class V>
class CExpirableDataManager : public CBaseDataManager
{

public:

	virtual void FreeAllData()
	{
		for (auto& pair : m_Data)
			delete pair.second;

		m_Data.clear();
	}

	void Update(float fCurrTime, float fLifeTime)
	{
		for (auto iter = m_Data.begin(); iter != m_Data.end();)
		{
			auto& pair = *iter;

			if (pair.second->IsExpired(fCurrTime, fLifeTime))
			{
				delete pair.second;
				iter = m_Data.erase(iter);
			}
			else
			{
				iter++;
			}
		}
	}

	std::unordered_map<K, V>	m_Data;
};

#endif