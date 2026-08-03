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

        ImGui::Text("Draw calls:    %i", m_frameStats.drawCalls);
        ImGui::Text("Instances:     %i", m_frameStats.instances);

        ImGui::End();
    }

    // -------------------------------------------------------------------------------------------------------------------------

}

// -------------------------------------------------------------------------------------------------------------------------
