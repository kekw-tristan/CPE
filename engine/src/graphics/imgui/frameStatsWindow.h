#pragma once 

#include "graphics/imgui/imguiWindow.h"

namespace Engine::GFX
{
    struct sFrameWindowStats
    {
        float deltaTime;

        int drawCalls;
        int instances;
    };

    class cFrameStatsWindow : public cImGuiWindow
    {

        public:

            cFrameStatsWindow() = default; 
           ~cFrameStatsWindow() = default;

           cFrameStatsWindow(const cFrameStatsWindow&)              = delete;
           cFrameStatsWindow& operator=(const cFrameStatsWindow&)   = delete;

        public:

            void SetDeltaTime(float _deltaTime);
            sFrameWindowStats& GetFrameWindowStats();

        protected:

            void OnDraw() const override;

        private:

            sFrameWindowStats m_frameStats = {};

    };
}

