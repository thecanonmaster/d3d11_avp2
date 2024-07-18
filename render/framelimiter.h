#ifndef __FRAMELIMITER_H__
#define __FRAMELIMITER_H__

class FrameLimiter
{

public:

	FrameLimiter()
	{
		QueryPerformanceFrequency(&m_liFrequency);
		QueryPerformanceCounter(&m_liStart);

		m_fTargetFrameRate = 60.0;
		m_dwPerCycleSleep = 0;
		m_fFrameRateUpdate = 0.2f;
		m_fLastFrameRateUpdate = 0.0f;
		m_dwLastFrameRate = 0;
	}

	void	SetTargetFrameRate(uint32 dwSet) { m_fTargetFrameRate = (double)dwSet; }
	
	void	SetFrameRateUpdate(float fSet) { m_fFrameRateUpdate = fSet; }
	uint32	GetLastFrameRate() { return m_dwLastFrameRate; }

	double CalcDifference(LARGE_INTEGER* pEnd)
	{
		return double(pEnd->QuadPart - m_liStart.QuadPart) / double(m_liFrequency.QuadPart / 1000);
	}

	void DebugOutFramerate()
	{
		char szBuffer[64];
		sprintf_s(szBuffer, sizeof(szBuffer) - 1, "FPS = %d\r", m_dwLastFrameRate);

		OutputDebugString(szBuffer);
	}

	void Update()
	{
		LARGE_INTEGER liEnd;
		QueryPerformanceCounter(&liEnd);
		double fDiff = CalcDifference(&liEnd);

		while (fDiff < 1000.0 / m_fTargetFrameRate)
		{
			Sleep(m_dwPerCycleSleep);

			QueryPerformanceCounter(&liEnd);
			fDiff = CalcDifference(&liEnd);
		}

		float fTime = Timer_GetTime();
		if (fTime - m_fLastFrameRateUpdate > m_fFrameRateUpdate)
		{
			m_fLastFrameRateUpdate = fTime;
			m_dwLastFrameRate = (uint32)(1000.0f / fDiff + 0.1f);
		}

		QueryPerformanceCounter(&m_liStart);
	}

	void UpdateNoSleep()
	{
		LARGE_INTEGER liEnd;
		QueryPerformanceCounter(&liEnd);
		double fDiff = CalcDifference(&liEnd);

		float fTime = Timer_GetTime();
		if (fTime - m_fLastFrameRateUpdate > m_fFrameRateUpdate)
		{
			m_fLastFrameRateUpdate = fTime;
			m_dwLastFrameRate = (uint32)(1000.0f / fDiff + 0.1f);
		}

		QueryPerformanceCounter(&m_liStart);
	}

private:

	LARGE_INTEGER	m_liFrequency;
	LARGE_INTEGER	m_liStart;

	double	m_fTargetFrameRate;

	uint32	m_dwPerCycleSleep;

	float	m_fFrameRateUpdate;
	float	m_fLastFrameRateUpdate;
	uint32	m_dwLastFrameRate;
};

#endif