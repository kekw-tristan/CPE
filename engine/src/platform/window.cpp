#include "platform/window.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <stdexcept> 

// -------------------------------------------------------------------------------------------------------------------------

namespace Engine::Platform
{

    // -------------------------------------------------------------------------------------------------------------------------
    // window constructor
    // creates glfw window

    cWindow::cWindow(int _width, int _height, const char* _pTitle)
    {
        if(!glfwInit())
        {
            throw std::runtime_error("Failed to initialize GLFW."); 
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE); 

        m_pWindow = glfwCreateWindow(_width, _height, _pTitle, nullptr, nullptr); 

        if(!m_pWindow)
        {
            glfwTerminate();
            throw std::runtime_error("Failed to create GFLW window!");
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------
    // window destructor

    cWindow::~cWindow()
    {
        if (m_pWindow)
        {
            glfwDestroyWindow(m_pWindow);
            m_pWindow = nullptr;
        }

        glfwTerminate();
    }
    
    // -------------------------------------------------------------------------------------------------------------------------

    bool cWindow::ShouldClose() const
    {
        return glfwWindowShouldClose(m_pWindow);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cWindow::PollEvents()
    {
        glfwPollEvents();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    GLFWwindow* cWindow::GetWindow() const
    {
        return m_pWindow;
    }

}

// -------------------------------------------------------------------------------------------------------------------------