#pragma once 

namespace Engine::GFX
{
    class cImGuiWindow;

    namespace ImGuiWindowManager
    {
        void AddWindow(cImGuiWindow& _rWindow);
        void Draw();
    }
}