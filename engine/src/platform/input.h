#pragma once

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace Engine::Platform
{
    class cInput
    {
        public:

            cInput();
            ~cInput() = default;

            cInput(const cInput&)               = delete;
            cInput& operator=(const cInput&)    = delete;

        public:
            void Init(GLFWwindow* _pWindow);
            void Update();

            bool IsKeyDown(int _key) const;
            bool WasKeyPressed(int _key) const;
            bool WasKeyReleased(int _key) const;

            bool IsMouseButtonDown(int _button) const;
            bool WasMouseButtonPressed(int _button) const;
            bool WasMouseButtonReleased(int _button) const;

            double GetMouseX() const;
            double GetMouseY() const;

            double GetMouseDeltaX() const;
            double GetMouseDeltaY() const;

            void SetMouseCaptured(bool _captured);
            bool IsMouseCaptured() const;

        private:

            bool IsValidKey(int _key) const;
            bool IsValidMouseButton(int _button) const;

        private:

            GLFWwindow* m_pWindow;

            bool m_keys[GLFW_KEY_LAST + 1];
            bool m_previousKeys[GLFW_KEY_LAST + 1];

            bool m_mouseButtons[GLFW_MOUSE_BUTTON_LAST + 1];
            bool m_previousMouseButtons[GLFW_MOUSE_BUTTON_LAST + 1];

            double m_mouseX;
            double m_mouseY;

            double m_previousMouseX;
            double m_previousMouseY;

            double m_mouseDeltaX;
            double m_mouseDeltaY;

            bool m_firstMouseUpdate;
            bool m_mouseCaptured;
    };
}