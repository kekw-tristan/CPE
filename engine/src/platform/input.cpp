#include "platform/input.h"

#include <stdexcept>

// -------------------------------------------------------------------------------------------------------------------------

namespace Engine::Platform
{
    // -------------------------------------------------------------------------------------------------------------------------

    cInput::cInput()
        : m_pWindow(nullptr)
        , m_mouseX(0.0)
        , m_mouseY(0.0)
        , m_previousMouseX(0.0)
        , m_previousMouseY(0.0)
        , m_mouseDeltaX(0.0)
        , m_mouseDeltaY(0.0)
        , m_firstMouseUpdate(true)
        , m_mouseCaptured(false)
    {
        for (int key = 0; key <= GLFW_KEY_LAST; ++key)
        {
            m_keys[key] = false;
            m_previousKeys[key] = false;
        }

        for (int button = 0; button <= GLFW_MOUSE_BUTTON_LAST; ++button)
        {
            m_mouseButtons[button] = false;
            m_previousMouseButtons[button] = false;
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cInput::Init(GLFWwindow* _pWindow)
    {
        if (_pWindow == nullptr)
        {
            return;
        }

        m_pWindow = _pWindow;

        glfwGetCursorPos(
            m_pWindow,
            &m_mouseX,
            &m_mouseY
        );

        m_previousMouseX = m_mouseX;
        m_previousMouseY = m_mouseY;

        m_mouseDeltaX = 0.0;
        m_mouseDeltaY = 0.0;

        m_firstMouseUpdate = true;
        m_mouseCaptured = false;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cInput::Update()
    {
        if (m_pWindow == nullptr)
        {
            return;
        }

        for (int key = 0; key <= GLFW_KEY_LAST; ++key)
        {
            m_previousKeys[key] = m_keys[key];
            m_keys[key] = glfwGetKey(m_pWindow, key) == GLFW_PRESS;
        }

        for (int button = 0; button <= GLFW_MOUSE_BUTTON_LAST; ++button)
        {
            m_previousMouseButtons[button] = m_mouseButtons[button];
            m_mouseButtons[button] = glfwGetMouseButton(m_pWindow, button) == GLFW_PRESS;
        }

        m_previousMouseX = m_mouseX;
        m_previousMouseY = m_mouseY;

        glfwGetCursorPos(m_pWindow, &m_mouseX, &m_mouseY);

        if (m_firstMouseUpdate)
        {
            m_mouseDeltaX = 0.0;
            m_mouseDeltaY = 0.0;
            m_firstMouseUpdate = false;
        }
        else
        {
            m_mouseDeltaX = m_mouseX - m_previousMouseX;
            m_mouseDeltaY = m_mouseY - m_previousMouseY;
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------

    bool cInput::IsKeyDown(int _key) const
    {
        if (!IsValidKey(_key))
        {
            return false;
        }

        return m_keys[_key];
    }

    // -------------------------------------------------------------------------------------------------------------------------

    bool cInput::WasKeyPressed(int _key) const
    {
        if (!IsValidKey(_key))
        {
            return false;
        }

        return m_keys[_key] && !m_previousKeys[_key];
    }

    // -------------------------------------------------------------------------------------------------------------------------

    bool cInput::WasKeyReleased(int _key) const
    {
        if (!IsValidKey(_key))
        {
            return false;
        }

        return !m_keys[_key] && m_previousKeys[_key];
    }

    // -------------------------------------------------------------------------------------------------------------------------

    bool cInput::IsMouseButtonDown(int _button) const
    {
        if (!IsValidMouseButton(_button))
        {
            return false;
        }

        return m_mouseButtons[_button];
    }

    // -------------------------------------------------------------------------------------------------------------------------

    bool cInput::WasMouseButtonPressed(int _button) const
    {
        if (!IsValidMouseButton(_button))
        {
            return false;
        }

        return m_mouseButtons[_button] && !m_previousMouseButtons[_button];
    }

    // -------------------------------------------------------------------------------------------------------------------------

    bool cInput::WasMouseButtonReleased(int _button) const
    {
        if (!IsValidMouseButton(_button))
        {
            return false;
        }

        return !m_mouseButtons[_button] && m_previousMouseButtons[_button];
    }

    // -------------------------------------------------------------------------------------------------------------------------

    double cInput::GetMouseX() const
    {
        return m_mouseX;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    double cInput::GetMouseY() const
    {
        return m_mouseY;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    double cInput::GetMouseDeltaX() const
    {
        return m_mouseDeltaX;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    double cInput::GetMouseDeltaY() const
    {
        return m_mouseDeltaY;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cInput::SetMouseCaptured(bool _captured)
    {
        if (m_pWindow == nullptr)
        {
            return;
        }

        m_mouseCaptured = _captured;

        glfwSetInputMode(m_pWindow, GLFW_CURSOR, _captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);

        glfwGetCursorPos(m_pWindow, &m_mouseX, &m_mouseY);

        m_previousMouseX = m_mouseX;
        m_previousMouseY = m_mouseY;

        m_mouseDeltaX = 0.0;
        m_mouseDeltaY = 0.0;

        m_firstMouseUpdate = true;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    bool cInput::IsMouseCaptured() const
    {
        return m_mouseCaptured;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    bool cInput::IsValidKey(int _key) const
    {
        return _key >= 0 && _key <= GLFW_KEY_LAST;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    bool cInput::IsValidMouseButton(int _button) const
    {
        return _button >= 0 && _button <= GLFW_MOUSE_BUTTON_LAST;
    }


    // -------------------------------------------------------------------------------------------------------------------------
}