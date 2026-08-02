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

                void AddWindow(const cImGuiWindow&); 
                void Draw() const;

            private:

                cImGuiWindowManager() = default; 
               ~cImGuiWindowManager() = default; 

               cImGuiWindowManager(const cImGuiWindowManager&)              = delete;
               cImGuiWindowManager& operator=(const cImGuiWindowManager&)   = delete;

            private:

                std::vector<const cImGuiWindow*> m_imguiWindows;

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

        void cImGuiWindowManager::AddWindow(const cImGuiWindow& _rWindow)
        {
            m_imguiWindows.push_back(&_rWindow);
        }

        // -------------------------------------------------------------------------------------------------------------------------

        void cImGuiWindowManager::Draw() const
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

        void AddWindow(const cImGuiWindow& _rWindow)
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
