#include "platform/window.h"

#include <GLFW/glfw3.h>

#include <algorithm>
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
        , m_isFullscreen(false) 
        , m_windowedX(100)
        , m_windowedY(100)
        , m_windowedWidth(_width) 
        , m_windowedHeight(_height) 
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

    void cWindow::ToggleFullscreen()
    {
        if (!m_pWindow)
        {
            return;
        }

        m_hasFramebufferResized = true;

        if (!m_isFullscreen)
        {
            glfwGetWindowPos(m_pWindow, &m_windowedX, &m_windowedY);
            glfwGetWindowSize(m_pWindow, &m_windowedWidth, &m_windowedHeight);

            GLFWmonitor*       pMonitor = GetCurrentMonitor();
            const GLFWvidmode* pMode    = glfwGetVideoMode(pMonitor);

            glfwSetWindowMonitor(m_pWindow, pMonitor, 0, 0, pMode->width, pMode->height, pMode->refreshRate);

            m_isFullscreen = true; 

        }
        else
        {
            glfwSetWindowMonitor(m_pWindow, nullptr, m_windowedX, m_windowedY, m_windowedWidth, m_windowedHeight, 0);
        
            m_isFullscreen = false;
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------

    bool cWindow::IsFullscreen()
    {
        return false;
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

    GLFWmonitor* cWindow::GetCurrentMonitor()
    {
        int windowX         = 0; 
        int windowY         = 0; 
        int windowWidth     = 0;
        int windowHeight    = 0;

        glfwGetWindowPos(m_pWindow, &windowX, &windowY); 
        glfwGetWindowSize(m_pWindow, &windowWidth, &windowHeight);

        int monitorCount = 0; 
        GLFWmonitor** ppMonitors = glfwGetMonitors(&monitorCount);

        if (monitorCount == 0 || ppMonitors == nullptr)
        {
            return glfwGetPrimaryMonitor();
        }

        GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
        int primaryOverlap = 0;

        for (int index = 0; index < monitorCount; ++index)
        {
            GLFWmonitor* pMonitor = ppMonitors[index];

            int monitorX = 0;
            int monitorY = 0;

            glfwGetMonitorPos(pMonitor, &monitorX, &monitorY);

            const GLFWvidmode* pMode = glfwGetVideoMode(pMonitor);

            if (!pMode)
            {
                continue;
            }

            int monitorWidth  = pMode->width;
            int monitorHeight = pMode->height;

            int overlapLeft     = std::max(windowX, monitorX);
            int overlapTop      = std::max(windowY, monitorY);

            int overlapRight    = std::min(windowX + windowWidth,  monitorX + monitorWidth);
            int overlapBottom   = std::min(windowY + windowHeight, monitorY + monitorHeight);

            int overlapWidth    = std::max(0, overlapRight  - overlapLeft);
            int overlapHeight   = std::max(0, overlapBottom - overlapTop);

            int overlapArea = overlapWidth * overlapHeight;

            if (overlapArea > primaryOverlap)
            {
                primaryOverlap = overlapArea; 
                primaryMonitor = pMonitor;
            }

        }

        return primaryMonitor;
    }

    // -------------------------------------------------------------------------------------------------------------------------

}

// -------------------------------------------------------------------------------------------------------------------------


