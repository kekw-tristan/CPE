#pragma once 

namespace Engine::GFX
{
    class cImGuiWindow;

    namespace ImGuiWindowManager
    {
        void AddWindow(const cImGuiWindow& _rWindow);
        void Draw();
    }
}