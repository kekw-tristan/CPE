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

    void cFrameStatsWindow::OnDraw() const
    {
        ImGui::Begin("Frame Statistics");

        ImGui::Text("FPS:           %.1f", 1 / m_frameStats.deltaTime);

        ImGui::Text("DeltaTime:     %.5f", m_frameStats.deltaTime);

        ImGui::Separator();

        ImGui::Text("Draw calls:    %i", m_frameStats.drawCalls);

        ImGui::Text("Instances:     %i", m_frameStats.instances);

        ImGui::End();
    }

    // -------------------------------------------------------------------------------------------------------------------------

}

// -------------------------------------------------------------------------------------------------------------------------
