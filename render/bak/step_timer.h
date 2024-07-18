#ifndef _TIMER_H_
#define _TIMER_H_

typedef long long          int64;
typedef unsigned long long uint64;

#define TICKS_PER_SECOND    10000000

class StepTimer 
{ 

public:

    StepTimer()
    {
        m_qwElapsedTicks = 0;
        m_qwTotalTicks = 0;
        m_qwLeftOverTicks = 0;
        m_dwFrameCount = 0;
        m_dwFramesPerSecond = 0;
        m_dwFramesThisSecond = 0;
        m_qwSecondCounter = 0;

        m_qwTargetElapsedTicks = TICKS_PER_SECOND / 60;

        QueryPerformanceFrequency(&m_liFrequency);
        QueryPerformanceCounter(&m_liLastTime);

        m_qwMaxDelta = (uint64)(m_liFrequency.QuadPart / 10);
    }

    static double TicksToSeconds(uint64 qwTicks) { return (double)(qwTicks) / TICKS_PER_SECOND; }
    static uint64 SecondsToTicks(double fSeconds) { return (uint64)(fSeconds * TICKS_PER_SECOND); }

    uint64  GetElapsedTicks() { return m_qwElapsedTicks; }
    double  GetElapsedSeconds() { return TicksToSeconds(m_qwElapsedTicks); }

    uint32  GetFramesPerSecond() { return m_dwFramesPerSecond; }

    void    SetTargetElapsedTicks(uint64 qwSet) { m_qwTargetElapsedTicks = qwSet; }
    void    SetTargetElapsedSeconds(double fSet) noexcept { m_qwTargetElapsedTicks = SecondsToTicks(fSet); }

    void ResetElapsedTime()
    {
        QueryPerformanceCounter(&m_liLastTime);

        m_qwLeftOverTicks = 0;
        m_dwFramesPerSecond = 0;
        m_dwFramesThisSecond = 0;
        m_qwSecondCounter = 0;
    }

    template<typename TUpdate>
    void Tick(const TUpdate& Update)
    {
        LARGE_INTEGER liCurrentTime;
        QueryPerformanceCounter(&liCurrentTime);

        uint64 qwTimeDelta = (uint64)(liCurrentTime.QuadPart - m_liLastTime.QuadPart);

        m_liLastTime = liCurrentTime;
        m_qwSecondCounter += qwTimeDelta;

        if (qwTimeDelta > m_qwMaxDelta)
            qwTimeDelta = m_qwMaxDelta;

        qwTimeDelta *= TICKS_PER_SECOND;
        qwTimeDelta /= (uint64)(m_liFrequency.QuadPart);

        uint32 dwLastFrameCount = m_dwFrameCount;

        if ((uint64)(std::abs((int64)(qwTimeDelta - m_qwTargetElapsedTicks))) < TICKS_PER_SECOND / 4000)
            qwTimeDelta = m_qwTargetElapsedTicks;

        m_qwLeftOverTicks += qwTimeDelta;

        while (m_qwLeftOverTicks >= m_qwTargetElapsedTicks)
        {
            m_qwElapsedTicks = m_qwTargetElapsedTicks;
            m_qwTotalTicks += m_qwTargetElapsedTicks;
            m_qwLeftOverTicks -= m_qwTargetElapsedTicks;
            m_dwFrameCount++;

            Update();
        }

        if (m_dwFrameCount != dwLastFrameCount)
            m_dwFramesThisSecond++;

        if (m_qwSecondCounter >= (uint64)(m_liFrequency.QuadPart))
        {
            m_dwFramesPerSecond = m_dwFramesThisSecond;
            m_dwFramesThisSecond = 0;
            m_qwSecondCounter %= (uint64)(m_liFrequency.QuadPart);
        }
    }

private:

    LARGE_INTEGER   m_liFrequency;
    LARGE_INTEGER   m_liLastTime;
    uint64          m_qwMaxDelta;

    uint64  m_qwElapsedTicks;
    uint64  m_qwTotalTicks;
    uint64  m_qwLeftOverTicks;

    uint32  m_dwFrameCount;
    uint32  m_dwFramesPerSecond;
    uint32  m_dwFramesThisSecond;
    uint64  m_qwSecondCounter;

    uint64  m_qwTargetElapsedTicks;
};

#endif
