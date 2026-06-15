#pragma once 

struct GLFWwindow; 

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

        private:

            static void FramebufferResizeCallback(GLFWwindow* _pWindow, int _width, int _height);

        private:

            GLFWwindow* m_pWindow;

            int m_width;
            int m_height;

            bool m_hasFramebufferResized;
    };
}