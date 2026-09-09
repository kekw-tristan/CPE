#pragma once

#include <cstdint>

namespace Engine::GFX
{
    namespace sAssetType
    {
        enum Enum
        {
            ShapeModel,
            Prefab,

            NumberOfElements,
            Undefined = -1
        };
    }


    struct sAssetHandle
    {
        sAssetType::Enum type = sAssetType::Undefined;

        int32_t handle = -1;


        bool IsValid() const
        {
            return type != sAssetType::Undefined && handle >= 0;
        }
    };
}