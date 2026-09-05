#pragma once

#include <cstdint>
#include <type_traits>

namespace Engine::GFX
{
    inline constexpr uint32_t c_maxNumberOfHealthBars = 4096;

    struct sHealthBarData
    {
        float positionWidth[4]  = { 0.0f,  0.0f,  0.0f,  1.0f };
        float heightFill[4]     = { 0.12f, 1.0f,  0.0f,  0.0f };
        float color[4]          = { 0.8f,  0.08f, 0.12f, 1.0f };
    };

    static_assert(std::is_standard_layout_v<sHealthBarData>);
    static_assert(sizeof(sHealthBarData) == 48);
}
