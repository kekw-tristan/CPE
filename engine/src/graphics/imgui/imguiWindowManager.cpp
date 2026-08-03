#include "imguiWindowManager.h"

#include "graphics/imgui/imguiWindow.h"

#include <assert.h>
#include <vector>

// -------------------------------------------------------------------------------------------------------------------------

namespace Engine::GFX
{

    // -------------------------------------------------------------------------------------------------------------------------

    namespace
    {

        // -------------------------------------------------------------------------------------------------------------------------

        class cImGuiWindowManager
        {
            public:

                static cImGuiWindowManager& GetInstance(); 

            public:

                void AddWindow(cImGuiWindow& _rWindow); 
                void Draw();

            private:

                cImGuiWindowManager() = default; 
               ~cImGuiWindowManager() = default; 

               cImGuiWindowManager(const cImGuiWindowManager&)              = delete;
               cImGuiWindowManager& operator=(const cImGuiWindowManager&)   = delete;

            private:

                std::vector<cImGuiWindow*> m_imguiWindows;

        };

        // -------------------------------------------------------------------------------------------------------------------------

    }

    // -------------------------------------------------------------------------------------------------------------------------

    namespace
    {

        // -------------------------------------------------------------------------------------------------------------------------

        cImGuiWindowManager& cImGuiWindowManager::GetInstance()
        {
            static cImGuiWindowManager s_instance; 
            return s_instance;
        }

        // -------------------------------------------------------------------------------------------------------------------------

        void cImGuiWindowManager::AddWindow(cImGuiWindow& _rWindow)
        {
            m_imguiWindows.push_back(&_rWindow);
        }

        // -------------------------------------------------------------------------------------------------------------------------

        void cImGuiWindowManager::Draw() 
        {
            for (auto* pWindow : m_imguiWindows)
            {
                assert(pWindow != nullptr);
                
                pWindow->Draw();
            }
        }

        // -------------------------------------------------------------------------------------------------------------------------

    }

    // -------------------------------------------------------------------------------------------------------------------------

    namespace ImGuiWindowManager
    {
        // -------------------------------------------------------------------------------------------------------------------------

        void AddWindow(cImGuiWindow& _rWindow)
        {
            cImGuiWindowManager::GetInstance().AddWindow(_rWindow);
        }

        // -------------------------------------------------------------------------------------------------------------------------

        void Draw()
        {
            cImGuiWindowManager::GetInstance().Draw();
        }

        // -------------------------------------------------------------------------------------------------------------------------
    }

    // -------------------------------------------------------------------------------------------------------------------------
    
}
