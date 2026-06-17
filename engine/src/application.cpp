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
        m_vulkanCommands .Init(m_vulkanDevice);
    
    std::vector<Engine::GFX::sVulkanVertex> vertices =
        {
            // Front face (+Z)
            {
                {-0.5f, -0.5f,  0.5f},
                { 0.0f,  0.0f,  1.0f},
                { 0.0f,  0.0f},
                { 1.0f,  0.0f,  0.0f, 1.0f}
            },
            {
                { 0.5f, -0.5f,  0.5f},
                { 0.0f,  0.0f,  1.0f},
                { 1.0f,  0.0f},
                { 1.0f,  0.0f,  0.0f, 1.0f}
            },
            {
                { 0.5f,  0.5f,  0.5f},
                { 0.0f,  0.0f,  1.0f},
                { 1.0f,  1.0f},
                { 1.0f,  0.0f,  0.0f, 1.0f}
            },
            {
                {-0.5f,  0.5f,  0.5f},
                { 0.0f,  0.0f,  1.0f},
                { 0.0f,  1.0f},
                { 1.0f,  0.0f,  0.0f, 1.0f}
            },

            // Back face (-Z)
            {
                { 0.5f, -0.5f, -0.5f},
                { 0.0f,  0.0f, -1.0f},
                { 0.0f,  0.0f},
                { 0.0f,  1.0f,  0.0f, 1.0f}
            },
            {
                {-0.5f, -0.5f, -0.5f},
                { 0.0f,  0.0f, -1.0f},
                { 1.0f,  0.0f},
                { 0.0f,  1.0f,  0.0f, 1.0f}
            },
            {
                {-0.5f,  0.5f, -0.5f},
                { 0.0f,  0.0f, -1.0f},
                { 1.0f,  1.0f},
                { 0.0f,  1.0f,  0.0f, 1.0f}
            },
            {
                { 0.5f,  0.5f, -0.5f},
                { 0.0f,  0.0f, -1.0f},
                { 0.0f,  1.0f},
                { 0.0f,  1.0f,  0.0f, 1.0f}
            },

            // Left face (-X)
            {
                {-0.5f, -0.5f, -0.5f},
                {-1.0f,  0.0f,  0.0f},
                { 0.0f,  0.0f},
                { 0.0f,  0.0f,  1.0f, 1.0f}
            },
            {
                {-0.5f, -0.5f,  0.5f},
                {-1.0f,  0.0f,  0.0f},
                { 1.0f,  0.0f},
                { 0.0f,  0.0f,  1.0f, 1.0f}
            },
            {
                {-0.5f,  0.5f,  0.5f},
                {-1.0f,  0.0f,  0.0f},
                { 1.0f,  1.0f},
                { 0.0f,  0.0f,  1.0f, 1.0f}
            },
            {
                {-0.5f,  0.5f, -0.5f},
                {-1.0f,  0.0f,  0.0f},
                { 0.0f,  1.0f},
                { 0.0f,  0.0f,  1.0f, 1.0f}
            },

            // Right face (+X)
            {
                { 0.5f, -0.5f,  0.5f},
                { 1.0f,  0.0f,  0.0f},
                { 0.0f,  0.0f},
                { 1.0f,  1.0f,  0.0f, 1.0f}
            },
            {
                { 0.5f, -0.5f, -0.5f},
                { 1.0f,  0.0f,  0.0f},
                { 1.0f,  0.0f},
                { 1.0f,  1.0f,  0.0f, 1.0f}
            },
            {
                { 0.5f,  0.5f, -0.5f},
                { 1.0f,  0.0f,  0.0f},
                { 1.0f,  1.0f},
                { 1.0f,  1.0f,  0.0f, 1.0f}
            },
            {
                { 0.5f,  0.5f,  0.5f},
                { 1.0f,  0.0f,  0.0f},
                { 0.0f,  1.0f},
                { 1.0f,  1.0f,  0.0f, 1.0f}
            },

            // Top face (+Y)
            {
                {-0.5f,  0.5f,  0.5f},
                { 0.0f,  1.0f,  0.0f},
                { 0.0f,  0.0f},
                { 1.0f,  0.0f,  1.0f, 1.0f}
            },
            {
                { 0.5f,  0.5f,  0.5f},
                { 0.0f,  1.0f,  0.0f},
                { 1.0f,  0.0f},
                { 1.0f,  0.0f,  1.0f, 1.0f}
            },
            {
                { 0.5f,  0.5f, -0.5f},
                { 0.0f,  1.0f,  0.0f},
                { 1.0f,  1.0f},
                { 1.0f,  0.0f,  1.0f, 1.0f}
            },
            {
                {-0.5f,  0.5f, -0.5f},
                { 0.0f,  1.0f,  0.0f},
                { 0.0f,  1.0f},
                { 1.0f,  0.0f,  1.0f, 1.0f}
            },

            // Bottom face (-Y)
            {
                {-0.5f, -0.5f, -0.5f},
                { 0.0f, -1.0f,  0.0f},
                { 0.0f,  0.0f},
                { 0.0f,  1.0f,  1.0f, 1.0f}
            },
            {
                { 0.5f, -0.5f, -0.5f},
                { 0.0f, -1.0f,  0.0f},
                { 1.0f,  0.0f},
                { 0.0f,  1.0f,  1.0f, 1.0f}
            },
            {
                { 0.5f, -0.5f,  0.5f},
                { 0.0f, -1.0f,  0.0f},
                { 1.0f,  1.0f},
                { 0.0f,  1.0f,  1.0f, 1.0f}
            },
            {
                {-0.5f, -0.5f,  0.5f},
                { 0.0f, -1.0f,  0.0f},
                { 0.0f,  1.0f},
                { 0.0f,  1.0f,  1.0f, 1.0f}
            }
        };

        std::vector<uint32_t> indices =
        {
            // Front
            0, 1, 2,
            2, 3, 0,

            // Back
            4, 5, 6,
            6, 7, 4,

            // Left
            8, 9, 10,
            10, 11, 8,

            // Right
            12, 13, 14,
            14, 15, 12,

            // Top
            16, 17, 18,
            18, 19, 16,

            // Bottom
            20, 21, 22,
            22, 23, 20
        };

        m_quadMesh.Create(
            m_vulkanDevice,
            m_vulkanCommands,
            vertices,
            indices
        );
        m_vulkanSwapchain.Init(m_vulkanContext, m_vulkanDevice, m_window);
        
        m_vulkanPipeline .Init(m_vulkanDevice, m_vulkanSwapchain);
        m_vulkanRenderer .Init(m_vulkanDevice, m_vulkanSwapchain, m_vulkanCommands, m_vulkanPipeline, m_quadMesh);

        m_camera.LookAt(
            2.0f, 1.5f, 3.0f,
            0.0f, 0.0f, 0.0f
        );

        m_camera.SetPerspective(
            60.0f,
            0.1f,
            100.0f
        );

        
        m_input.Init(m_window.GetWindow());
    }

    // -------------------------------------------------------------------------------------------------------------------------
    // application destructor

    cApplication::~cApplication()
    {
        m_vulkanDevice.WaitIdle();

        m_quadMesh       .Shutdown(m_vulkanDevice);
        m_vulkanRenderer .ShutDown();
        m_vulkanPipeline .Shutdown(m_vulkanDevice);
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
            m_input.Update();
            UpdateCamera();
            
            // fullscreen
            if (m_input.WasKeyPressed(GLFW_KEY_F11))
            {
                m_window.ToggleFullscreen();
            }

            // draw frame
            bool isFrameOk = m_vulkanRenderer.DrawFrame(m_camera);

            if (!isFrameOk || m_window.WasResized())
            {
                m_window.ResetRezisedFlag();
                m_vulkanSwapchain.Recreate(m_vulkanContext, m_vulkanDevice, m_window);
                m_vulkanRenderer.RecreateDepthBuffer();
            }
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cApplication::UpdateCamera()
    {
            float moveSpeed = 3.0f;

        if (m_input.IsKeyDown(GLFW_KEY_LEFT_SHIFT))
        {
            moveSpeed = 8.0f;
        }

        const float moveAmount = 0.03f;

        if (m_input.IsKeyDown(GLFW_KEY_W))
        {
            m_camera.MoveForward(moveAmount);
        }

        if (m_input.IsKeyDown(GLFW_KEY_S))
        {
            m_camera.MoveForward(-moveAmount);
        }

        if (m_input.IsKeyDown(GLFW_KEY_D))
        {
            m_camera.MoveRight(moveAmount);
        }

        if (m_input.IsKeyDown(GLFW_KEY_A))
        {
            m_camera.MoveRight(-moveAmount);
        }

        if (m_input.IsKeyDown(GLFW_KEY_E))
        {
            m_camera.MoveUp(moveAmount);
        }

        if (m_input.IsKeyDown(GLFW_KEY_Q))
        {
            m_camera.MoveUp(-moveAmount);
        }

        if (m_input.WasMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT))
        {
            m_input.SetMouseCaptured(true);
        }

        if (m_input.WasKeyPressed(GLFW_KEY_ESCAPE))
        {
            m_input.SetMouseCaptured(false);
        }

        if (m_input.IsMouseCaptured())
        {
            const float mouseSensitivity = 0.12f;

            m_camera.AddYaw(static_cast<float>(m_input.GetMouseDeltaX()) * mouseSensitivity);

            m_camera.AddPitch(-static_cast<float>(m_input.GetMouseDeltaY()) * mouseSensitivity);
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------

}

// -------------------------------------------------------------------------------------------------------------------------