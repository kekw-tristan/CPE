#pragma once 

struct GLFWwindow; 
struct GLFWmonitor;

namespace Engine::Platform
{
    class cWindow
    {
        public:

            cWindow(int _width, int _height, const char* _pTitle);
           ~cWindow();

            cWindow(const cWindow&)             = delete; 
            cWindow& operator=(const cWindow&)  = delete;

        public:

            bool ShouldClose() const;
            void PollEvents(); 
            
            GLFWwindow* GetWindow() const;

            int GetWidth()  const;
            int GetHeight() const;

            bool WasResized() const;
            void ResetRezisedFlag();
            
            void WaitUntilFramebufferHasSize(); 

            void ToggleFullscreen(); 
            bool IsFullscreen();


        private:

            static void FramebufferResizeCallback(GLFWwindow* _pWindow, int _width, int _height);
            GLFWmonitor* GetCurrentMonitor();

        private:

            GLFWwindow* m_pWindow;

            int m_width;
            int m_height;

            bool m_hasFramebufferResized;

            bool m_isFullscreen; 

            int m_windowedX;
            int m_windowedY;
            int m_windowedWidth; 
            int m_windowedHeight; 

    };
}