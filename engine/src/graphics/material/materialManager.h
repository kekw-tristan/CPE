#pragma once

#include <vector>

namespace Engine::GFX
{
    using MaterialHandle = int;

    struct sMaterial;

    namespace MaterialManager
    {
        MaterialHandle  CreateMaterial(const sMaterial& _rMaterial);
        sMaterial&      GetMaterial(MaterialHandle _shapeModelHandle);
        std::vector<sMaterial>&    GetMaterials();
    }

}