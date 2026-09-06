#pragma once

#include <vector>

namespace Engine::GFX
{
    using LightHandle = int;

    constexpr LightHandle c_invalidLightHandle = -1;

    struct sLight;

    namespace LightManager
    {
        LightHandle                CreateLight(const sLight& _rLight);
        bool                       DestroyLight(LightHandle _lightHandle);
        bool                       UpdateLight(LightHandle _lightHandle, const sLight& _rLight);
        sLight*                    TryGetLight(LightHandle _lightHandle);
        sLight&                    GetLight(LightHandle _lightHandle);
        const std::vector<sLight>& GetLights();
    }
}
