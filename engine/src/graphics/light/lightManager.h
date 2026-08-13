#pragma once

#include <vector>

namespace Engine::GFX
{
    using LightHandle = int;

    struct sLight;

    namespace LightManager
    {
        LightHandle             CreateLight(const sLight& _rLight);
        sLight&                 GetLight(LightHandle _lightHandle);
        std::vector<sLight>&    GetLights();
    }
}