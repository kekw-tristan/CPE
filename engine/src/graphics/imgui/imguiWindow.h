#pragma once 

namespace Engine::GFX
{

    class cImGuiWindow
    {
        public:
    
            cImGuiWindow()          = default;
            virtual ~cImGuiWindow() = default;
    
            cImGuiWindow(const cImGuiWindow&)           = delete;
            cImGuiWindow& operator=(const cImGuiWindow) = delete;
    
        public:
    
            void Draw() const; 
    
        protected:
    
            virtual void OnDraw() const = 0;
    
    };

}

