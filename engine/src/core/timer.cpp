#include "timer.h"

// -------------------------------------------------------------------------------------------------------------------------

namespace Engine::Core
{

    // -------------------------------------------------------------------------------------------------------------------------

    cTimer::cTimer()
        : m_startTime(Clock::now())
        , m_previousTime(m_startTime)
        , m_currentTime(m_startTime)
        , m_deltaTime(0.0f)
        , m_totalTime(0.0f)
    {
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cTimer::Reset()
    {
        m_startTime     = Clock::now(); 
        m_previousTime  = m_startTime; 
        m_currentTime   = m_startTime;

        m_deltaTime = 0.f;
        m_totalTime = 0.f; 
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cTimer::Tick()
    {
        m_currentTime = Clock::now();

        const std::chrono::duration<float> deltaDuration = m_currentTime - m_previousTime;
        const std::chrono::duration<float> totalDuration = m_currentTime - m_startTime;

        m_deltaTime = deltaDuration.count();
        m_totalTime = totalDuration.count();

        m_previousTime = m_currentTime;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    float cTimer::GetDeltaTime() const
    {
        return m_deltaTime;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    float cTimer::GetTotalTime() const
    {
        return m_totalTime;
    }

    // -------------------------------------------------------------------------------------------------------------------------

}

// -------------------------------------------------------------------------------------------------------------------------