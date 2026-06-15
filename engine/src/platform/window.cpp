#include "platform/window.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <stdexcept> 
#include "window.h"

// -------------------------------------------------------------------------------------------------------------------------

namespace Engine::Platform
{

    // -------------------------------------------------------------------------------------------------------------------------
    // window constructor
    // creates glfw window

    cWindow::cWindow(int _width, int _height, const char* _pTitle)
        : m_pWindow(nullptr)
        , m_width(_width)
        , m_height(_height)
        , m_hasFramebufferResized(false)
    {
        if(!glfwInit())
        {
            throw std::runtime_error("Failed to initialize GLFW."); 
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE); 

        m_pWindow = glfwCreateWindow(m_width, m_height, _pTitle, nullptr, nullptr); 

        if(!m_pWindow)
        {
            glfwTerminate();
            throw std::runtime_error("Failed to create GFLW window!");
        }

        glfwSetWindowUserPointer(m_pWindow, this); 

        glfwSetFramebufferSizeCallback(m_pWindow, FramebufferResizeCallback);
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

    // -------------------------------------------------------------------------------------------------------------------------

    int cWindow::GetWidth() const
    {
        return m_width;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    int Engine::Platform::cWindow::GetHeight() const
    {
        return m_height;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    bool cWindow::WasResized() const
    {
        return m_hasFramebufferResized;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cWindow::ResetRezisedFlag()
    {
        m_hasFramebufferResized = false; 
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cWindow::WaitUntilFramebufferHasSize()
    {
        int width  = 0;
        int height = 0;

        glfwGetFramebufferSize(m_pWindow, &width, &height); 

        while (width == 0 || height == 0)
        {
            glfwWaitEvents();
            glfwGetFramebufferSize(m_pWindow, &width, &height);
        }

        m_width  = width;
        m_height = height; 
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cWindow::FramebufferResizeCallback(GLFWwindow *_pWindow, int _width, int _height)
    {
        cWindow* pWindow = reinterpret_cast<cWindow*>(glfwGetWindowUserPointer(_pWindow));   

        pWindow->m_width                    = _width; 
        pWindow->m_height                   = _height;
        pWindow->m_hasFramebufferResized    = true;
    }

    // -------------------------------------------------------------------------------------------------------------------------

}

// -------------------------------------------------------------------------------------------------------------------------


