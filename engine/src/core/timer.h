#pragma once

#include <chrono>

namespace Engine::Core
{
    class cTimer
    {
        public:

            cTimer();
            ~cTimer() = default;

            cTimer(const cTimer&)               = delete;
            cTimer& operator=(const cTimer&)    = delete;

        public:

            void Reset();
            void Tick();

            float GetDeltaTime() const;
            float GetTotalTime() const;

        private:

            using Clock     = std::chrono::steady_clock;
            using TimePoint = std::chrono::time_point<Clock>;

        private:
            TimePoint m_startTime;
            TimePoint m_previousTime;
            TimePoint m_currentTime;

            float m_deltaTime;
            float m_totalTime;
    };
}