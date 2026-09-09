#pragma once

#include "prefabDesc.h"

namespace Engine::GFX
{
    namespace PrefabManager
    {
        PrefabHandle CreatePrefab(const sPrefabDesc& _rPrefabDesc);

        const sPrefabDesc& GetPrefab(PrefabHandle _prefabHandle);
    }
}