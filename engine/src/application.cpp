#include "application.h"

// -------------------------------------------------------------------------------------------------------------------------

namespace Engine
{

    // -------------------------------------------------------------------------------------------------------------------------
    // application contructor
    // initializes window and vulkan context
    
    cApplication::cApplication()
        : m_window(1280, 720, "Vulkan Engine")
    {
        m_vulkanContext  .Init(m_window);
        m_vulkanDevice   .Init(m_vulkanContext);
        m_vulkanSwapchain.Init(m_vulkanContext, m_vulkanDevice, m_window);
        m_vulkanCommands .Init(m_vulkanDevice);
        m_vulkanSync     .Init(m_vulkanDevice);
    }

    // -------------------------------------------------------------------------------------------------------------------------
    // application destructor

    cApplication::~cApplication()
    {
        m_vulkanSync     .Shutdown(m_vulkanDevice);
        m_vulkanCommands .Shutdown(m_vulkanDevice);
        m_vulkanSwapchain.Shutdown(m_vulkanDevice);
        m_vulkanDevice   .Shutdown();
        m_vulkanContext  .Shutdown();
    }

    // -------------------------------------------------------------------------------------------------------------------------
    // game loop

    void cApplication::Run()
    {
        while (!m_window.ShouldClose())
        {
            m_window.PollEvents();
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------

}

// -------------------------------------------------------------------------------------------------------------------------