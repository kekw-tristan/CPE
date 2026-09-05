#include "frameStatsWindow.h"

#include <imgui.h>

// -------------------------------------------------------------------------------------------------------------------------

namespace Engine::GFX 
{

    // -------------------------------------------------------------------------------------------------------------------------

    sFrameWindowStats& cFrameStatsWindow::GetFrameWindowStats()
    {
        return m_frameStats;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cFrameStatsWindow::OnDraw()
    {
        static float accumulatedTime    = 0.0f;
        static int   accumulatedFrames  = 0;
        static float averageFps         = 0.0f;

        const float deltaTime = m_frameStats.deltaTime;
        const bool gpuTimesAvailable = m_frameStats.shadowGpuMilliseconds >= 0.0 && m_frameStats.mainGpuMilliseconds >= 0.0;

        if (gpuTimesAvailable && deltaTime > 0.0f)
        {
            m_gpuAccumulatedTime += deltaTime;
            m_shadowGpuSum += m_frameStats.shadowGpuMilliseconds;
            m_mainGpuSum += m_frameStats.mainGpuMilliseconds;
            ++m_gpuSampleCount;

            if (m_gpuAccumulatedTime >= 1.0f)
            {
                m_shadowGpuAverage = m_shadowGpuSum / m_gpuSampleCount;
                m_mainGpuAverage = m_mainGpuSum / m_gpuSampleCount;

                m_gpuAccumulatedTime = 0.0f;
                m_shadowGpuSum = 0.0;
                m_mainGpuSum = 0.0;
                m_gpuSampleCount = 0;
            }
        }
        else if (!gpuTimesAvailable)
        {
            m_gpuAccumulatedTime = 0.0f;
            m_shadowGpuSum = 0.0;
            m_mainGpuSum = 0.0;
            m_gpuSampleCount = 0;
            m_shadowGpuAverage = -1.0;
            m_mainGpuAverage = -1.0;
        }

        if (deltaTime > 0.0f)
        {
            accumulatedTime += deltaTime;
            ++accumulatedFrames;

            if (accumulatedTime >= 1.0f)
            {
                averageFps = static_cast<float>(accumulatedFrames) / accumulatedTime;

                accumulatedTime = 0.0f;
                accumulatedFrames = 0;
            }
        }

        const float currentFps = deltaTime > 0.0f ? 1.0f / deltaTime : 0.0f;

        ImGui::Begin("Frame Statistics");

        ImGui::Text("FPS:           %.1f", currentFps);
        ImGui::Text("Average FPS:   %.1f", averageFps);
        ImGui::Text("DeltaTime:     %.5f", deltaTime);

        ImGui::Separator();

        if (gpuTimesAvailable)
        {
            ImGui::Text("GPU shadows:   %.3f ms", m_frameStats.shadowGpuMilliseconds);
            if (m_shadowGpuAverage >= 0.0)
            {
                ImGui::Text("Average (1 s): %.3f ms", m_shadowGpuAverage);
            }
            else
            {
                ImGui::TextUnformatted("Average (1 s): collecting...");
            }

            ImGui::Spacing();
            ImGui::Text("GPU main + UI: %.3f ms", m_frameStats.mainGpuMilliseconds);
            if (m_mainGpuAverage >= 0.0)
            {
                ImGui::Text("Average (1 s): %.3f ms", m_mainGpuAverage);
            }
            else
            {
                ImGui::TextUnformatted("Average (1 s): collecting...");
            }

            ImGui::Spacing();
            ImGui::TextWrapped("Compare castsShadow on/off at the same camera position. Main-pass difference includes shadow sampling. Pass timings may overlap.");
        }
        else
        {
            ImGui::TextUnformatted("GPU timings unavailable / waiting for completed frame");
        }

        ImGui::Separator();

        ImGui::Text("Draw calls:    %i", m_frameStats.drawCalls);
        ImGui::Text("Instances:     %i", m_frameStats.instances);

        ImGui::End();
    }

    // -------------------------------------------------------------------------------------------------------------------------

}

// -------------------------------------------------------------------------------------------------------------------------
