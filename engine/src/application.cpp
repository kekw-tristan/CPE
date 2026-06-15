#include "application.h"

#include "graphics/vulkan/vulkanBuffer.h"
#include "graphics/vulkan/vulkanVertex.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <iostream>

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
    
        std::vector<Engine::GFX::sVulkanVertex> vertices =
        {
            {
                {-0.5f, -0.5f, 0.0f},
                { 0.0f,  0.0f, 1.0f},
                { 0.0f,  0.0f},
                { 1.0f,  0.0f, 0.0f, 1.0f}
            },
            {
                { 0.5f, -0.5f, 0.0f},
                { 0.0f,  0.0f, 1.0f},
                { 1.0f,  0.0f},
                { 0.0f,  1.0f, 0.0f, 1.0f}
            },
            {
                { 0.5f,  0.5f, 0.0f},
                { 0.0f,  0.0f, 1.0f},
                { 1.0f,  1.0f},
                { 0.0f,  0.0f, 1.0f, 1.0f}
            },
            {
                {-0.5f,  0.5f, 0.0f},
                { 0.0f,  0.0f, 1.0f},
                { 0.0f,  1.0f},
                { 1.0f,  1.0f, 1.0f, 1.0f}
            }
        };

        std::vector<uint32_t> indices =
        {
            0, 1, 2,
            2, 3, 0
        };

        m_quadMesh.Create(
            m_vulkanDevice,
            vertices,
            indices
        );

        m_vulkanSwapchain.Init(m_vulkanContext, m_vulkanDevice, m_window);
        m_vulkanCommands .Init(m_vulkanDevice);
        m_vulkanSync     .Init(m_vulkanDevice);
        m_vulkanPipeline .Init(m_vulkanDevice, m_vulkanSwapchain);
        m_vulkanRenderer .Init(m_vulkanDevice, m_vulkanSwapchain, m_vulkanCommands, m_vulkanSync, m_vulkanPipeline, m_quadMesh);
    }

    // -------------------------------------------------------------------------------------------------------------------------
    // application destructor

    cApplication::~cApplication()
    {
        m_vulkanDevice.WaitIdle();

        m_quadMesh       .Shutdown(m_vulkanDevice);
        m_vulkanPipeline .Shutdown(m_vulkanDevice);
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
        bool wasF11Pressed = false;

        while (!m_window.ShouldClose())
        {
            m_window.PollEvents();
            
            // fullscreen
            bool isF11Pressed = glfwGetKey(m_window.GetWindow(), GLFW_KEY_F11) == GLFW_PRESS;
            if (isF11Pressed && !wasF11Pressed)
            {
                m_window.ToggleFullscreen();
            }

            wasF11Pressed = isF11Pressed;

            // draw frame
            bool isFrameOk = m_vulkanRenderer.DrawFrame();

            if (!isFrameOk || m_window.WasResized())
            {
                m_window.ResetRezisedFlag();
                m_vulkanSwapchain.Recreate(m_vulkanContext, m_vulkanDevice, m_window);
            }
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------

}

// -------------------------------------------------------------------------------------------------------------------------