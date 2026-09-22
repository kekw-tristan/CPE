#pragma once 

#include "graphics/imgui/imguiWindow.h"

namespace Engine::GFX
{
    struct sFrameWindowStats
    {
        float deltaTime;
        double shadowGpuMilliseconds = -1.0;
        double mainGpuMilliseconds = -1.0;

        int drawCalls;
        int instances;
        int mainInstances = 0;
        int shadowInstances = 0;
        int reflectionInstances = 0;
        int occlusionInstances = 0;
    };

    class cFrameStatsWindow : public cImGuiWindow
    {

        public:

            cFrameStatsWindow() = default; 
           ~cFrameStatsWindow() = default;

           cFrameStatsWindow(const cFrameStatsWindow&)              = delete;
           cFrameStatsWindow& operator=(const cFrameStatsWindow&)   = delete;

        public:

            sFrameWindowStats& GetFrameWindowStats();

        protected:

            void OnDraw() override;

        private:

            sFrameWindowStats m_frameStats = {};

            float m_gpuAccumulatedTime = 0.0f;
            int m_gpuSampleCount = 0;
            double m_shadowGpuSum = 0.0;
            double m_mainGpuSum = 0.0;
            double m_shadowGpuAverage = -1.0;
            double m_mainGpuAverage = -1.0;

    };
}

