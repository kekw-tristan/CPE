#include "application.h"

#include "graphics/vertex.h"

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
        , m_cubeMesh()
    {
        m_vulkanContext  .Init(m_window);
        m_vulkanDevice   .Init(m_vulkanContext);
        m_vulkanCommands .Init(m_vulkanDevice);
        m_vulkanSwapchain.Init(m_vulkanContext, m_vulkanDevice, m_window);
        m_vulkanPipeline .Init(m_vulkanDevice, m_vulkanSwapchain);
        m_vulkanRenderer .Init(m_vulkanDevice, m_vulkanSwapchain, m_vulkanCommands, m_vulkanPipeline);


        GFX::sCubeDesc cubeDesc; 

        cubeDesc.width = 1.f; 
        cubeDesc.height = 1.f; 
        cubeDesc.depth = 1.f; 
        cubeDesc.color = { 1.f, 0.2f, 0.1f, 1.f };

        GFX::sMeshData cubeData = GFX::cMeshGenerator::CreateCube(cubeDesc);

        m_cubeMesh.Create(
            m_vulkanDevice,
            m_vulkanCommands,
            cubeData.vertices,
            cubeData.indices
        );

        m_vulkanRenderer.SubmitMesh(m_cubeMesh);

        m_camera.LookAt(
            2.0f, 1.5f, 3.0f,
            0.0f, 0.0f, 0.0f
        );

        m_camera.SetPerspective(
            60.0f,
            0.1f,
            100.0f
        );

        m_Timer.Reset();
        m_input.Init(m_window.GetWindow());
    }

    // -------------------------------------------------------------------------------------------------------------------------
    // application destructor

    cApplication::~cApplication()
    {
        m_vulkanDevice.WaitIdle();

        m_cubeMesh       .Shutdown(m_vulkanDevice);
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
        bool     wasF11Pressed  = false;
        float    fpsTimer       = 0.f;
        uint32_t frameCounter   = 0; 

        while (!m_window.ShouldClose())
        {
            m_window.PollEvents();
            m_input.Update();
            m_Timer.Tick();

            const float deltaTime = m_Timer.GetDeltaTime();

            UpdateCamera(deltaTime);

            // fps
            fpsTimer += deltaTime;
            ++frameCounter;
            if (fpsTimer >= 1.f)
            {
                const float fps = static_cast<float>(frameCounter) / fpsTimer; 
                const float frameTimeMs = 1000.f / fps; 

                std::cout << "Fps: " << fps << " | Frame Time: " << frameTimeMs << " ms" << std::endl;

                fpsTimer = 0;
                frameCounter = 0; 
            }
            
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

    void cApplication::UpdateCamera(float _deltaTime)
    {
        float moveSpeed = 3.0f;

        if (m_input.IsKeyDown(GLFW_KEY_LEFT_SHIFT))
        {
            moveSpeed = 8.0f;
        }
        
        const float moveAmount = moveSpeed * _deltaTime;

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