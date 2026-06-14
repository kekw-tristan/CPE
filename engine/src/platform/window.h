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

        private:

            GLFWwindow* m_pWindow;
    };
}